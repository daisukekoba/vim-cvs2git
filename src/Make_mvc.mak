# Makefile for Vim on Win32 (Windows NT and Windows 95), using the
# Microsoft Visual C++ 2.x and MSVC 4.x compilers (or newer).
# It builds on Windows 95 and all four NT platforms: i386, Alpha, MIPS, and
# PowerPC.  The NT/i386 binary and the Windows 95 binary are identical.
#
# This makefile can build the console, GUI, OLE-enable, Perl-enabled and
# Python-enabled versions of vim for Win32 platforms.
#
# When compiling different versions, do "nmake clean" first!
#
# The basic command line to build vim is:
#	nmake -f Make_mvc.mak
# This will build the console version of vim with no additional interfaces.
# To add interfaces, define any of the following:
#	GUI interface: GUI=yes
#	OLE interface: OLE=yes
#	IME support: IME=yes	(requires GUI=yes)
#	  DYNAMIC_IME=[yes or no]  (to load the imm32.dll dynamically, default
#	  is yes)
#	Global IME support: GIME=yes (requires GUI=yes)
#	Perl interface:
#	  PERL=[Path to Perl directory]
#	  DYNAMIC_PERL=yes (to load the Perl DLL dynamically)
#	  PERL_VER=[Perl version, in the form 55 (5.005), 56 (5.6.x), etc]
#	Python interface:
#	  PYTHON=[Path to Python directory]
#	  DYNAMIC_PYTHON=yes (to load the Python DLL dynamically)
#	  PYTHON_VER=[Python version, eg 15, 20]  (default is 15)
#	Ruby interface:
#	  RUBY=[Path to Ruby directory]
#	  DYNAMIC_RUBY=yes (to load the Ruby DLL dynamically)
#	  RUBY_VER=[Ruby version, eg 16, 17] (default is 16)
#	  RUBY_VER_LONG=[Ruby version, eg 1.6, 1.7] (default is 1.6)
#	    You must set RUBY_VER_LONG when change RUBY_VER.
#	Tcl interface:
#	  TCL=[Path to Tcl directory]
#	  DYNAMIC_TCL=yes (to load the Tcl DLL dynamically)
#	  TCL_VER=[Tcl version, e.g. 80, 83]  (default is 83)
#	  TCL_VER_LONG=[Tcl version, eg 8.3] (default is 8.3)
#	    You must set TCL_VER_LONG when change TCL_VER.
#	Debug version: DEBUG=1
#	SNiFF+ interface: SNIFF=yes
#	Iconv library support (always dynamically loaded):
#	  ICONV=[yes or no]  (default is yes)
#	Intl library support (always dynamically loaded):
#	  INTL=[yes or no]  (default is yes)
#	See http://sourceforge.net/projects/gettext/
#
# You can combine any of these interfaces
#
# Example: To build the non-debug, GUI version with Perl interface:
#	nmake -f Make_mvc.mak GUI=yes PERL=C:\Perl
#
# To build using Borland C++, use Make_bc3.mak or Make_bc5.mak.
#
# DEBUG with Make_mvc.mak and Make_dvc.mak:
#	This makefile gives a fineness of control which is not supported in
#	Visual C++ configuration files.  Therefore, debugging requires a bit of
#	extra work.
#	Make_dvc.mak is a Visual C++ project to access that support.
#	To use Make_dvc.mak:
#	1) Build Vim with Make_mvc.mak.
#	     Use a "DEBUG=1" argument to build Vim with debug support.
#	     E.g. the following builds gvimd.exe:
#		nmake -f Make_mvc.mak debug=1 gui=yes
#	2) Use MS Devstudio and set it up to allow that file to be debugged:
#	    i) Pass Make_dvc.mak to the IDE.
#	         Use the "open workspace" menu entry to load Make_dvc.mak.
#	         Alternatively, from the command line:
#			msdev /nologo Make_dvc.mak
#		Note: Make_dvc.mak is in VC4.0 format. Later VC versions see
#		this and offer to convert it to their own format. Accept that.
#		It creates a file called Make_dvc.dsw which can then be used
#		for further operations.  E.g.
#		    msdev /nologo Make_dvc.dsw
#	    ii) Set the built executable for debugging:
#		a) Alt+F7/Debug takes you to the Debug dialog.
#		b) Fill "Executable for debug session". e.g. gvimd.exe
#		c) Fill "Program arguments". e.g. -R dosinst.c
#		d) Complete the dialog
#	3) You can now debug the executable you built with Make_mvc.mak
#
#	Note: Make_dvc.mak builds vimrun.exe, because it must build something
#	to be a valid makefile..

