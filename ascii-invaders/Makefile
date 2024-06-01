# PLAT := arm-linux-androideabi-
#PLAT := STAGING_DIR=/home/wmt/work_mt300nv2/openwrt-toolchain-23.05.3-ramips-mt76x8_gcc-12.3.0_musl.Linux-x86_64/toolchain-mipsel_24kc_gcc-12.3.0_musl /home/wmt/work_mt300nv2/openwrt-toolchain-23.05.3-ramips-mt76x8_gcc-12.3.0_musl.Linux-x86_64/toolchain-mipsel_24kc_gcc-12.3.0_musl/bin/mipsel-openwrt-linux-

# ncurses-6.4-20230603.tar.gz

CC := $(PLAT)gcc 
CPP := $(PLAT)g++
LD := $(PLAT)ld
AR := $(PLAT)ar rcu 
RANLIB := $(PLAT)ranlib
STRIP := $(PLAT)strip
RM := rm -f

CFLAGS := 
#CFLAGS += -g -O2
CFLAGS += -g3 -O0
CFLAGS += -DHAVE_CONFIG_H -DBUILDING_NCURSES -D_DEFAULT_SOURCE 
CFLAGS += -D_XOPEN_SOURCE=600 -DNCURSES_STATIC 
CFLAGS += --param max-inline-insns-single=1200 
#CFLAGS += -DNDEBUG -O2    
CFLAGS += -I. -I./ncurses-snapshots-6_4_20230603/include 
CFLAGS += -I./ncurses-snapshots-6_4_20230603/ncurses

objects :=

objects += ncurses-snapshots-6_4_20230603/ncurses/lib_gen.o
objects += ncurses-snapshots-6_4_20230603/ncurses/expanded.o
objects += ncurses-snapshots-6_4_20230603/ncurses/codes.o
objects += ncurses-snapshots-6_4_20230603/ncurses/comp_captab.o
objects += ncurses-snapshots-6_4_20230603/ncurses/comp_userdefs.o
objects += ncurses-snapshots-6_4_20230603/ncurses/fallback.o
objects += ncurses-snapshots-6_4_20230603/ncurses/lib_keyname.o
objects += ncurses-snapshots-6_4_20230603/ncurses/names.o
objects += ncurses-snapshots-6_4_20230603/ncurses/unctrl.o

objects += ncurses-snapshots-6_4_20230603/ncurses/tty/hardscroll.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tty/hashmap.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tty/lib_tstp.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tty/lib_vidattr.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tty/lib_mvcur.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tty/tty_update.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tty/lib_twait.o

objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_addch.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_addstr.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_beep.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_bkgd.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_box.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_chgat.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_clear.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_clearok.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_clrbot.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_clreol.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_color.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_colorset.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_delch.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_delwin.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_echo.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_endwin.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_erase.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_flash.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_getch.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_getstr.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_hline.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_immedok.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_inchstr.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_initscr.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_insch.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_insdel.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_insnstr.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_instr.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_isendwin.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_leaveok.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_mouse.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_move.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/define_key.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/key_defined.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/keybound.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/keyok.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/version.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_mvwin.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_newterm.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_newwin.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_nl.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_overlay.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_pad.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_printw.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_redrawln.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_refresh.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_restart.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_scanw.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_screen.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_scroll.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_scrollok.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_scrreg.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_set_term.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slk.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkatr_set.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkatrof.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkatron.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkatrset.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkattr.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkclear.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkcolor.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkinit.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slklab.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkrefr.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slkset.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_slktouch.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_touch.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_ungetch.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_vline.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_wattroff.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_wattron.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_winch.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_window.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/nc_panel.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/safe_sprintf.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/vsscanf.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_freeall.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/legacy_coding.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/lib_dft_fgbg.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/new_pair.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/resizeterm.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/use_window.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/wresize.o
objects += ncurses-snapshots-6_4_20230603/ncurses/base/tries.o

objects += ncurses-snapshots-6_4_20230603/ncurses/trace/varargs.o

objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_print.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/use_screen.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/access.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/add_tries.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/alloc_ttype.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/comp_error.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/comp_hash.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/db_iterator.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/doalloc.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/entries.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/free_ttype.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/getenv_num.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/home_terminfo.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/init_keytry.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_acs.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_baudrate.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_cur_term.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_data.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_has_cap.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_kernel.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_longname.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_napms.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_options.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_raw.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_setup.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_termcap.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_termname.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_tgoto.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_ti.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_tparm.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_tputs.o
objects += ncurses-snapshots-6_4_20230603/ncurses/trace/lib_trace.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/lib_ttyflags.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/name_match.o

objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/obsolete.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/read_entry.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/read_termcap.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/strings.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/trim_sgr0.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/alloc_entry.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/captoinfo.o 
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/comp_expand.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/comp_parse.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/comp_scan.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/parse_entry.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/write_entry.o
objects += ncurses-snapshots-6_4_20230603/ncurses/tinfo/hashed_db.o

objects += ncurses-snapshots-6_4_20230603/ncurses/trace/visbuf.o

TARGET := ascii_invaders

all : $(TARGET)

$(TARGET) : libncurses.a
	$(CC) $(CFLAGS) -o $@ invaders.c libncurses.a
	
#$(STRIP) $@

libncurses.a : $(objects)
	$(AR) $@ $(objects) 
	$(RANLIB) $@

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	$(RM) $(objects) *.a *.exe $(TARGET)

scp:
	scp ./$(TARGET) root@192.168.8.1:/dev/shm/

#scp ./$(TARGET) root@192.168.8.1:/tmp

ssh:
	ssh root@192.168.8.1

#
#TERM=xterm-256color TERMINFO=. ./ascii_invaders
#TERMINFO=/usr/share/terminfo
#
