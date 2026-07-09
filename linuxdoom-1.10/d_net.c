//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM Network game communication and protocol,
//	all OS independend parts.
//
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <limits.h>

#include "m_menu.h"
#include "i_system.h"
#include "i_video.h"
#include "i_net.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"

#include "netscope.h"

#define	NCMD_EXIT		0x80000000
#define	NCMD_RETRANSMIT		0x40000000
#define	NCMD_SETUP		0x20000000
#define	NCMD_KILL		0x10000000	// kill game
#define	NCMD_CHECKSUM	 	0x0fffffff

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))
 
doomcom_t*	doomcom;	
doomdata_t*	netbuffer;		// points inside doomcom


//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// nettics[] has the maketics for all players 
//
// a gametic cannot be run until nettics[] > gametic for all players
//
#define	RESENDCOUNT	10
#define	PL_DRONE	0x80	// bit flag in doomdata->player

ticcmd_t	localcmds[BACKUPTICS];

ticcmd_t        netcmds[MAXPLAYERS][BACKUPTICS];
int         	nettics[MAXNETNODES];
boolean		nodeingame[MAXNETNODES];		// set false as nodes leave game
boolean		remoteresend[MAXNETNODES];		// set when local needs tics
int		resendto[MAXNETNODES];			// set when remote needs tics
int		resendcount[MAXNETNODES];

int		nodeforplayer[MAXPLAYERS];

int             maketic;
int		lastnettic;
int		skiptics;
int		ticdup;		
int		maxsend;	// BACKUPTICS/(2*ticdup)-1


void D_ProcessEvents (void); 
void G_BuildTiccmd (ticcmd_t *cmd); 
void D_DoAdvanceDemo (void);
void D_Display(void);
void S_UpdateSounds(void* listener_p);
 
boolean		reboundpacket;
doomdata_t	reboundstore;

extern	boolean	advancedemo;

char    exitmsg[80];

struct netstatus netstatus_object;
struct netstatus *netstatus = &netstatus_object;

boolean HGetPacket (void);
void HSendPacket (int node, int flags);
int ExpandTics (int low);


// send what needs to be sent to node i
void N_ServiceNode(int i) {
    if (!nodeingame[i]) return;
    if (resendto[i] == maketic) return; // nothing to send

    // resendto[i] determines how much we're sending
    int start = resendto[i] - (doomcom->extratics ? 1 : 0);
    int end = maketic - 1;
    int N = end - start + 1;

    if (N > BACKUPTICS) I_Error ("NetUpdate: netbuffer->numtics > BACKUPTICS");

    // prepare the packet
    netbuffer->player = consoleplayer;
    netbuffer->starttic = start;
    netbuffer->numtics = N;
    for (int j=0; j < N; j++)
        netbuffer->cmds[j] = localcmds[(start + j) % BACKUPTICS];

    // send the packet
    if (remoteresend[i])
    {
        netbuffer->retransmitfrom = nettics[i];
        HSendPacket(i, NCMD_RETRANSMIT);
    }
    else
    {
        netbuffer->retransmitfrom = 0;
        HSendPacket(i, 0);
    }

    // nothing left to send to node i at this time
    resendto[i] = maketic;
}

// Vomiter
// send new (or old) localcmds to all nodes and include necessary retransmit request bits
void N_Vomit(void) {
    for (int i=0; i < doomcom->numnodes; i++) N_ServiceNode(i);
}