### See feature.h for a list of optionals.
# If you want to build some optional features without modifying the source,
# you can set DEFINES on the command line, e.g.,
#	nmake -f makefile.mvc "DEFINES=-DEMACS_TAGS"

# Build on both Windows NT and Windows 95

TARGETOS = BOTH

# Select one of eight object code directories, depends on GUI, OLE and DEBUG.
# If you change something else, do "make clean" first!
!ifdef GUI
OBJDIR = .\ObjG
!else
OBJDIR = .\ObjC
!endif
!ifdef OLE
OBJDIR = $(OBJDIR)O
!endif
!ifdef DEBUG
OBJDIR = $(OBJDIR)d
!endif

# ntwin32.mak requires that CPU be set appropriately

!ifdef PROCESSOR_ARCHITECTURE
# We're on Windows NT or using VC 6
CPU = $(PROCESSOR_ARCHITECTURE)
! if "$(CPU)" == "x86"
CPU = i386
! endif
!else  # !PROCESSOR_ARCHITECTURE
# We're on Windows 95
CPU = i386
!endif # !PROCESSOR_ARCHITECTURE


# Build a retail version by default

!ifndef DEBUG
NODEBUG = 1
!endif


# Build a multithreaded version for the Windows 95 dead keys hack
# Commented out because it doesn't work.
# MULTITHREADED = 1


# Get all sorts of useful, standard macros from the SDK.  (Note that
# MSVC 2.2 does not install <ntwin32.mak> in the \msvc20\include
# directory, but you can find it in \msvc20\include on the CD-ROM.
# You may also need <win32.mak> from the same place.)

!include <ntwin32.mak>


#>>>>> path of the compiler and linker; name of include and lib directories
# PATH = c:\msvc20\bin;$(PATH)
# INCLUDE = c:\msvc20\include
# LIB = c:\msvc20\lib

!ifndef CTAGS
CTAGS = ctags
!endif

!if "$(SNIFF)" == "yes"
# SNIFF - Include support for SNiFF+.
SNIFF_INCL  = if_sniff.h
SNIFF_OBJ   = $(OBJDIR)/if_sniff.obj
SNIFF_LIB    = shell32.lib
SNIFF_DEFS  = -DFEAT_SNIFF
# The SNiFF integration needs multithreaded libraries!
MULTITHREADED = yes
!endif

!if defined(USE_MSVCRT)
CVARS = $(cvarsdll)
!elseif defined(MULTITHREADED)
CVARS = $(cvarsmt)
!else
CVARS = $(cvars)
!endif

# need advapi32.lib for GetUserName()
# need shell32.lib for ExtractIcon()
# gdi32.lib and comdlg32.lib for printing support
# ole32.lib and uuid.lib are needed for FEAT_SHORTCUT
CON_LIB = advapi32.lib shell32.lib gdi32.lib comdlg32.lib ole32.lib uuid.lib

# If you have a fixed directory for $VIM or $VIMRUNTIME, other than the normal
# default, use these lines.
#VIMRCLOC = somewhere
#VIMRUNTIMEDIR = somewhere

CFLAGS = -c /W3 /nologo $(CVARS) -I. -Iproto -DHAVE_PATHDEF -DWIN32 \
		$(SNIFF_DEFS) $(DEFINES)

#>>>>> end of choices
###########################################################################

!ifdef OS
OS_TYPE	= winnt
DEL_TREE = rmdir /s /q
!else
OS_TYPE	= win95
DEL_TREE = deltree /y
!endif

INTDIR=$(OBJDIR)
OUTDIR=$(OBJDIR)

