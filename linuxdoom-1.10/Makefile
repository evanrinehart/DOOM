################################################################
#
# $Id:$
#
# $Log:$
#
CC=  gcc  # gcc or g++

CFLAGS=-g -Wall -Wno-unused-const-variable -Wno-unused-but-set-variable -DNORMALUNIX -DLINUX # -DUSEASM
LDFLAGS=-L/usr/X11R6/lib
LIBS=-lXext -lX11 -lm -lstdc++

# subdirectory for objects
O=linux

# not too sophisticated dependency
OBJS=				\
		$(O)/doomdef.o		\
		$(O)/doomstat.o		\
		$(O)/dstrings.o		\
		$(O)/i_system.o		\
		$(O)/i_sound.o		\
		$(O)/i_video.o		\
		$(O)/i_net.o			\
		$(O)/tables.o			\
		$(O)/f_finale.o		\
		$(O)/f_wipe.o 		\
		$(O)/d_main.o			\
		$(O)/d_net.o			\
		$(O)/d_items.o		\
		$(O)/g_game.o			\
		$(O)/m_menu.o			\
		$(O)/m_misc.o			\
		$(O)/m_argv.o  		\
		$(O)/m_bbox.o			\
		$(O)/m_fixed.o		\
		$(O)/m_swap.o			\
		$(O)/m_cheat.o		\
		$(O)/m_random.o		\
		$(O)/am_map.o			\
		$(O)/p_ceilng.o		\
		$(O)/p_doors.o		\
		$(O)/p_enemy.o		\
		$(O)/p_floor.o		\
		$(O)/p_inter.o		\
		$(O)/p_lights.o		\
		$(O)/p_map.o			\
		$(O)/p_maputl.o		\
		$(O)/p_plats.o		\
		$(O)/p_pspr.o			\
		$(O)/p_setup.o		\
		$(O)/p_sight.o		\
		$(O)/p_spec.o			\
		$(O)/p_switch.o		\
		$(O)/p_mobj.o			\
		$(O)/p_telept.o		\
		$(O)/p_tick.o			\
		$(O)/p_saveg.o		\
		$(O)/p_user.o			\
		$(O)/r_bsp.o			\
		$(O)/r_data.o			\
		$(O)/r_draw.o			\
		$(O)/r_main.o			\
		$(O)/r_plane.o		\
		$(O)/r_segs.o			\
		$(O)/r_sky.o			\
		$(O)/r_things.o		\
		$(O)/w_wad.o			\
		$(O)/wi_stuff.o		\
		$(O)/v_video.o		\
		$(O)/st_lib.o			\
		$(O)/st_stuff.o		\
		$(O)/hu_stuff.o		\
		$(O)/hu_lib.o			\
		$(O)/s_sound.o		\
		$(O)/z_zone.o			\
		$(O)/info.o				\
		$(O)/sounds.o

all:	 $(O)/linuxxdoom

clean:
	rm -f *.o *~ *.flc
	rm -f linux/*.o
	rm -f linux/libraylib.a
	rm -f raylib.h
	rm -f adlmidi.h

$(O)/linuxxdoom:	$(OBJS) $(O)/i_main.o $(O)/libraylib.a $(O)/libADLMIDI.a
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(O)/i_main.o $(O)/libraylib.a $(O)/libADLMIDI.a \
	-o $(O)/linuxxdoom $(LIBS)

$(O)/%.o:	%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(O)/i_video.o : i_video.c raylib.h
$(O)/i_sound.o : i_sound.c raylib.h adlmidi.h

#############################################################
#
#############################################################

RAYLIB_VERSION = 6.0
RAYLIB_URL = https://github.com/raysan5/raylib.git

raylib :
	git clone --depth=1 --branch=$(RAYLIB_VERSION) $(RAYLIB_URL) raylib

raylib.h : raylib
	cp raylib/src/raylib.h .

$(O)/libraylib.a : raylib
	$(MAKE) -C raylib/src
	cp raylib/src/libraylib.a $(O)/

ADLMIDI_VERSION = v1.6.1
ADLMIDI_URL = https://github.com/Wohlstand/libADLMIDI

adlmidi :
	git clone --depth=1 --branch=$(ADLMIDI_VERSION) $(ADLMIDI_URL) adlmidi

adlmidi.h : adlmidi
	cp adlmidi/include/adlmidi.h .

$(O)/libADLMIDI.a : adlmidi
	mkdir -p adlmidi/build
	cd adlmidi/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make
	cp adlmidi/include/adlmidi.h .
	cp adlmidi/build/libADLMIDI.a linux/