// Solidifier
// process 1 incoming packet if any. Returns 0 if there was none.
int N_Solidify(void) {
    if (!HGetPacket()) return 0;

    // extra setup packet
    if (netbuffer->checksum & NCMD_SETUP) return 1;

    int netconsole = netbuffer->player & ~PL_DRONE;
    int netnode = doomcom->remotenode;

    // to save bytes, only the low byte of tic numbers are sent
    // Figure out what the rest of the bytes are
    int realstart = ExpandTics (netbuffer->starttic);
    int realend = (realstart+netbuffer->numtics);

    // check for exiting the game
    if (netbuffer->checksum & NCMD_EXIT)
    {
        if (!nodeingame[netnode]) return 1;
        nodeingame[netnode] = false;
        playeringame[netconsole] = false;
        printf("Player %d left the game\n", netconsole + 1);
        strcpy (exitmsg, "Player 1 left the game");
        exitmsg[7] += netconsole;
        players[consoleplayer].message = exitmsg;
        if (demorecording) G_CheckDemoStatus ();
        return 1;
    }

    // check for a remote game kill
    if (netbuffer->checksum & NCMD_KILL) I_Error ("Killed by network driver");

    // learn node for player if we don't already know
    nodeforplayer[netconsole] = netnode;
    netstatus->players[netnode] = netconsole;
    netstatus->num_nodes = doomcom->numnodes;
    netstatus->contact_time[netnode] = I_GetMonotime();

    // check for retransmit request
    if ( resendcount[netnode] <= 0 && (netbuffer->checksum & NCMD_RETRANSMIT) )
    {
        resendto[netnode] = ExpandTics(netbuffer->retransmitfrom);
        resendcount[netnode] = RESENDCOUNT;
    }
    else {
        resendcount[netnode]--;
    }

    // check for out of order / duplicated packet
    if (realend == nettics[netnode]) return 1;
    if (realend < nettics[netnode]) return 1;

    // check for a missed packet
    if (realstart > nettics[netnode]) {
        netstatus->botch_start[netnode] = realstart;
        netstatus->botch_num[netnode] = netbuffer->numtics;
        netstatus->recent_botch = realstart;
        remoteresend[netnode] = true;
        return 1;
    }

    netstatus->botch_start[netnode] = 0;

    // a good packet, copy into netcmds
    remoteresend[netnode] = false;

    int start = nettics[netnode] - realstart; // this formula skips copying cmds we already have
    ticcmd_t *src = &netbuffer->cmds[start];

    while (nettics[netnode] < realend) {
        netcmds[netconsole][nettics[netnode]%BACKUPTICS] = *src++;
        nettics[netnode]++;
    }

    netstatus->nettics[netnode] = nettics[netnode];

    return 1;
}


void MenuOnlyTic(void) {
    I_PumpEvents();
    D_ProcessEvents();
    M_Ticker();
}

// Evil Eye
// can only be called if there is room in the buffer
// should not be called if we're way ahead of peers
void MakeTic(ticcmd_t *cmdbuf) {
    I_PumpEvents();
    D_ProcessEvents();
    G_BuildTiccmd(&cmdbuf[maketic % BACKUPTICS]);
    maketic++;
    netstatus->maketic = maketic;
}

// Dreameater
// can only be called if all players netcmds are available
void GameTic(void) {
    if (advancedemo) D_DoAdvanceDemo ();
    M_Ticker();
    G_Ticker();
    gametic++;
    netstatus->gametic = gametic / ticdup;
}

// when -dup N is given, the same cmd is applied N times
void GameTicDuplicated(int N) {
    for (int i = 0; i < N; i++) {
        // modify command for duplicated tics
        if (i > 0)
            for (int p = 0; p < MAXPLAYERS; p++) {
                ticcmd_t *cmd = &netcmds[p][(gametic / N) % BACKUPTICS];
                cmd->chatchar = 0;
                if (cmd->buttons & BT_SPECIAL) cmd->buttons = 0;
            }
        GameTic();
    }
}

// the least nettic of all nodes, which is like a remote maketic
int N_Lowtic(void) {
    int lowtic = INT_MAX;
	for (int i=0 ; i<doomcom->numnodes ; i++)
	    if (nodeingame[i] && nettics[i] < lowtic)
		lowtic = nettics[i];
    return lowtic;
}


// used for -timedemo
void TimeCore(void) {
    for (;;) {
        MakeTic(netcmds[consoleplayer]);
        GameTic();
        I_StartFrame();
        S_UpdateSounds(players[consoleplayer].mo);
        D_Display();
    }
}

// not used but can be used for -playdemo or single player
void DemoCore(void) {
    int prev = I_GetTime();
    for (;;) {
        int now = I_GetTime();
        int ticks = now - prev;
        prev = now;

        if (ticks > 15) continue; // negate wipe effect

        for (int i = 0; i < ticks; i++) {
            MakeTic(netcmds[consoleplayer]); // mainly to keep the menu responsive
            GameTic();
        }

        if (ticks > 0) {
            I_StartFrame();
            S_UpdateSounds(players[consoleplayer].mo);
            D_Display();
        }

        I_Sleep(0.001);
    }
}