!ifdef NODEBUG
VIM = vim
CFLAGS = $(CFLAGS) -DNDEBUG /Ox /Zi
RCFLAGS = $(rcflags) $(rcvars) -DNDEBUG
PDB = /Fd$(OUTDIR)/
LINK_PDB = /PDB:$(OUTDIR)/
! ifndef USE_MSVCRT
LIBC = libc.lib
! else
CFLAGS = $(CFLAGS) -MD
LIBC = msvcrt.lib
! endif
!else  # DEBUG
VIM = vimd
# MSVC 4.1
PDB = /Fd$(OUTDIR)/
LINK_PDB = /PDB:$(OUTDIR)/
# MSVC 2.2
# PDB = /Fd$(OUTDIR)/vim.pdb
# LINK_PDB = /PDB:$(OUTDIR)/vim.pdb
CFLAGS = $(CFLAGS) -D_DEBUG -DDEBUG /Zi /Od
RCFLAGS = $(rcflags) $(rcvars) -D_DEBUG -DDEBUG
# The /fixed:no is needed for Quantify.
! ifndef USE_MSVCRT
LIBC = /fixed:no libcd.lib
! else
CFLAGS = $(CFLAGS) -MDd
LIBC = /fixed:no msvcrtd.lib
! endif
!endif # DEBUG

INCL =	vim.h os_win32.h ascii.h feature.h globals.h keymap.h macros.h \
	proto.h option.h structs.h term.h $(SNIFF_INCL)

OBJ = \
	$(OUTDIR)\buffer.obj \
	$(OUTDIR)\charset.obj \
	$(OUTDIR)\diff.obj \
	$(OUTDIR)\digraph.obj \
	$(OUTDIR)\edit.obj \
	$(OUTDIR)\eval.obj \
	$(OUTDIR)\ex_cmds.obj \
	$(OUTDIR)\ex_cmds2.obj \
	$(OUTDIR)\ex_docmd.obj \
	$(OUTDIR)\ex_getln.obj \
	$(OUTDIR)\fileio.obj \
	$(OUTDIR)\fold.obj \
	$(OUTDIR)\getchar.obj \
	$(OUTDIR)\main.obj \
	$(OUTDIR)\mark.obj \
	$(OUTDIR)\mbyte.obj \
	$(OUTDIR)\memfile.obj \
	$(OUTDIR)\memline.obj \
	$(OUTDIR)\menu.obj \
	$(OUTDIR)\message.obj \
	$(OUTDIR)\misc1.obj \
	$(OUTDIR)\misc2.obj \
	$(OUTDIR)\move.obj \
	$(OUTDIR)\normal.obj \
	$(OUTDIR)\ops.obj \
	$(OUTDIR)\option.obj \
	$(OUTDIR)\os_mswin.obj \
	$(OUTDIR)\os_win32.obj \
	$(OUTDIR)\pathdef.obj \
	$(OUTDIR)\quickfix.obj \
	$(OUTDIR)\regexp.obj \
	$(OUTDIR)\screen.obj \
	$(OUTDIR)\search.obj \
	$(OUTDIR)\syntax.obj \
	$(OUTDIR)\tag.obj \
	$(OUTDIR)\term.obj \
	$(OUTDIR)\ui.obj \
	$(OUTDIR)\undo.obj \
	$(OUTDIR)\window.obj \
	$(OUTDIR)\vim.res

!if "$(OLE)" == "yes"
# Force GUI version to be built
GUI = yes
CFLAGS = $(CFLAGS) -DFEAT_OLE
RCFLAGS = $(RCFLAGS) -DFEAT_OLE
OLE_OBJ = $(OUTDIR)\if_ole.obj
OLE_IDL = if_ole.idl
OLE_LIB = oleaut32.lib
!endif

!if "$(IME)" == "yes"
CFLAGS = $(CFLAGS) -DFEAT_MBYTE_IME
!ifndef DYNAMIC_IME
DYNAMIC_IME = yes
!endif
!if "$(DYNAMIC_IME)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_IME
!else
IME_LIB = imm32.lib
!endif
!endif

