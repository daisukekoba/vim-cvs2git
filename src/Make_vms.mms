#
# Makefile for Vim on OpenVMS
#
# Maintainer:   Zoltan Arpadffy <arpadffy@polarfox.com>
# Last change:  2003 Apr 25
#
# This has script been tested on VMS 6.2 to 7.3 on DEC Alpha and VAX
# with MMS and MMK
#
# The following could be built:
#	vim.exe:	standard (terminal, GUI/Motif, GUI/GTK)
#	dvim.exe:	debug
#
# Edit the lines in the Configuration section below for fine tuning.
#
# To build:    mms/descrip=Make_vms.mms
# To clean up: mms/descrip=Make_vms.mms clean
#
# Hints and detailed description could be found in INSTALLVMS.TXT file.
#
######################################################################
# Configuration section.
######################################################################
# Platform selection
# Define this if you will use the VAX platform to build.
# VAX = YES

# VMS version
# Uncomment if you use VMS version 6.2 or older
# OLD_VMS = YES

# Compiler selection.
# Comment out if you use the VAXC compiler
DECC = YES

# Build model selection
# TINY   - Almost no features enabled, not even multiple windows
# SMALL  - Few features enabled, as basic as possible
# NORMAL - A default selection of features enabled
# BIG    - Many features enabled, as rich as possible. (default)
# HUGE   - All possible featues enabled.
# Please select one of these alternatives above.
MODEL = BIG

# GUI or terminal mode executable.
# Comment out if you want just the character terminal mode only.
GUI = YES

# GUI with GTK
# If you have GTK installed you might want to enable this option.
# GTK = YES

# Comment out if you want the compiler version with :ver command.
# NOTE: This part can make some complications if you're using some
# predefined symbols/flags for your compiler. If does, just leave behind
# the comment varialbe CCVER.
CCVER = YES

# Uncomment if want a debug version. Resulting executable is DVIM.EXE
# Development purpose only! Normally, it should not be defined. !!!
# DEBUG = YES

# Languages support for Perl, Python, TCL etc.
# If you don't need it really, leave them behind the comment.
# You will need related libraries, include files etc.
# VIM_TCL    = YES
# VIM_PERL   = YES
# VIM_PYTHON = YES
# VIM_RUBY   = YES
# VIM_SNIFF  = YES

# X Input Method.  For entering special languages like chinese and
# Japanese. Please define just one: VIM_XIM or VIM_HANGULIN
# If you don't need it really, leave it behind the comment.
# VIM_XIM = YES

# Internal Hangul input method. GUI only.
# If you don't need it really, leave it behind the comment.
# VIM_HANGULIN = YES

# Allow any white space to separate the fields in a tags file
# When not defined, only a TAB is allowed.
# VIM_TAG_ANYWHITE = YES

######################################################################
# Directory, library and include files configuration section.
# Normally you need not to change anything below. !
# These may need to be defined if things are not in standard locations
#
# You can find some explanation in INSTALLVMS.TXT
######################################################################

# Compiler setup

.IFDEF VAX
.IFDEF DECC          # VAX with DECC
CC_DEF  = cc/decc
PREFIX  = /prefix=all
.ELSE                # VAX with VAXC
CC_DEF  = cc
PREFIX  =
CCVER   =
.ENDIF
.ELSE                # AXP wixh DECC
CC_DEF  = cc
PREFIX  = /prefix=all
.ENDIF

LD_DEF  = link
C_INC   = [.proto]

.IFDEF OLD_VMS
VMS_DEF = ,"OLD_VMS"
.ENDIF

.IFDEF DEBUG
DEBUG_DEF = ,"DEBUG"
TARGET    = dvim.exe
CFLAGS    = /debug/noopt$(PREFIX)
LDFLAGS   = /debug
.ELSE
TARGET    = vim.exe
CFLAGS    = /opt$(PREFIX)
LDFLAGS   =
.ENDIF