// adaptive timing for netgame
int netgame_prevtime;
int netgame_nitro;
int netgame_brakes;
int netgame_frame;
int frameskip[4];
int frameon;


// ideally nettics[0] should be 1 - 3 tics above lowtic
// if we are consistently slower, speed up time.
void N_Adaptive(void) {

    int key;
    for (key = 0; key < MAXPLAYERS; key++) if (playeringame[key]) break;
    if (consoleplayer == key) return; // key player does not adapt

    int keynode = nodeforplayer[key];

    // if we're not ahead, speed up
    if (nettics[0] <= nettics[keynode]) {
        netgame_nitro++;
    }

    int lag = 2;

    // if we're consistently ahead, slow down
    frameon++;
    frameskip[frameon & 3] = (nettics[0] > nettics[keynode] + lag);
    if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3]) {
        netgame_brakes = 1;
    }

}

// runs a netgame, singleplayer game, or singledemo once engine is set up
void NetgameCore(void) {

    netgame_prevtime = I_GetTime() / ticdup;

    for (;;) {

        netstatus->current_time = I_GetMonotime();

        // use clock to drive the production of localcmds which are immediately sent off
        int now = I_GetTime() / ticdup;
        int ticks = now - netgame_prevtime;
        netgame_prevtime = now;

        bool wayahead = maketic - gametic/ticdup >= BACKUPTICS/2 - 1; // originally from the evil eye
        int produce = ticks + netgame_nitro; netgame_nitro = 0; // if any, nitro injected
        if (netgame_nitro) netstatus->recent_nitro = maketic;
        netstatus->wayahead = wayahead;

        for (int i = 0; i < produce; i++) {
            if (wayahead) { continue; }
            if (netgame_brakes > 0) netstatus->recent_brakes = maketic;
            if (netgame_brakes > 0) { netgame_brakes--; continue; }
            MakeTic(localcmds); // poll user input, cache, assemble cmd, maketic++
            N_Vomit();          // broadcast cmd to all nodes
            N_Solidify();       // receive your own packet precisely
        }

        // filter and stash incoming packets to netcmds, honor retransmit requests
        while (N_Solidify()) { ; }
        N_Vomit();

        // advance the game if possible by consuming netcmds
        {
            int consume = N_Lowtic() - gametic/ticdup;
            for (int i = 0; i < consume; i++) { GameTicDuplicated(ticdup); }
        }

        netstatus->lowtic = N_Lowtic();

        // occasionally render something
        if (ticks > 0) {
            I_StartFrame (); // frame syncronous IO operations
            S_UpdateSounds(players[consoleplayer].mo); // move positional sounds
            D_Display(); // freezes momentarily during wipe effect but adjusts our clock remotely

            N_Adaptive(); // judge if nitro or brakes are needed
        }

        // don't freeze menu if we're way ahead for a long time
        if (ticks > 0 && wayahead) MenuOnlyTic();

        // don't hammer the CPU
        I_Sleep(0.001);
    }
}






//
//
//
int NetbufferSize (void)
{
    return (intptr_t)&(((doomdata_t *)0)->cmds[netbuffer->numtics]);
}

//
// Checksum 
//
unsigned NetbufferChecksum (void)
{
    unsigned		c;
    int		i,l;

    c = 0x1234567;

    // FIXME -endianess?
#ifdef NORMALUNIX
    return 0;			// byte order problems
#endif

    l = (NetbufferSize () - (intptr_t)&(((doomdata_t *)0)->retransmitfrom))/4;
    for (i=0 ; i<l ; i++)
	c += ((unsigned *)&netbuffer->retransmitfrom)[i] * (i+1);

    return c & NCMD_CHECKSUM;
}

//
//
//
int ExpandTics (int low)
{
    int	delta;
	
    delta = low - (maketic&0xff);
	
    if (delta >= -64 && delta <= 64)
	return (maketic&~0xff) + low;
    if (delta > 64)
	return (maketic&~0xff) - 256 + low;
    if (delta < -64)
	return (maketic&~0xff) + 256 + low;
		
    I_Error ("ExpandTics: strange value %i at maketic %i",low,maketic);
    return 0;
}