!if "$(GIME)" == "yes"
CFLAGS = $(CFLAGS) -DFEAT_MBYTE -DGLOBAL_IME
OBJ = $(OBJ) $(OUTDIR)\dimm_i.obj $(OUTDIR)\glbl_ime.obj
!endif

!if "$(GUI)" == "yes"
SUBSYSTEM = windows
CFLAGS = $(CFLAGS) -DFEAT_GUI_W32
RCFLAGS = $(RCFLAGS) -DFEAT_GUI_W32
VIM = g$(VIM)
GUI_INCL = \
	gui.h \
	regexp.h \
	ascii.h \
	ex_cmds.h \
	farsi.h \
	feature.h \
	globals.h \
	keymap.h \
	macros.h \
	option.h \
	os_dos.h \
	os_win32.h
GUI_OBJ = \
	$(OUTDIR)\gui.obj \
	$(OUTDIR)\gui_w32.obj \
	$(OUTDIR)\os_w32exe.obj
GUI_LIB = \
	oldnames.lib kernel32.lib gdi32.lib $(IME_LIB) \
	winspool.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib \
	/machine:$(CPU) /nodefaultlib
!else
SUBSYSTEM = console
!endif

# iconv.dll library (dynamically loaded)
!ifndef ICONV
ICONV = yes
!endif
!if "$(ICONV)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_ICONV
!endif

# libintl.dll library
!ifndef INTL
INTL = yes
!endif
!if "$(INTL)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_GETTEXT
!endif

# TCL interface
!ifdef TCL
!ifndef TCL_VER
TCL_VER = 83
TCL_VER_LONG = 8.3
!endif
!message Tcl detected (version $(TCL_VER)) - root dir is "$(TCL)"
!if "$(DYNAMIC_TCL)" == "yes"
!message Tcl DLL will be loaded dynamically
TCL_DLL = tcl$(TCL_VER).dll
CFLAGS  = $(CFLAGS) -DFEAT_TCL -DDYNAMIC_TCL -DDYNAMIC_TCL_DLL=\"$(TCL_DLL)\" -DDYNAMIC_TCL_VER=\"$(TCL_VER_LONG)\"
TCL_OBJ	= $(OUTDIR)\if_tcl.obj
TCL_INC	= /I "$(TCL)\Include" /I "$(TCL)"
TCL_LIB = $(TCL)\lib\tclstub$(TCL_VER).lib
!else
CFLAGS  = $(CFLAGS) -DFEAT_TCL
TCL_OBJ	= $(OUTDIR)\if_tcl.obj
TCL_INC	= /I "$(TCL)\Include" /I "$(TCL)"
TCL_LIB = $(TCL)\lib\tcl$(TCL_VER)vc.lib
!endif
!endif

# PYTHON interface
!ifdef PYTHON
!ifndef PYTHON_VER
PYTHON_VER = 15
!endif
!message Python detected (version $(PYTHON_VER)) - root dir is "$(PYTHON)"
!if "$(DYNAMIC_PYTHON)" == "yes"
!message Python DLL will be loaded dynamically
!endif
CFLAGS = $(CFLAGS) -DFEAT_PYTHON
PYTHON_OBJ = $(OUTDIR)\if_python.obj
PYTHON_INC = /I "$(PYTHON)\Include" /I "$(PYTHON)\PC"
!if "$(DYNAMIC_PYTHON)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PYTHON -DDYNAMIC_PYTHON_DLL=\"python$(PYTHON_VER).dll\"
PYTHON_LIB = /nodefaultlib:python$(PYTHON_VER).lib
!else
PYTHON_LIB = $(PYTHON)\libs\python$(PYTHON_VER).lib
!endif
!endif

# Perl interface
!ifdef PERL
!message Perl detected (version $(PERL_VER)) - root dir is "$(PERL)"
!if "$(DYNAMIC_PERL)" == "yes"
!if "$(PERL_VER)" == "56"
!message Perl DLL will be loaded dynamically
!else
!message Dynamic loading is not supported for Perl versions earlier than 5.6.0
!message Reverting to static loading...
!undef DYNAMIC_PERL
!endif
!endif