# Predefined VIM directories
# Please, use $VIM and $VIMRUNTIME logicals instead
VIMLOC  = ""
VIMRUN  = ""

CONFIG_H = os_vms_conf.h

.IFDEF GTK
.IFDEF GUI
.ELSE
GUI = YES
.ENDIF
.ENDIF

.IFDEF GUI
# X/Motif/GTK executable  (also works in terminal mode )

.IFDEF GTK
GTK_DIR  = ALPHA$DKA0:[GTK128.]
DEFS     = "HAVE_CONFIG_H","FEAT_GUI_GTK"
LIBS     = ,OS_VMS_GTK.OPT/OPT
GUI_FLAG = /name=(as_is,short)/float=ieee/ieee=denorm
GUI_SRC  = gui.c gui_gtk.c gui_gtk_f.c gui_gtk_x11.c pty.c
GUI_OBJ  = gui.obj gui_gtk.obj gui_gtk_f.obj gui_gtk_x11.obj pty.obj
GUI_INC  = ,"/gtk_root/gtk","/gtk_root/glib"
# GUI_INC_VER is used just for :ver information
# this string should escape from C and DCL in the same time
GUI_INC_VER= ,\""/gtk_root/gtk\"",\""/gtk_root/glib\""
.else
MOTIF	 = YES
DEFS     = "HAVE_CONFIG_H","FEAT_GUI_MOTIF"
LIBS     = ,OS_VMS_MOTIF.OPT/OPT
GUI_FLAG =
GUI_SRC  = gui.c gui_motif.c gui_x11.c
GUI_OBJ  = gui.obj gui_motif.obj gui_x11.obj
GUI_INC  =
.ENDIF

# You need to define these variables if you do not have DECW files
# at standard location
GUI_INC_DIR = ,decw$include:
# GUI_LIB_DIR = ,sys$library:

.ELSE
# Character terminal only executable
DEFS	 = "HAVE_CONFIG_H"
LIBS	 =
.ENDIF

.IFDEF VIM_PERL
# Perl related setup.
PERL	 = perl
PERL_DEF = ,"FEAT_PERL"
PERL_SRC = if_perlsfio.c if_perl.xs
PERL_OBJ = if_perlsfio.obj if_perl.obj
PERL_LIB = ,OS_VMS_PERL.OPT/OPT
PERL_INC = ,dka0:[perlbuild.perl.lib.vms_axp.5_6_1.core]
.ENDIF

.IFDEF VIM_PYTHON
# Python related setup.
PYTHON_DEF = ,"FEAT_PYTHON"
PYTHON_SRC = if_python.c
PYTHON_OBJ = if_python.obj
PYTHON_LIB = ,OS_VMS_PYTHON.OPT/OPT
PYTHON_INC = ,PYTHON_INCLUDE
.ENDIF

.IFDEF VIM_TCL
# TCL related setup.
TCL_DEF = ,"FEAT_TCL"
TCL_SRC = if_tcl.c
TCL_OBJ = if_tcl.obj
TCL_LIB = ,OS_VMS_TCL.OPT/OPT
TCL_INC = ,dka0:[tcl80.generic]
.ENDIF

.IFDEF VIM_SNIFF
# SNIFF related setup.
SNIFF_DEF = ,"FEAT_SNIFF"
SNIFF_SRC = if_sniff.c
SNIFF_OBJ = if_sniff.obj
SNIFF_LIB =
SNIFF_INC =
.ENDIF

.IFDEF VIM_RUBY
# RUBY related setup.
RUBY_DEF = ,"FEAT_RUBY"
RUBY_SRC = if_ruby.c
RUBY_OBJ = if_ruby.obj
RUBY_LIB = ,OS_VMS_RUBY.OPT/OPT
RUBY_INC =
.ENDIF

.IFDEF VIM_XIM
# XIM related setup.
.IFDEF GUI
XIM_DEF = ,"FEAT_XIM"
.ENDIF
.ENDIF