//
// HSendPacket
//
void
HSendPacket
 (int	node,
  int	flags )
{
    netbuffer->checksum = NetbufferChecksum () | flags;

    if (!node)
    {
	reboundstore = *netbuffer;
	reboundpacket = true;
	return;
    }

    if (demoplayback)
	return;

    if (!netgame)
	I_Error ("Tried to transmit to another node");
		
    doomcom->command = CMD_SEND;
    doomcom->remotenode = node;
    doomcom->datalength = NetbufferSize ();
	
    if (debugfile)
    {
	int		i;
	int		realretrans;
	if (netbuffer->checksum & NCMD_RETRANSMIT)
	    realretrans = ExpandTics (netbuffer->retransmitfrom);
	else
	    realretrans = -1;

	fprintf (debugfile,"send (%i + %i, R %i) [%i] ",
		 ExpandTics(netbuffer->starttic),
		 netbuffer->numtics, realretrans, doomcom->datalength);
	
	for (i=0 ; i<doomcom->datalength ; i++)
	    fprintf (debugfile,"%i ",((byte *)netbuffer)[i]);

	fprintf (debugfile,"\n");
    }

    I_NetCmd ();
}

//
// HGetPacket
// Returns false if no packet is waiting
//
boolean HGetPacket (void)
{	
    if (reboundpacket)
    {
	*netbuffer = reboundstore;
	doomcom->remotenode = 0;
	reboundpacket = false;
	return true;
    }

    if (!netgame)
	return false;

    if (demoplayback)
	return false;
		
    doomcom->command = CMD_GET;
    I_NetCmd ();
    
    if (doomcom->remotenode == -1)
	return false;

    if (doomcom->datalength != NetbufferSize ())
    {
	if (debugfile)
	    fprintf (debugfile,"bad packet length %i\n",doomcom->datalength);
	return false;
    }
	
    if (NetbufferChecksum () != (netbuffer->checksum&NCMD_CHECKSUM) )
    {
	if (debugfile)
	    fprintf (debugfile,"bad packet checksum\n");
	return false;
    }

    if (debugfile)
    {
	int		realretrans;
	int	i;
			
	if (netbuffer->checksum & NCMD_SETUP)
	    fprintf (debugfile,"setup packet\n");
	else
	{
	    if (netbuffer->checksum & NCMD_RETRANSMIT)
		realretrans = ExpandTics (netbuffer->retransmitfrom);
	    else
		realretrans = -1;
	    
	    fprintf (debugfile,"get %i = (%i + %i, R %i)[%i] ",
		     doomcom->remotenode,
		     ExpandTics(netbuffer->starttic),
		     netbuffer->numtics, realretrans, doomcom->datalength);

	    for (i=0 ; i<doomcom->datalength ; i++)
		fprintf (debugfile,"%i ",((byte *)netbuffer)[i]);
	    fprintf (debugfile,"\n");
	}
    }
    return true;	
}



//
// CheckAbort
//
void CheckAbort (void)
{
    event_t *ev;
    int		stoptic;

// this code is currently useless since it is used before graphics (events) are initialized
	
    stoptic = I_GetTime () + 2; 
    while (I_GetTime() < stoptic) 
	I_StartTic (); 
	
    I_StartTic ();
    for ( ; eventtail != eventhead 
	      ; eventtail = (eventtail)&(MAXEVENTS-1) )
    { 
	ev = &events[eventtail++];
	if (ev->type == ev_keydown && ev->data1 == KEY_ESCAPE)
	    I_Error ("Network game synchronization aborted.");
    } 
}

const char *stringify_doomdata(doomdata_t *dd);
const char *stringify_doomdata_setup(doomdata_t *dd);