# Is Perl installed in architecture-specific directories?
!if exist($(PERL)\Bin\MSWin32-x86)
PERL_ARCH = \MSWin32-x86
!endif

PERL_INCDIR = $(PERL)\Lib$(PERL_ARCH)\Core

# Version-dependent stuff
!if "$(PERL_VER)" == "55"
PERL_LIB = $(PERL_INCDIR)\perl.lib
!else
PERL_DLL = perl$(PERL_VER).dll
PERL_LIB = $(PERL_INCDIR)\perl$(PERL_VER).lib
!endif

CFLAGS = $(CFLAGS) -DFEAT_PERL

# Do we want to load Perl dynamically?
!if "$(DYNAMIC_PERL)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PERL -DDYNAMIC_PERL_DLL=\"$(PERL_DLL)\"
!undef PERL_LIB
!endif

PERL_EXE = $(PERL)\Bin$(PERL_ARCH)\perl
PERL_INC = /I $(PERL_INCDIR)
PERL_OBJ = $(OUTDIR)\if_perl.obj $(OUTDIR)\if_perlsfio.obj
XSUBPP = $(PERL)\lib\ExtUtils\xsubpp
XSUBPP_TYPEMAP = $(PERL)\lib\ExtUtils\typemap

!endif

#
# Support Ruby interface
#
!ifdef RUBY
#  Set default value
!ifndef RUBY_VER
RUBY_VER = 16
!endif
!ifndef RUBY_VER_LONG
RUBY_VER_LONG = 1.6
!endif
!ifndef RUBY_PLATFORM
RUBY_PLATFORM = i586-mswin32
!endif
RUBY_INSTALL_NAME = mswin32-ruby$(RUBY_VER)

!message Ruby detected (version $(RUBY_VER)) - root dir is "$(RUBY)"
CFLAGS = $(CFLAGS) -DFEAT_RUBY
RUBY_OBJ = $(OUTDIR)\if_ruby.obj
RUBY_INC = /I "$(RUBY)\lib\ruby\$(RUBY_VER_LONG)\$(RUBY_PLATFORM)"
RUBY_LIB = $(RUBY)\lib\$(RUBY_INSTALL_NAME).lib
# Do we want to load Ruby dynamically?
!if "$(DYNAMIC_RUBY)" == "yes"
!message Ruby DLL will be loaded dynamically
CFLAGS = $(CFLAGS) -DDYNAMIC_RUBY -DDYNAMIC_RUBY_DLL=\"$(RUBY_INSTALL_NAME).dll\"
!undef RUBY_LIB
!endif
!endif # RUBY

#
# End extra featuare include
#
!message

conflags = /nologo /subsystem:$(SUBSYSTEM) /incremental:no

LINKARGS1 = /debug $(linkdebug) $(conflags) /nodefaultlib:libc
LINKARGS2 = $(CON_LIB) $(GUI_LIB) $(LIBC) $(OLE_LIB)  user32.lib $(SNIFF_LIB) \
		$(PERL_LIB) $(PYTHON_LIB) $(RUBY_LIB) $(TCL_LIB) $(LINK_PDB)

all:	$(VIM) vimrun.exe install.exe uninstal.exe xxd/xxd.exe

$(VIM): $(OUTDIR) $(OBJ) $(GUI_OBJ) $(OLE_OBJ) $(OLE_IDL) $(PERL_OBJ) $(PYTHON_OBJ) $(RUBY_OBJ) $(TCL_OBJ) $(SNIFF_OBJ) $(OUTDIR)\version.obj
	$(link) $(LINKARGS1) -out:$*.exe $(OBJ) $(GUI_OBJ) $(OLE_OBJ) \
		$(PERL_OBJ) $(PYTHON_OBJ) $(RUBY_OBJ) $(TCL_OBJ) $(SNIFF_OBJ) \
		$(OUTDIR)\version.obj $(LINKARGS2)
	if exist $(OUTDIR)\version.obj del $(OUTDIR)\version.obj
	if exist auto\pathdef.c del auto\pathdef.c

$(VIM).exe: $(VIM)

$(OUTDIR):
	if not exist $(OUTDIR)/nul    mkdir $(OUTDIR)