.IFDEF VIM_HANGULIN
# HANGULIN related setup.
.IFDEF GUI
HANGULIN_DEF = ,"FEAT_HANGULIN"
HANGULIN_SRC = hangulin.c
HANGULIN_OBJ = hangulin.obj
.ENDIF
.ENDIF

.IFDEF VIM_TAG_ANYWHITE
# TAG_ANYWHITE related setup.
TAG_DEF = ,"FEAT_TAG_ANYWHITE"
.ENDIF


######################################################################
# End of configuration section.
# Please, do not change anything below without programming experience.
######################################################################


MODEL_DEF = "FEAT_$(MODEL)",

# These go into pathdef.c
VIMUSER = "''F$EDIT(F$GETJPI(" ","USERNAME"),"TRIM")'"
VIMHOST = "''F$TRNLNM("SYS$NODE")'''F$TRNLNM("UCX$INET_HOST")'.''F$TRNLNM("UCX$INET_DOMAIN")'"

.SUFFIXES : .obj .c

ALL_CFLAGS = /def=($(MODEL_DEF)$(DEFS)$(VMS_DEF)$(DEBUG_DEF)$(PERL_DEF)$(PYTHON_DEF) -
 $(TCL_DEF)$(SNIFF_DEF)$(RUBY_DEF)$(XIM_DEF)$(HANGULIN_DEF)$(TAG_DEF)) -
 $(CFLAGS)$(GUI_FLAG) -
 /include=($(C_INC)$(GUI_INC_DIR)$(GUI_INC)$(PERL_INC)$(PYTHON_INC)$(TCL_INC))

# CFLAGS displayed in :ver information
# It is specially formated for correct display of unix like includes
# as $(GUI_INC) - replaced with $(GUI_INC_VER)
# Otherwise should not be any other difference.
ALL_CFLAGS_VER = /def=($(MODEL_DEF)$(DEFS)$(VMS_DEF)$(DEBUG_DEF)$(PERL_DEF)$(PYTHON_DEF) -
 $(TCL_DEF)$(SNIFF_DEF)$(RUBY_DEF)$(XIM_DEF)$(HANGULIN_DEF)$(TAG_DEF)) -
 $(CFLAGS)$(GUI_FLAG) -
 /include=($(C_INC)$(GUI_INC_DIR)$(GUI_INC_VER)$(PERL_INC)$(PYTHON_INC)$(TCL_INC))

ALL_LIBS = $(LIBS) $(GUI_LIB_DIR) $(GUI_LIB) \
	   $(PERL_LIB) $(PYTHON_LIB) $(TCL_LIB) $(SNIFF_LIB) $(RUBY_LIB)

SRC =	buffer.c charset.c diff.c digraph.c edit.c eval.c ex_cmds.c ex_cmds2.c \
	ex_docmd.c ex_getln.c if_xcmdsrv.c fileio.c fold.c getchar.c main.c \
	mark.c menu.c mbyte.c memfile.c memline.c message.c misc1.c misc2.c \
	move.c normal.c ops.c option.c quickfix.c regexp.c search.c syntax.c \
	tag.c term.c termlib.c ui.c undo.c version.c screen.c window.c \
	os_unix.c os_vms.c pathdef.c \
	$(GUI_SRC) $(PERL_SRC) $(PYTHON_SRC) $(TCL_SRC) $(SNIFF_SRC) \
	$(RUBY_SRC) $(HANGULIN_SRC)

OBJ =	buffer.obj charset.obj diff.obj digraph.obj edit.obj eval.obj \
	ex_cmds.obj ex_cmds2.obj ex_docmd.obj ex_getln.obj if_xcmdsrv.obj \
	fileio.obj fold.obj getchar.obj main.obj mark.obj menu.obj \
	memfile.obj memline.obj message.obj misc1.obj misc2.obj move.obj \
	mbyte.obj normal.obj ops.obj option.obj quickfix.obj regexp.obj \
	search.obj syntax.obj tag.obj term.obj termlib.obj ui.obj undo.obj \
	screen.obj version.obj window.obj os_unix.obj os_vms.obj pathdef.obj \
	$(GUI_OBJ) $(PERL_OBJ) $(PYTHON_OBJ) $(TCL_OBJ) $(SNIFF_OBJ) \
	$(RUBY_OBJ) $(HANGULIN_OBJ)