//
// D_ArbitrateNetStart
//
void D_ArbitrateNetStart (void)
{
    boolean	gotinfo[MAXNETNODES];
	
    autostart = true;
    memset (gotinfo,0,sizeof(gotinfo));
	
    if (doomcom->consoleplayer)
    {
	// listen for setup info from key player
	printf ("Arbiter: begin doomshake as player %d (subordinate)\n", doomcom->consoleplayer + 1);
	printf ("Arbiter: listening for network start info...\n");
	while (1)
	{
	    CheckAbort ();
	    if (!HGetPacket ())
		continue;
	    if (netbuffer->checksum & NCMD_SETUP)
	    {
		printf("Arbiter: got setup packet, version = %d, ", netbuffer->player);
		if (netbuffer->player != VERSION)
		    I_Error ("Different DOOM versions cannot play a net game!");
		printf("matching our own\n");
		startskill = netbuffer->retransmitfrom & 7;
		deathmatch = (netbuffer->retransmitfrom & 0xc0) >> 6;
		nomonsters = (netbuffer->retransmitfrom & 0x20) > 0;
		respawnparm = (netbuffer->retransmitfrom & 0x10) > 0;
		pistolstart = (netbuffer->retransmitfrom & 0x08) > 0;
		spthings = (netbuffer->retransmitfrom & 0x40) > 0;
		startmap = netbuffer->starttic & 0x3f;
		startepisode = netbuffer->starttic >> 6;
		printf("Arbiter: got game config %s\n", stringify_doomdata_setup(netbuffer));
		printf("Arbiter: done here. Continuing with game bootup.\n");
		return;
	    }
	}
    }
    else
    {
	// key player, send the setup info
        doomdata_t setup;
        setup.retransmitfrom = startskill;
        if (pistolstart) setup.retransmitfrom |= 0x08;
        if (spthings) setup.retransmitfrom |= 0x40;
        if (deathmatch) setup.retransmitfrom |= (deathmatch<<6);
        if (nomonsters) setup.retransmitfrom |= 0x20;
        if (respawnparm) setup.retransmitfrom |= 0x10;
        setup.starttic = startepisode * 64 + startmap;
        setup.player = VERSION;
        setup.numtics = 0;

        char who[64];

        printf ("Arbiter: begin doomshake as player %d (leader)\n", doomcom->consoleplayer + 1);
        printf ("Arbiter: spamming network start info... %s\n", stringify_doomdata_setup(&setup));
        int spam = 0;
        int left = doomcom->numnodes - 1;
        while (left > 0) {
	    CheckAbort ();

            char *p = who;
            for (int n = 0; n < doomcom->numnodes; n++) p += sprintf(who, "%s%d", n?"":", ", n);
            int count = 0;
            for (int n = 1; n < doomcom->numnodes; n++) { if (gotinfo[n]) count++; }

            for (int i=0 ; i<doomcom->numnodes ; i++)
	    {
		spam++;
		printf("Arbiter: sending setup to node %s. %d/%d nodes responding. spam = %-20d\r",
		    who, count, doomcom->numnodes-1, spam);
		*netbuffer = setup;
		HSendPacket (i, NCMD_SETUP);
	    }

#if 1
	    int i;
	    for(i = 10 ; i  &&  HGetPacket(); --i)
	    {
		int node = netbuffer->player & 0x7f; // in setup only, this is node not player
		if(node < MAXNETNODES) {
		    if (gotinfo[node]) continue;

		    gotinfo[node] = true;
		    if (spam > 0) { printf("\n"); spam = 0; }
		    left--;
		    printf("Arbiter: got packet from node %d. %d left to go.\n", node, left);
		}
	    }
#else
	    while (HGetPacket ())
	    {
		gotinfo[netbuffer->player&0x7f] = true;
	    }
#endif

	}
	printf("Arbiter: all nodes are trying to play! D_ArbitrateNetStart done\n");
    }
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//

void D_CheckNetGame (void)
{
    int             i;
	
    for (i=0 ; i<MAXNETNODES ; i++)
    {
	nodeingame[i] = false;
       	nettics[i] = 0;
	remoteresend[i] = false;	// set when local needs tics
	resendto[i] = 0;		// which tic to start sending
    }
	
    // I_InitNetwork sets doomcom and netgame
    I_InitNetwork ();
    if (doomcom->id != DOOMCOM_ID)
	I_Error ("Doomcom buffer invalid!");
    
    netbuffer = &doomcom->data;
    consoleplayer = displayplayer = doomcom->consoleplayer;
    if (netgame)
	D_ArbitrateNetStart ();

    printf("skill=%d ", startskill + 1);
    if (deathmatch) printf("deathmatch=%d ", deathmatch);
    if (nomonsters) printf("nomonsters=1 ");
    if (fastparm) printf("fastmonsters=1 ");
    if (respawnparm) printf("respawn=1 ");
    if (pistolstart) printf("pistolstart=1 ");
    if (spthings) printf("spthings=1 ");
    printf("episode=%d ", startepisode);
    printf("map=%d\n", startmap);
	
    // read values out of doomcom
    ticdup = doomcom->ticdup;
    maxsend = BACKUPTICS/(2*ticdup)-1;
    if (maxsend<1)
	maxsend = 1;
			
    for (i=0 ; i<doomcom->numplayers ; i++)
	playeringame[i] = true;
    for (i=0 ; i<doomcom->numnodes ; i++)
	nodeingame[i] = true;
	
    printf ("player %i of %i (%i nodes)\n",
	    consoleplayer+1, doomcom->numplayers, doomcom->numnodes);

}


//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame (void)
{
    int             i, j;
	
    if (debugfile)
	fclose (debugfile);
		
    if (!netgame || !usergame || consoleplayer == -1 || demoplayback)
	return;
	
    // send a bunch of packets for security
    netbuffer->player = consoleplayer;
    netbuffer->numtics = 0;
    for (i=0 ; i<4 ; i++)
    {
	for (j=1 ; j<doomcom->numnodes ; j++)
	    if (nodeingame[j])
		HSendPacket (j, NCMD_EXIT);
	I_WaitVBL (1);
    }
}



char ticcmd_string[256];
const char *stringify_ticcmd(ticcmd_t *cmd) {
    char *p = ticcmd_string;
    p += sprintf(p, "Cmd{");
    p += sprintf(p, "fwd=%d ", cmd->forwardmove);
    p += sprintf(p, "side=%d ", cmd->sidemove);
    p += sprintf(p, "turn=%d ", cmd->angleturn);
    p += sprintf(p, "con=%d ", cmd->consistancy);
    p += sprintf(p, "char=%u ", cmd->chatchar);
    p += sprintf(p, "buttons=%08x", cmd->buttons);
    p += sprintf(p, "}");
    return ticcmd_string;
}

char doomdata_string[1024];
const char *stringify_doomdata(doomdata_t *dd) {
    char *p = doomdata_string;
    p += sprintf(p, "Doomdata{");
    p += sprintf(p, "chk=%x ", dd->checksum);
    p += sprintf(p, "ret=%u ", dd->retransmitfrom);
    p += sprintf(p, "strt=%u ", dd->starttic);
    p += sprintf(p, "plyr=%u ", dd->player);
    p += sprintf(p, "num=%u ", dd->numtics);
    p += sprintf(p, "cmds=[");
    for (int i = 0; i < dd->numtics; i++)
        p += sprintf(p, "%s%s", i?" ":"", stringify_ticcmd(&dd->cmds[i]));
    p += sprintf(p, "]");
    p += sprintf(p, "}");
    return doomdata_string;
}

const char *stringify_doomdata_setup(doomdata_t *dd) {
    char *p = doomdata_string;
    p += sprintf(p, "Doomdata{");
    p += sprintf(p, "ver=%d ", dd->player);
    p += sprintf(p, "skill=%d ", 1 + (dd->retransmitfrom & 7));
    p += sprintf(p, "deth=%d ", (dd->retransmitfrom & 0xc0) >> 6);
    p += sprintf(p, "nomon=%d ", (dd->retransmitfrom & 0x20) > 0);
    p += sprintf(p, "resp=%d ", (dd->retransmitfrom & 0x10) > 0);
    p += sprintf(p, "pistol=%d ", (dd->retransmitfrom & 0x08) > 0);
    p += sprintf(p, "spthings=%d ", (dd->retransmitfrom & 0x40) > 0);
    p += sprintf(p, "epi=%d ", dd->starttic >> 6);
    p += sprintf(p, "map=%d", dd->starttic & 0x3f);
    p += sprintf(p, "}");
    return doomdata_string;
}