install.exe: dosinst.c
	$(CC) /nologo -DNDEBUG -DWIN32 dosinst.c kernel32.lib shell32.lib ole32.lib advapi32.lib uuid.lib
	- if exist install.exe del install.exe
	ren dosinst.exe install.exe

uninstal.exe: uninstal.c
	$(CC) /nologo -DNDEBUG -DWIN32 uninstal.c shell32.lib advapi32.lib

vimrun.exe: vimrun.c
	$(CC) /nologo -DNDEBUG vimrun.c

xxd/xxd.exe: xxd/xxd.c
	cd xxd
	$(MAKE) /NOLOGO -f Make_mvc.mak
	cd ..


tags: notags
	$(CTAGS) *.c *.h if_perl.xs proto\*.pro

notags:
	- if exist tags del tags

clean:
	- $(DEL_TREE) $(OUTDIR) auto
	- if exist *.obj del *.obj
	- if exist $(VIM).exe del $(VIM).exe
	- if exist $(VIM).ilk del $(VIM).ilk
	- if exist $(VIM).pdb del $(VIM).pdb
	- if exist $(VIM).map del $(VIM).map
	- if exist $(VIM).ncb del $(VIM).ncb
	- if exist vimrun.exe del vimrun.exe
	- if exist install.exe del install.exe
	- if exist uninstal.exe del uninstal.exe
	- if exist if_perl.c del if_perl.c
	- if exist dimm.h del dimm.h
	- if exist dimm_i.c del dimm_i.c
	- if exist dimm.tlb del dimm.tlb
	- if exist dosinst.exe del dosinst.exe
	cd xxd
	$(MAKE) /NOLOGO -f Make_mvc.mak clean
	cd ..
	- if exist testdir\*.out del testdir\*.out

test:
	cd testdir
	$(MAKE) /NOLOGO -f Make_dos.mak
	cd ..

###########################################################################

# Create a default rule for transforming .c files to .obj files in $(OUTDIR)
# Batch compilation is supported by nmake 1.62 (part of VS 5.0) and later)
!IF "$(_NMAKE_VER)" == ""
.c{$(OUTDIR)/}.obj:
!ELSE
.c{$(OUTDIR)/}.obj::
!ENDIF
	$(CC) $(CFLAGS) /Fo$(OUTDIR)/ $(PDB) $<

# Create a default rule for transforming .cpp files to .obj files in $(OUTDIR)
# Batch compilation is supported by nmake 1.62 (part of VS 5.0) and later)
!IF "$(_NMAKE_VER)" == ""
.cpp{$(OUTDIR)/}.obj:
!ELSE
.cpp{$(OUTDIR)/}.obj::
!ENDIF
	$(CC) $(CFLAGS) /Fo$(OUTDIR)/ $(PDB) $<

$(OUTDIR)/buffer.obj:	$(OUTDIR) buffer.c  $(INCL)

$(OUTDIR)/charset.obj:	$(OUTDIR) charset.c  $(INCL)

$(OUTDIR)/diff.obj:	$(OUTDIR) diff.c  $(INCL)

$(OUTDIR)/digraph.obj:	$(OUTDIR) digraph.c  $(INCL)

$(OUTDIR)/edit.obj:	$(OUTDIR) edit.c  $(INCL)

$(OUTDIR)/eval.obj:	$(OUTDIR) eval.c  $(INCL)

$(OUTDIR)/ex_cmds.obj:	$(OUTDIR) ex_cmds.c  $(INCL)

$(OUTDIR)/ex_cmds2.obj:	$(OUTDIR) ex_cmds2.c  $(INCL)

$(OUTDIR)/ex_docmd.obj:	$(OUTDIR) ex_docmd.c  $(INCL) ex_cmds.h

$(OUTDIR)/ex_getln.obj:	$(OUTDIR) ex_getln.c  $(INCL)

$(OUTDIR)/fileio.obj:	$(OUTDIR) fileio.c  $(INCL)

$(OUTDIR)/fold.obj:	$(OUTDIR) fold.c  $(INCL)