# Default target is making the executable
all : [.auto]config.h mmk_compat motif_env gtk_env perl_env python_env tcl_env ruby_env $(TARGET)
	! $@

[.auto]config.h : $(CONFIG_H)
	copy/nolog $(CONFIG_H) [.auto]config.h

mmk_compat :
        -@ open/write pd pathdef.c
        -@ write pd "/* Empty file to satisfy MMK depend.  */"
        -@ write pd "/* It will be owerwritten later on... */"
        -@ close pd
clean :
        -@ if "''F$SEARCH("*.exe")'" .NES. "" then delete/noconfirm/nolog *.exe;*
	-@ if "''F$SEARCH("*.obj")'" .NES. "" then delete/noconfirm/nolog *.obj;*
	-@ if "''F$SEARCH("[.auto]config.h")'" .NES. "" then delete/noconfirm/nolog [.auto]config.h;*
	-@ if "''F$SEARCH("pathdef.c")'" .NES. "" then delete/noconfirm/nolog pathdef.c;*
	-@ if "''F$SEARCH("if_perl.c")'" .NES. "" then delete/noconfirm/nolog if_perl.c;*
	-@ if "''F$SEARCH("*.opt")'" .NES. "" then delete/noconfirm/nolog *.opt;*

# Link the target
$(TARGET) : $(OBJ)
	$(LD_DEF) $(LDFLAGS) /exe=$(TARGET) $+ $(ALL_LIBS)

.c.obj :
	$(CC_DEF) $(ALL_CFLAGS) $<

pathdef.c : check_ccver $(CONFIG_H)
	-@ write sys$output "creating PATHDEF.C file."
	-@ open/write pd pathdef.c
	-@ write pd "/* pathdef.c -- DO NOT EDIT! */"
	-@ write pd "/* This file is automatically created by MAKE_VMS.MMS"
	-@ write pd " * Change the file MAKE_VMS.MMS Only. */"
	-@ write pd "typedef unsigned char   char_u;"
	-@ write pd "char_u *default_vim_dir = (char_u *)"$(VIMLOC)";"
	-@ write pd "char_u *default_vimruntime_dir = (char_u *)"$(VIMRUN)";"
	-@ write pd "char_u *all_cflags = (char_u *)""$(CC_DEF)$(ALL_CFLAGS_VER)"";"
	-@ write pd "char_u *all_lflags = (char_u *)""$(LD_DEF)$(LDFLAGS) /exe=$(TARGET) *.OBJ $(ALL_LIBS)"";"
	-@ write pd "char_u *compiler_version = (char_u *) ""''CC_VER'"";"
	-@ write pd "char_u *compiled_user = (char_u *) "$(VIMUSER)";"
	-@ write pd "char_u *compiled_sys = (char_u *) "$(VIMHOST)";"
	-@ close pd

if_perl.c : if_perl.xs
	-@ $(PERL) PERL_ROOT:[LIB.ExtUtils]xsubpp -prototypes -typemap -
 PERL_ROOT:[LIB.ExtUtils]typemap if_perl.xs >> $@

make_vms.mms :
	-@ write sys$output "The name of the makefile MUST be <MAKE_VMS.MMS> !!!"

.IFDEF CCVER
# This part can make some complications if you're using some predefined
# symbols/flags for your compiler. If does, just comment out CCVER variable
check_ccver :
	-@ define sys$output cc_ver.tmp
	-@ $(CC_DEF)/version
	-@ deassign sys$output
	-@ open/read file cc_ver.tmp
	-@ read file CC_VER
	-@ close file
	-@ delete/noconfirm/nolog cc_ver.tmp.*
