BINNAME=doom

CC=gcc  # gcc or clang

BASE_CFLAGS = -Wall -Wextra -std=c11 -Wno-unused-parameter -Wno-missing-field-initializers -DNORMALUNIX
CF ?= -O2
CFLAGS=$(CF) $(BASE_CFLAGS)
LDFLAGS=-L/usr/X11R6/lib
LIBS=-lX11 -lm -lstdc++

# subdirectory for objects
O=out

# not too sophisticated dependency
OBJS=				\
		$(O)/doomdef.o		\
		$(O)/doomstat.o		\
		$(O)/dstrings.o		\
		$(O)/i_main.o		\
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
		$(O)/sounds.o   \
		$(O)/x_mapinfo.o   \
		$(O)/i_endoom.o

all:	 $(BINNAME)

clean:
	rm -f *.o *~ *.flc
	rm -f $(O)/*.o
	rm -f $(BINNAME)

$(O):
	mkdir -p $(O)

#############################################################
#
#############################################################

RAYLIB_VERSION = 6.0
RAYLIB_URL = https://github.com/raysan5/raylib.git
RAYLIB = raylib/README.md
RAYLIB_LIB = raylib/src/libraylib.a

$(RAYLIB) :
	git clone --depth=1 --branch=$(RAYLIB_VERSION) $(RAYLIB_URL) raylib

$(RAYLIB_LIB) : $(RAYLIB)
	$(MAKE) -C raylib/src

ADLMIDI_VERSION = v1.6.1
ADLMIDI_URL = https://github.com/Wohlstand/libADLMIDI
ADLMIDI = adlmidi/README.md
ADLMIDI_LIB = adlmidi/build/libADLMIDI.a

$(ADLMIDI) :
	git clone --depth=1 --branch=$(ADLMIDI_VERSION) $(ADLMIDI_URL) adlmidi

$(ADLMIDI_LIB) : $(ADLMIDI)
	mkdir -p adlmidi/build
	cd adlmidi/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make

#############################################################
#
#############################################################

GREEN := \033[1;32m
CYAN  := \033[1;36m
RED   := \033[1;31m
RESET := \033[0m

$(O)/%.o:	%.c | $(O)
	@printf "$(CYAN)%-10s$(RESET) %s %10s -> %s\n" "compile" "$(CF)" "$<" "$@"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(O)/i_video.o : i_video.c $(RAYLIB)
	@printf "$(CYAN)%-10s$(RESET) %s %10s -> %s\n" "compile" "$(CF)" "$<" "$@"
	@$(CC) $(CFLAGS) -c -o $@ -Iraylib/src i_video.c

$(O)/i_sound.o : i_sound.c $(ADLMIDI) $(RAYLIB)
	@printf "$(CYAN)%-10s$(RESET) %s %10s -> %s\n" "compile" "$(CF)" "$<" "$@"
	@$(CC) $(CFLAGS) -c -o $@ -Iadlmidi/include -Iraylib/src i_sound.c

$(BINNAME): $(OBJS) $(RAYLIB_LIB) $(ADLMIDI_LIB)
	@printf "$(GREEN)%-10s$(RESET) %s %10s %s %s -> %s\n" "link" "$(LIBS)" "$(OBJS)" "$(RAYLIB_LIB)" "$(ADLMIDI_LIB)" "$@"
	@$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(RAYLIB_LIB) $(ADLMIDI_LIB) \
	-o $(BINNAME) $(LIBS)