$(OUTDIR)/getchar.obj:	$(OUTDIR) getchar.c  $(INCL)

$(OUTDIR)/gui.obj:	$(OUTDIR) gui.c  $(INCL) $(GUI_INCL)

$(OUTDIR)/gui_w32.obj:	$(OUTDIR) gui_w32.c gui_w48.c $(INCL) $(GUI_INCL)

if_perl.c : if_perl.xs typemap
	$(PERL_EXE) $(XSUBPP) -prototypes -typemap $(XSUBPP_TYPEMAP) -typemap typemap if_perl.xs > if_perl.c

$(OUTDIR)/if_perl.obj: $(OUTDIR) if_perl.c  $(INCL)
	$(CC) $(CFLAGS) $(PERL_INC) if_perl.c /Fo$(OUTDIR)/if_perl.obj $(PDB)

$(OUTDIR)/if_perlsfio.obj: $(OUTDIR) if_perlsfio.c  $(INCL)
	$(CC) $(CFLAGS) $(PERL_INC) if_perlsfio.c /Fo$(OUTDIR)/if_perlsfio.obj $(PDB)

$(OUTDIR)/if_python.obj: $(OUTDIR) if_python.c  $(INCL)
	$(CC) $(CFLAGS) $(PYTHON_INC) if_python.c /Fo$(OUTDIR)/if_python.obj $(PDB)

$(OUTDIR)/if_ole.obj: $(OUTDIR) if_ole.cpp  $(INCL) if_ole.h

$(OUTDIR)/if_ruby.obj: $(OUTDIR) if_ruby.c  $(INCL)
	$(CC) $(CFLAGS) $(RUBY_INC) if_ruby.c /Fo$(OUTDIR)/if_ruby.obj $(PDB)

$(OUTDIR)/if_sniff.obj:	$(OUTDIR) if_sniff.c  $(INCL)
	$(CC) $(CFLAGS) if_sniff.c /Fo$(OUTDIR)/if_sniff.obj $(PDB)

$(OUTDIR)/if_tcl.obj: $(OUTDIR) if_tcl.c  $(INCL)
	$(CC) $(CFLAGS) $(TCL_INC) if_tcl.c /Fo$(OUTDIR)/if_tcl.obj $(PDB)

$(OUTDIR)/main.obj:	$(OUTDIR) main.c  $(INCL)

$(OUTDIR)/mark.obj:	$(OUTDIR) mark.c  $(INCL)

$(OUTDIR)/memfile.obj:	$(OUTDIR) memfile.c  $(INCL)

$(OUTDIR)/memline.obj:	$(OUTDIR) memline.c  $(INCL)

$(OUTDIR)/menu.obj:	$(OUTDIR) menu.c  $(INCL)

$(OUTDIR)/message.obj:	$(OUTDIR) message.c  $(INCL)

$(OUTDIR)/misc1.obj:	$(OUTDIR) misc1.c  $(INCL)

$(OUTDIR)/misc2.obj:	$(OUTDIR) misc2.c  $(INCL)

$(OUTDIR)/move.obj:	$(OUTDIR) move.c  $(INCL)

$(OUTDIR)/mbyte.obj: $(OUTDIR) mbyte.c  $(INCL)

$(OUTDIR)/normal.obj:	$(OUTDIR) normal.c  $(INCL)

$(OUTDIR)/option.obj:	$(OUTDIR) option.c  $(INCL)

$(OUTDIR)/ops.obj:	$(OUTDIR) ops.c  $(INCL)

$(OUTDIR)/os_mswin.obj:	$(OUTDIR) os_mswin.c  $(INCL)

$(OUTDIR)/os_win32.obj:	$(OUTDIR) os_win32.c  $(INCL) os_win32.h

$(OUTDIR)/os_w32exe.obj:	$(OUTDIR) os_w32exe.c  $(INCL)

$(OUTDIR)/pathdef.obj:	$(OUTDIR) auto/pathdef.c  $(INCL)
	$(CC) $(CFLAGS) auto/pathdef.c /Fo$(OUTDIR)/pathdef.obj $(PDB)

$(OUTDIR)/quickfix.obj:	$(OUTDIR) quickfix.c  $(INCL)