.ELSE
check_ccver :
	-@ !
.ENDIF

.IFDEF MOTIF
motif_env :
        -@ write sys$output "using DECW/Motif environment."
	-@ write sys$output "creating OS_VMS_MOTIF.OPT file."
        -@ open/write opt_file OS_VMS_MOTIF.OPT
        -@ write opt_file "sys$share:decw$xmlibshr12.exe/share,-"
        -@ write opt_file "sys$share:decw$xtlibshrr5.exe/share,-"
        -@ write opt_file "sys$share:decw$xlibshr.exe/share"
	-@ close opt_file
.ELSE
motif_env :
        -@ !
.ENDIF


.IFDEF GTK
gtk_env :
        -@ write sys$output "using GTK environment:"
        -@ define/nolog gtk_root /trans=conc $(GTK_DIR)
        -@ show logical gtk_root
        -@ write sys$output "    include path: "$(GUI_INC)""
	-@ write sys$output "creating OS_VMS_GTK.OPT file."
        -@ open/write opt_file OS_VMS_GTK.OPT
	-@ write opt_file "gtk_root:[glib]libglib.exe /share,-"
        -@ write opt_file "gtk_root:[glib.gmodule]libgmodule.exe /share,-"
        -@ write opt_file "gtk_root:[gtk.gdk]libgdk.exe /share,-"
	-@ write opt_file "gtk_root:[gtk.gtk]libgtk.exe /share,-"
	-@ write opt_file "sys$share:decw$xmlibshr12.exe/share,-"
        -@ write opt_file "sys$share:decw$xtlibshrr5.exe/share,-"
        -@ write opt_file "sys$share:decw$xlibshr.exe/share"
        -@ close opt_file
.ELSE
gtk_env :
        -@ !
.ENDIF

.IFDEF VIM_PERL
perl_env :
        -@ write sys$output "using PERL environment:"
        -@ show logical PERLSHR
        -@ write sys$output "    include path: ""$(PERL_INC)"""
	-@ show symbol perl
        -@ open/write pd if_perl.c
        -@ write pd "/* Empty file to satisfy MMK depend.  */"
        -@ write pd "/* It will be owerwritten later on... */"
        -@ close pd
        -@ write sys$output "creating OS_VMS_PERL.OPT file."
        -@ open/write opt_file OS_VMS_PERL.OPT
        -@ write opt_file "PERLSHR /share"
        -@ close opt_file
.ELSE
perl_env :
        -@ !
.ENDIF

.IFDEF VIM_PYTHON
python_env :
        -@ write sys$output "using PYTHON environment:"
        -@ show logical PYTHON_INCLUDE
        -@ show logical PYTHON_OLB
        -@ write sys$output "creating OS_VMS_PYTHON.OPT file."
        -@ open/write opt_file OS_VMS_PYTHON.OPT
        -@ write opt_file "PYTHON_OLB:PYTHON.OLB /share"
        -@ close opt_file
.ELSE
python_env :
        -@ !
.ENDIF

.IFDEF VIM_TCL
tcl_env :
        -@ write sys$output "using TCL environment:"
        -@ show logical TCLSHR
        -@ write sys$output "    include path: ""$(TCL_INC)"""
        -@ write sys$output "creating OS_VMS_TCL.OPT file."
        -@ open/write opt_file OS_VMS_TCL.OPT
        -@ write opt_file "TCLSHR /share"
        -@ close opt_file
.ELSE
tcl_env :
        -@ !
.ENDIF

.IFDEF VIM_RUBY
ruby_env :
        -@ write sys$output "using RUBY environment:"
        -@ write sys$output "    include path: ""$(RUBY_INC)"""
        -@ write sys$output "creating OS_VMS_RUBY.OPT file."
        -@ open/write opt_file OS_VMS_RUBY.OPT
        -@ write opt_file "RUBYSHR /share"
        -@ close opt_file
.ELSE
ruby_env :
        -@ !
.ENDIF