$(OUTDIR)/regexp.obj:	$(OUTDIR) regexp.c  $(INCL)

$(OUTDIR)/screen.obj:	$(OUTDIR) screen.c  $(INCL)

$(OUTDIR)/search.obj:	$(OUTDIR) search.c  $(INCL)

$(OUTDIR)/syntax.obj:	$(OUTDIR) syntax.c  $(INCL)

$(OUTDIR)/tag.obj:	$(OUTDIR) tag.c  $(INCL)

$(OUTDIR)/term.obj:	$(OUTDIR) term.c  $(INCL)

$(OUTDIR)/ui.obj:	$(OUTDIR) ui.c  $(INCL)

$(OUTDIR)/undo.obj:	$(OUTDIR) undo.c  $(INCL)

$(OUTDIR)/version.obj: $(OUTDIR) version.c version.h

$(OUTDIR)/window.obj:	$(OUTDIR) window.c  $(INCL)

$(OUTDIR)/vim.res:	$(OUTDIR) vim.rc version.h tools.bmp tearoff.bmp vim.ico vim_error.ico vim_alert.ico vim_info.ico vim_quest.ico
	$(RC) /l 0x409 /Fo$(OUTDIR)/vim.res $(RCFLAGS) vim.rc

iid_ole.c if_ole.h vim.tlb: if_ole.idl $(INTDIR) $(OUTDIR)
	midl /nologo /proxy nul /iid iid_ole.c /tlb vim.tlb /header if_ole.h if_ole.idl

dimm.h dimm_i.c: dimm.idl
	midl /nologo /proxy nul dimm.idl

$(OUTDIR)/dimm_i.obj: $(OUTDIR) dimm_i.c $(INCL)

$(OUTDIR)/glbl_ime.obj:	$(OUTDIR) glbl_ime.cpp  dimm.h $(INCL)

auto/pathdef.c: auto
	@echo creating auto/pathdef.c
	@echo /* pathdef.c */ > auto\pathdef.c
	@echo #include "vim.h" >> auto\pathdef.c
	@echo char_u *default_vim_dir = (char_u *)"$(VIMRCLOC:\=\\)"; >> auto\pathdef.c
	@echo char_u *default_vimruntime_dir = (char_u *)"$(VIMRUNTIMEDIR:\=\\)"; >> auto\pathdef.c
	@echo char_u *all_cflags = (char_u *)"$(CC:\=\\) $(CFLAGS)"; >> auto\pathdef.c
	@echo char_u *all_lflags = (char_u *)"$(link:\=\\) $(LINKARGS1:\=\\) $(LINKARGS2:\=\\)"; >> auto\pathdef.c
	@echo char_u *compiled_user = (char_u *)"$(USERNAME)"; >> auto\pathdef.c
	@echo char_u *compiled_sys = (char_u *)"$(USERDOMAIN)"; >> auto\pathdef.c

auto:
	if not exist auto/nul mkdir auto

# End Custom Build
proto.h: \
	proto/buffer.pro \
	proto/charset.pro \
	proto/diff.pro \
	proto/digraph.pro \
	proto/edit.pro \
	proto/eval.pro \
	proto/ex_cmds.pro \
	proto/ex_cmds2.pro \
	proto/ex_docmd.pro \
	proto/ex_getln.pro \
	proto/fileio.pro \
	proto/getchar.pro \
	proto/main.pro \
	proto/mark.pro \
	proto/memfile.pro \
	proto/memline.pro \
	proto/menu.pro \
	proto/message.pro \
	proto/misc1.pro \
	proto/misc2.pro \
	proto/move.pro \
	proto/mbyte.pro \
	proto/normal.pro \
	proto/ops.pro \
	proto/option.pro \
	proto/os_mswin.pro \
	proto/os_win32.pro \
	proto/quickfix.pro \
	proto/regexp.pro \
	proto/screen.pro \
	proto/search.pro \
	proto/syntax.pro \
	proto/tag.pro \
	proto/term.pro \
	proto/ui.pro \
	proto/undo.pro \
	proto/window.pro

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=0: