/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#ifndef VIM__H
# define VIM__H

/* use fastcall for Borland, when compiling for Win32 (not for DOS16) */
#if defined(__BORLANDC__) && defined(WIN32) && !defined(DEBUG)
#if defined(FEAT_PERL) || \
    defined(FEAT_PYTHON) || \
    defined(FEAT_RUBY) || \
    defined(FEAT_TCL) || \
    defined(DYNAMIC_GETTEXT) || \
    defined(DYNAMIC_ICONV) || \
    defined(DYNAMIC_IME) || \
    defined(XPM)
#  pragma option -pc
# else
#  pragma option -pr
# endif
#endif

/* ============ the header file puzzle (ca. 50-100 pieces) ========= */

#ifdef HAVE_CONFIG_H	/* GNU autoconf (or something else) was here */
# include "auto/config.h"
# define HAVE_PATHDEF

/*
 * Check if configure correcly managed to find sizeof(int).  If this failed,
 * it becomes zero.  This is likely a problem of not being able to run the
 * test program.  Other items from configure may also be wrong then!
 */
# if (SIZEOF_INT == 0)
    Error: configure did not run properly.  Check auto/config.log.
# endif

/*
 * Cygwin may have fchdir() in a newer rleease, but in most versions it
 * doesn't work well and avoiding it keeps the binary backward compatible.
 */
# if defined(__CYGWIN32__) && defined(HAVE_FCHDIR)
#  undef HAVE_FCHDIR
# endif
#endif

#ifdef __EMX__		/* hand-edited config.h for OS/2 with EMX */
# include "os_os2_cfg.h"
#endif

/*
 * MACOS_CLASSIC compiling for MacOS prior to MacOS X
 * MACOS_X_UNIX  compiling for MacOS X (using os_unix.c)
 * MACOS_X       compiling for MacOS X (using os_unix.c or os_mac.c)
 * MACOS	 compiling for either one
 */
#if defined(macintosh) && !defined(MACOS_CLASSIC)
#  define MACOS_CLASSIC
#endif
#if defined(MACOS_X_UNIX)
# define MACOS_X
# ifndef HAVE_CONFIG_H
#  define UNIX
# endif
#endif
#if defined(MACOS_X) || defined(MACOS_CLASSIC)
#  define MACOS
#endif
#if defined(MACOS_X) && defined(MACOS_CLASSIC)
    Error: To compile for both MACOS X and Classic use a Classic Carbon
#endif


#if defined(FEAT_GUI_MOTIF) \
    || defined(FEAT_GUI_GTK) \
    || defined(FEAT_GUI_ATHENA) \
    || defined(FEAT_GUI_MAC) \
    || defined(FEAT_GUI_W32) \
    || defined(FEAT_GUI_W16) \
    || defined(FEAT_GUI_BEOS) \
    || defined(FEAT_GUI_AMIGA) \
    || defined(FEAT_GUI_PHOTON)
# ifndef FEAT_GUI
#  define FEAT_GUI
# endif
#endif

#if defined(FEAT_GUI_W32) || defined(FEAT_GUI_W16)
# define FEAT_GUI_MSWIN
#endif
#if defined(WIN16) || defined(WIN32) || defined(_WIN64)
# define MSWIN
#endif
/* Practically everything is common to both Win32 and Win64 */
#if defined(WIN32) || defined(_WIN64)
# define WIN3264
#endif

/*
 * SIZEOF_INT is used in feature.h, and the system-specific included files
 * need items from feature.h.  Therefore define SIZEOF_INT here.
 */
#ifdef WIN3264
# define SIZEOF_INT 4
#endif
#ifdef MSDOS
# ifdef DJGPP
#  ifndef FEAT_GUI_GTK		/* avoid problems when generating prototypes */
#   define SIZEOF_INT 4		/* 32 bit ints */
#  endif
#  define DOS32
#  define FEAT_CLIPBOARD
# else
#  ifndef FEAT_GUI_GTK		/* avoid problems when generating prototypes */
#   define SIZEOF_INT 2		/* 16 bit ints */
#  endif
#  define SMALL_MALLOC		/* 16 bit storage allocation */
#  define DOS16
# endif
#endif

#ifdef AMIGA
  /* Be conservative about sizeof(int). It could be 4 too. */
# ifndef FEAT_GUI_GTK	/* avoid problems when generating prototypes */
#  ifdef __GNUC__
#   define SIZEOF_INT	4
#  else
#   define SIZEOF_INT	2
#  endif
# endif
#endif
#ifdef MACOS
# if defined(__POWERPC__) || defined(__fourbyteints__) \
  || defined(__MRC__) || defined(__SC__) || defined(__APPLE_CC__)/* MPW Compilers */
#  define SIZEOF_INT 4
# else
#  define SIZEOF_INT 2
# endif
#endif
#ifdef RISCOS
# define SIZEOF_INT 4
#endif


#include "feature.h"	/* #defines for optionals and features */

/* +x11 is only enabled when it's both available and wanted. */
#if defined(HAVE_X11) && defined(WANT_X11)
# define FEAT_X11
#endif

/* Can't use "PACKAGE" here, conflicts with a Perl include file. */
#ifndef VIMPACKAGE
# define VIMPACKAGE	"vim"
#endif

/*
 * Find out if function definitions should include argument types
 */
#ifdef AZTEC_C
# include <functions.h>
# define __ARGS(x)  x
#endif

#ifdef SASC
# include <clib/exec_protos.h>
# define __ARGS(x)  x
#endif

#ifdef _DCC
# include <clib/exec_protos.h>
# define __ARGS(x)  x
#endif

#ifdef __TURBOC__
# define __ARGS(x) x
#endif

#ifdef __BEOS__
# include "os_beos.h"
# define __ARGS(x)  x
#endif

#if (defined(UNIX) || defined(__EMX__) || defined(VMS)) \
	&& (!defined(MACOS_X) || defined(HAVE_CONFIG_H))
# include "os_unix.h"	    /* bring lots of system header files */
#endif

#if defined(MACOS) && (defined(__MRC__) || defined(__SC__))
   /* Apple's Compilers support prototypes */
# define __ARGS(x) x
#endif
#ifndef __ARGS
# if defined(__STDC__) || defined(__GNUC__) || defined(WIN3264)
#  define __ARGS(x) x
# else
#  define __ARGS(x) ()
# endif
#endif

/* __ARGS and __PARMS are the same thing. */
#ifndef __PARMS
# define __PARMS(x) __ARGS(x)
#endif

#if defined(UNIX) && !defined(MACOS_X) /* MACOS_X doesn't yet support osdef.h */
# include "auto/osdef.h"	/* bring missing declarations in */
#endif

#ifdef __EMX__
# define    getcwd  _getcwd2
# define    chdir   _chdir2
# undef	    CHECK_INODE
#endif

#ifdef AMIGA
# include "os_amiga.h"
#endif

#ifdef MSDOS
# include "os_msdos.h"
#endif

#ifdef WIN16
# include "os_win16.h"
#endif

#ifdef WIN3264
# include "os_win32.h"
#endif

#ifdef __MINT__
# include "os_mint.h"
#endif

#if defined(MACOS)
# if defined(__MRC__) || defined(__SC__) /* MPW Compilers */
#  define HAVE_SETENV
# endif
# include "os_mac.h"
#endif

#ifdef RISCOS
# include "os_riscos.h"
#endif

#ifdef __QNX__
# include "os_qnx.h"
#endif

#ifdef FEAT_SUN_WORKSHOP
# include "workshop.h"
#endif

#ifdef X_LOCALE
# include <X11/Xlocale.h>
#else
# ifdef HAVE_LOCALE_H
#  include <locale.h>
# endif
#endif

/*
 * Maximum length of a path (for non-unix systems) Make it a bit long, to stay
 * on the safe side.  But not too long to put on the stack.
 */
#ifndef MAXPATHL
# ifdef MAXPATHLEN
#  define MAXPATHL  MAXPATHLEN
# else
#  define MAXPATHL  256
# endif
#endif
#ifdef BACKSLASH_IN_FILENAME
# define PATH_ESC_CHARS ((char_u *)" *?[{`%#")
#else
# ifdef COLON_AS_PATHSEP
#  define PATH_ESC_CHARS ((char_u *)" *?[{`$%#/")
# else
#  define PATH_ESC_CHARS ((char_u *)" *?[{`$\\%#'\"|")
# endif
#endif

#define NUMBUFLEN 30	    /* length of a buffer to store a number in ASCII */

/*
 * Shorthand for unsigned variables. Many systems, but not all, have u_char
 * already defined, so we use char_u to avoid trouble.
 */
typedef unsigned char	char_u;
typedef unsigned short	short_u;
typedef unsigned int	int_u;
/* Make sure long_u is big enough to hold a pointer.  On Win64 longs are 32
 * bit and pointers 64 bit. */
#ifdef _WIN64
typedef unsigned __int64 long_u;
#else
typedef unsigned long	long_u;
#endif

/*
 * The characters and attributes cached for the screen.
 * Currently a single byte.  The attributes may become larger some day.
 */
#define schar_T	char_u
#define sattr_T	char_u

/*
 * The u8char_T can hold one decoded UTF-8 character.
 * Vim always use an int (32 bits) for characters most places, so that we can
 * handle 32 bit characters in the file.  u8char_T is only used for
 * displaying.  That should be enough, because there is no font for > 16 bits.
 */
#ifdef FEAT_MBYTE
typedef unsigned short u8char_T;
#endif

#ifndef UNIX		    /* For Unix this is included in os_unix.h */
# include <stdio.h>
# include <ctype.h>
#endif

#include "ascii.h"
#include "keymap.h"
#include "term.h"
#include "macros.h"

#ifdef LATTICE
# include <sys/types.h>
# include <sys/stat.h>
#endif
#ifdef _DCC
# include <sys/stat.h>
#endif
#if defined(MSDOS) || defined(MSWIN)
# include <sys/stat.h>
#endif

/*
 * Allow other (non-unix) systems to configure themselves now
 * These are also in os_unix.h, because osdef.sh needs them there.
 */
#ifndef UNIX
/* Note: Some systems need both string.h and strings.h (Savage).  If the
 * system can't handle this, define NO_STRINGS_WITH_STRING_H. */
# ifdef HAVE_STRING_H
#  include <string.h>
# endif
# if defined(HAVE_STRINGS_H) && !defined(NO_STRINGS_WITH_STRING_H)
#   include <strings.h>
# endif
# ifdef HAVE_STAT_H
#  include <stat.h>
# endif
# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif /* NON-UNIX */

#include <assert.h>

/* ================ end of the header file puzzle =============== */

/*
 * For dynamically loaded imm library. Currently, only for Win32.
 */
#ifdef DYNAMIC_IME
# ifndef FEAT_MBYTE_IME
#  define FEAT_MBYTE_IME
# endif
#endif

/*
 * Check input method control.
 */
#if defined(FEAT_XIM) || \
    (defined(FEAT_GUI) && (defined(FEAT_MBYTE_IME) || defined(GLOBAL_IME)))
# define USE_IM_CONTROL
#endif

/*
 * For dynamically loaded gettext library.  Currently, only for Win32.
 */
#ifdef DYNAMIC_GETTEXT
# ifndef FEAT_GETTEXT
#  define FEAT_GETTEXT
# endif
/* These are in os_win32.c */
extern char* (*dyn_libintl_gettext)(const char* msgid);
extern char* (*dyn_libintl_bindtextdomain)(const char* domainname, const char* dirname);
extern char* (*dyn_libintl_textdomain)(const char* domainname);
#endif


/*
 * The _() stuff is for using gettext().  It is a no-op when libintl.h is not
 * found or the +multilang feature is disabled.
 */
#ifdef FEAT_GETTEXT
# ifdef DYNAMIC_GETTEXT
#  define _(x) (*dyn_libintl_gettext)((char *)(x))
#  define N_(x) x
#  define bindtextdomain(domain,dir) (*dyn_libintl_bindtextdomain)(domain,dir)
#  define textdomain(domain) (*dyn_libintl_textdomain)(domain)
# else
#  include <libintl.h>
#  define _(x) gettext((char *)(x))
#  ifdef gettext_noop
#   define N_(x) gettext_noop(x)
#  else
#   define N_(x) x
#  endif
# endif
#else
# define _(x) ((char *)(x))
# define N_(x) x
# ifdef bindtextdomain
#  undef bindtextdomain
# endif
# define bindtextdomain(x, y) /* empty */
# ifdef textdomain
#  undef textdomain
# endif
# define textdomain(x) /* empty */
#endif

/*
 * flags for update_screen()
 * The higher the value, the higher the priority
 */
#define VALID			10  /* buffer not changed, or changes marked
				       with b_mod_* */
#define INVERTED		20  /* redisplay inverted part that changed */
#define INVERTED_ALL		25  /* redisplay whole inverted part */
#define REDRAW_TOP		30  /* display first w_upd_rows screen lines */
#define NOT_VALID		40  /* buffer needs complete redraw */
#define CLEAR			50  /* screen messed up, clear it */

/*
 * Hints used to optimize screen updating.
 */
#define HINT_NONE	0	    /* no current hint */
#define HINT_DEL_CHAR	1	    /* delete character */
#define HINT_INS_CHAR	2	    /* insert character */

/*
 * Flags for w_valid.
 * These are set when something in a window structure becomes invalid, except
 * when the cursor is moved.  Call check_cursor_moved() before testing one of
 * the flags.
 * These are reset when that thing has been updated and is valid again.
 *
 * Every function that invalidates one of these must call one of the
 * invalidate_* functions.
 *
 * w_valid is supposed to be used only in screen.c.  From other files, use the
 * functions that set or reset the flags.
 *
 * VALID_BOTLINE    VALID_BOTLINE_AP
 *     on		on		w_botline valid
 *     off		on		w_botline approximated
 *     off		off		w_botline not valid
 *     on		off		not possible
 */
#define VALID_WROW	0x01	/* w_wrow (window row) is valid */
#define VALID_WCOL	0x02	/* w_wcol (window col) is valid */
#define VALID_VIRTCOL	0x04	/* w_virtcol (file col) is valid */
#define VALID_CHEIGHT	0x08	/* w_cline_height and w_cline_folded valid */
#define VALID_CROW	0x10	/* w_cline_row is valid */
#define VALID_BOTLINE	0x20	/* w_botine and w_empty_rows are valid */
#define VALID_BOTLINE_AP 0x40	/* w_botine is approximated */
#define VALID_TOPLINE	0x80	/* w_topline is valid (for cursor position) */

/*
 * Terminal highlighting attribute bits.
 * Attibutes above HL_ALL are used for syntax highlighting.
 */
#define HL_NORMAL		0x00
#define HL_INVERSE		0x01
#define HL_BOLD			0x02
#define HL_ITALIC		0x04
#define HL_UNDERLINE		0x08
#define HL_STANDOUT		0x10
#define HL_ALL			0x1f

/* special attribute addition: Put message in history */
#define MSG_HIST		0x1000

/*
 * values for State
 *
 * The lower bits up to 0x20 are used to distinguish normal/visual/op_pending
 * and cmdline/insert+replace mode.  This is used for mapping.  If none of
 * these bits are set, no mapping is done.
 * The upper bits are used to distinguish between other states.
 */
#define NORMAL		0x01	/* Normal mode, command expected */
#define VISUAL		0x02	/* Visual mode - use get_real_state() */
#define OP_PENDING	0x04	/* Normal mode, operator is pending - use
				   get_real_state() */
#define CMDLINE		0x08	/* Editing command line */
#define INSERT		0x10	/* Insert mode */
#define LANGMAP		0x20	/* Language mapping, can be combined with
				   INSERT and CMDLINE */
#define MAP_ALL_MODES	0x3f	/* all mode bits used for mapping */

#define REPLACE_FLAG	0x40	/* Replace mode flag */
#define REPLACE		(REPLACE_FLAG + INSERT)
#ifdef FEAT_VREPLACE
# define VREPLACE_FLAG	0x80	/* Virtual-replace mode flag */
# define VREPLACE	(REPLACE_FLAG + VREPLACE_FLAG + INSERT)
#endif
#define LREPLACE	(REPLACE_FLAG + LANGMAP)

#define NORMAL_BUSY	(0x100 + NORMAL) /* Normal mode, busy with a command */
#define HITRETURN	(0x200 + NORMAL) /* waiting for return or command */
#define ASKMORE		0x300	/* Asking if you want --more-- */
#define SETWSIZE	0x400	/* window size has changed */
#define ABBREV		0x500	/* abbreviation instead of mapping */
#define EXTERNCMD	0x600	/* executing an external command */
#define SHOWMATCH	(0x700 + INSERT) /* show matching paren */
#define CONFIRM		0x800	/* ":confirm" prompt */

/* directions */
#define FORWARD			1
#define BACKWARD		(-1)
#define FORWARD_FILE		3
#define BACKWARD_FILE		(-3)

/* return values for functions */
#if !(defined(OK) && (OK == 1))
/* OK already defined to 1 in MacOS X curses, skip this */
# define OK			1
#endif
#define FAIL			0

/* flags for b_flags */
#define BF_RECOVERED	0x01	/* buffer has been recovered */
#define BF_CHECK_RO	0x02	/* need to check readonly when loading file
				   into buffer (set by ":e", may be reset by
				   ":buf" */
#define BF_NEVERLOADED	0x04	/* file has never been loaded into buffer,
				   many variables still need to be set */
#define BF_NOTEDITED	0x08	/* Set when file name is changed after
				   starting to edit, reset when file is
				   written out. */
#define BF_NEW		0x10	/* file didn't exist when editing started */
#define BF_NEW_W	0x20	/* Warned for BF_NEW and file created */
#define BF_READERR	0x40	/* got errors while reading the file */

/* Mask to check for flags that prevent normal writing */
#define BF_WRITE_MASK	(BF_NOTEDITED + BF_NEW + BF_READERR)

/*
 * values for xp_context when doing command line completion
 */
#define CONTEXT_UNKNOWN		(-3)
#define EXPAND_UNSUCCESSFUL	(-2)
#define EXPAND_OK		(-1)
#define EXPAND_NOTHING		0
#define EXPAND_COMMANDS		1
#define EXPAND_FILES		2
#define EXPAND_DIRECTORIES	3
#define EXPAND_SETTINGS		4
#define EXPAND_BOOL_SETTINGS	5
#define EXPAND_TAGS		6
#define EXPAND_OLD_SETTING	7
#define EXPAND_HELP		8
#define EXPAND_BUFFERS		9
#define EXPAND_EVENTS		10
#define EXPAND_MENUS		11
#define EXPAND_SYNTAX		12
#define EXPAND_HIGHLIGHT	13
#define EXPAND_AUGROUP		14
#define EXPAND_USER_VARS	15
#define EXPAND_MAPPINGS		16
#define EXPAND_TAGS_LISTFILES	17
#define EXPAND_FUNCTIONS	18
#define EXPAND_USER_FUNC	19
#define EXPAND_EXPRESSION	20
#define EXPAND_MENUNAMES	21
#define EXPAND_USER_COMMANDS	22
#define EXPAND_USER_CMD_FLAGS	23
#define EXPAND_USER_NARGS	24
#define EXPAND_USER_COMPLETE	25
#define EXPAND_ENV_VARS		26
#define EXPAND_LANGUAGE		27
#define EXPAND_COLORS		28
#define EXPAND_COMPILER		29
#define EXPAND_USER_DEFINED	30

/* Values for exmode_active (0 is no exmode) */
#define EXMODE_NORMAL		1
#define EXMODE_VIM		2

/* Values for nextwild() and ExpandOne().  See ExpandOne() for meaning. */
#define WILD_FREE		1
#define WILD_EXPAND_FREE	2
#define WILD_EXPAND_KEEP	3
#define WILD_NEXT		4
#define WILD_PREV		5
#define WILD_ALL		6
#define WILD_LONGEST		7

#define WILD_LIST_NOTFOUND	1
#define WILD_HOME_REPLACE	2
#define WILD_USE_NL		4
#define WILD_NO_BEEP		8
#define WILD_ADD_SLASH		16
#define WILD_KEEP_ALL		32
#define WILD_SILENT		64
#define WILD_ESCAPE		128

/* Flags for expand_wildcards() */
#define EW_DIR		1	/* include directory names */
#define EW_FILE		2	/* include file names */
#define EW_NOTFOUND	4	/* include not found names */
#define EW_ADDSLASH	8	/* append slash to directory name */
#define EW_KEEPALL	16	/* keep all matches */
#define EW_SILENT	32	/* don't print "1 returned" from shell */
/* Note: mostly EW_NOTFOUND and EW_SILENT are mutually exclusive: EW_NOTFOUND
 * is used when executing commands and EW_SILENT for interactive expanding. */

#ifdef FEAT_VERTSPLIT
# define W_WINCOL(wp)	(wp->w_wincol)
# define W_WIDTH(wp)	(wp->w_width)
# define W_ENDCOL(wp)	(wp->w_wincol + wp->w_width)
# define W_VSEP_WIDTH(wp) (wp->w_vsep_width)
#else
# define W_WINCOL(wp)	0
# define W_WIDTH(wp)	Columns
# define W_ENDCOL(wp)	Columns
# define W_VSEP_WIDTH(wp) 0
#endif
#ifdef FEAT_WINDOWS
# define W_STATUS_HEIGHT(wp) (wp->w_status_height)
# define W_WINROW(wp)	(wp->w_winrow)
#else
# define W_STATUS_HEIGHT(wp) 0
# define W_WINROW(wp)	0
#endif

#ifdef NO_EXPANDPATH
# define gen_expand_wildcards mch_expand_wildcards
#endif

/* Values for the find_pattern_in_path() function args 'type' and 'action': */
#define FIND_ANY	1
#define FIND_DEFINE	2
#define CHECK_PATH	3

#define ACTION_SHOW	1
#define ACTION_GOTO	2
#define ACTION_SPLIT	3
#define ACTION_SHOW_ALL	4
#ifdef FEAT_INS_EXPAND
# define ACTION_EXPAND	5
#endif

#ifdef FEAT_SYN_HL
# define SST_MIN_ENTRIES 150	/* minimal size for state stack array */
# ifdef FEAT_GUI_W16
#  define SST_MAX_ENTRIES 500	/* (only up to 64K blocks) */
# else
#  define SST_MAX_ENTRIES 1000	/* maximal size for state stack array */
# endif
# define SST_FIX_STATES	 7	/* size of sst_stack[]. */
# define SST_DIST	 16	/* normal distance between entries */
# define SST_INVALID	(synstate_T *)-1	/* invalid syn_state pointer */
#endif

/* Values for 'options' argument in do_search() and searchit() */
#define SEARCH_REV    0x01  /* go in reverse of previous dir. */
#define SEARCH_ECHO   0x02  /* echo the search command and handle options */
#define SEARCH_MSG    0x0c  /* give messages (yes, it's not 0x04) */
#define SEARCH_NFMSG  0x08  /* give all messages except not found */
#define SEARCH_OPT    0x10  /* interpret optional flags */
#define SEARCH_HIS    0x20  /* put search pattern in history */
#define SEARCH_END    0x40  /* put cursor at end of match */
#define SEARCH_NOOF   0x80  /* don't add offset to position */
#define SEARCH_START 0x100  /* start search without col offset */
#define SEARCH_MARK  0x200  /* set previous context mark */
#define SEARCH_KEEP  0x400  /* keep previous search pattern */
#define SEARCH_PEEK  0x800  /* peek for typed char, cancel search */

/* Values for find_ident_under_cursor() */
#define FIND_IDENT	1	/* find identifier (word) */
#define FIND_STRING	2	/* find any string (WORD) */

/* Values for file_name_in_line() */
#define FNAME_MESS	1	/* give error message */
#define FNAME_EXP	2	/* expand to path */
#define FNAME_HYP	4	/* check for hypertext link */
#define FNAME_INCL	8	/* apply 'includeexpr' */
#define FNAME_REL	16	/* ".." and "./" are relative to the (current)
				   file instead of the current directory */

/* Values for buflist_getfile() */
#define GETF_SETMARK	0x01	/* set pcmark before jumping */
#define GETF_ALT	0x02	/* jumping to alternate file (not buf num) */
#define GETF_SWITCH	0x04	/* respect 'switchbuf' settings when jumping */

/* Values for buflist_new() flags */
#define BLN_CURBUF	1	/* May re-use curbuf for new buffer */
#define BLN_LISTED	2	/* Put new buffer in buffer list */
#define BLN_DUMMY	4	/* Allocating dummy buffer */

/* Values for in_cinkeys() */
#define KEY_OPEN_FORW	0x101
#define KEY_OPEN_BACK	0x102
#define KEY_COMPLETE	0x103	/* end of completion */

/* Values for "noremap" argument of ins_typebuf().  Also used for
 * map->m_noremap and menu->noremap[]. */
#define REMAP_YES	0	/* allow remapping */
#define REMAP_NONE	-1	/* no remapping */
#define REMAP_SCRIPT	-2	/* remap script-local mappings only */

/* Values for mch_call_shell() second argument */
#define SHELL_FILTER	1	/* filtering text */
#define SHELL_EXPAND	2	/* expanding wildcards */
#define SHELL_COOKED	4	/* set term to cooked mode */
#define SHELL_DOOUT	8	/* redirecting output */
#define SHELL_SILENT	16	/* don't print error returned by command */

/* Values returned by mch_nodetype() */
#define NODE_NORMAL	0	/* file or directory, check with mch_isdir()*/
#define NODE_WRITABLE	1	/* something we can write to (character
				   device, fifo, socket, ..) */
#define NODE_OTHER	2	/* non-writable thing (e.g., block device) */

/* Values for readfile() flags */
#define READ_NEW	0x01	/* read a file into a new buffer */
#define READ_FILTER	0x02	/* read filter output */
#define READ_STDIN	0x04	/* read from stdin */
#define READ_BUFFER	0x08	/* read from curbuf (converting stdin) */
#define READ_DUMMY	0x10	/* reading into a dummy buffer */

/* Values for change_indent() */
#define INDENT_SET	1	/* set indent */
#define INDENT_INC	2	/* increase indent */
#define INDENT_DEC	3	/* decrease indent */

/* Values for flags argument for findmatchlimit() */
#define FM_BACKWARD	0x01	/* search backwards */
#define FM_FORWARD	0x02	/* search forwards */
#define FM_BLOCKSTOP	0x04	/* stop at start/end of block */
#define FM_SKIPCOMM	0x08	/* skip comments */

/* Values for action argument for do_buffer() */
#define DOBUF_GOTO	0	/* go to specified buffer */
#define DOBUF_SPLIT	1	/* split window and go to specified buffer */
#define DOBUF_UNLOAD	2	/* unload specified buffer(s) */
#define DOBUF_DEL	3	/* delete specified buffer(s) from buflist */
#define DOBUF_WIPE	4	/* delete specified buffer(s) really */

/* Values for start argument for do_buffer() */
#define DOBUF_CURRENT	0	/* "count" buffer from current buffer */
#define DOBUF_FIRST	1	/* "count" buffer from first buffer */
#define DOBUF_LAST	2	/* "count" buffer from last buffer */
#define DOBUF_MOD	3	/* "count" mod. buffer from current buffer */

/* Values for sub_cmd and which_pat argument for search_regcomp() */
/* Also used for which_pat argument for searchit() */
#define RE_SEARCH	0	/* save/use pat in/from search_pattern */
#define RE_SUBST	1	/* save/use pat in/from subst_pattern */
#define RE_BOTH		2	/* save pat in both patterns */
#define RE_LAST		2	/* use last used pattern if "pat" is NULL */

/* Second argument for vim_regcomp(). */
#define RE_MAGIC	1	/* 'magic' option */
#define RE_STRING	2	/* match in string instead of buffer text */

#ifdef FEAT_SYN_HL
/* values for reg_do_extmatch */
# define REX_SET	1	/* to allow \z\(...\), */
# define REX_USE	2	/* to allow \z\1 et al. */
#endif

/* Return values for fullpathcmp() */
/* Note: can use (fullpathcmp() & FPC_SAME) to check for equal files */
#define FPC_SAME	1	/* both exist and are the same file. */
#define FPC_DIFF	2	/* both exist and are different files. */
#define FPC_NOTX	4	/* both don't exist. */
#define FPC_DIFFX	6	/* one of them doesn't exist. */
#define FPC_SAMEX	7	/* both don't exist and file names are same. */

/* flags for do_ecmd() */
#define ECMD_HIDE	0x01	/* don't free the current buffer */
#define ECMD_SET_HELP	0x02	/* set b_help flag of (new) buffer before
				   opening file */
#define ECMD_OLDBUF	0x04	/* use existing buffer if it exists */
#define ECMD_FORCEIT	0x08	/* ! used in Ex command */
#define ECMD_ADDBUF	0x10	/* don't edit, just add to buffer list */

/* for lnum argument in do_ecmd() */
#define ECMD_LASTL	(linenr_T)0	/* use last position in loaded file */
#define ECMD_LAST	(linenr_T)-1	/* use last position in all files */
#define ECMD_ONE	(linenr_T)1	/* use first line */

/* flags for do_cmdline() */
#define DOCMD_VERBOSE	0x01	/* included command in error message */
#define DOCMD_NOWAIT	0x02	/* don't call wait_return() and friends */
#define DOCMD_REPEAT	0x04	/* repeat exec. until getline() returns NULL */
#define DOCMD_KEYTYPED	0x08	/* don't reset KeyTyped */
#define DOCMD_EXCRESET	0x10	/* reset exception environment (for debugging)*/
#define DOCMD_KEEPLINE  0x20	/* keep typed line for repeating with "." */

/* flags for beginline() */
#define BL_WHITE	1	/* cursor on first non-white in the line */
#define BL_SOL		2	/* use 'sol' option */
#define BL_FIX		4	/* don't leave cursor on a NUL */

/* flags for mf_sync() */
#define MFS_ALL		1	/* also sync blocks with negative numbers */
#define MFS_STOP	2	/* stop syncing when a character is available */
#define MFS_FLUSH	4	/* flushed file to disk */
#define MFS_ZERO	8	/* only write block 0 */

/* flags for buf_copy_options() */
#define BCO_ENTER	1	/* going to enter the buffer */
#define BCO_ALWAYS	2	/* always copy the options */
#define BCO_NOHELP	4	/* don't touch the help related options */

/* flags for do_put() */
#define PUT_FIXINDENT	1	/* make indent look nice */
#define PUT_CURSEND	2	/* leave cursor after end of new text */
#define PUT_LINE	4	/* put register as lines */
#define PUT_LINE_SPLIT	8	/* split line for linewise register */
#define PUT_LINE_FORWARD 16	/* put linewise register below Visual sel. */

/* flags for set_indent() */
#define SIN_CHANGED	1	/* call changed_bytes() when line changed */
#define SIN_INSERT	2	/* insert indent before existing text */
#define SIN_UNDO	4	/* save line for undo before changing it */

/* flags for insertchar() */
#define INSCHAR_FORMAT	1	/* force formatting */
#define INSCHAR_DO_COM	2	/* format comments */
#define INSCHAR_CTRLV	4	/* char typed just after CTRL-V */

/* flags for open_line() */
#define OPENLINE_DELSPACES  1	/* delete spaces after cursor */
#define OPENLINE_DO_COM	    2	/* format comments */
#define OPENLINE_KEEPTRAIL  4	/* keep trailing spaces */
#define OPENLINE_MARKFIX    8	/* fix mark positions */

/*
 * There are four history tables:
 */
#define HIST_CMD	0	/* colon commands */
#define HIST_SEARCH	1	/* search commands */
#define HIST_EXPR	2	/* expressions (from entering = register) */
#define HIST_INPUT	3	/* input() lines */
#define HIST_DEBUG	4	/* debug commands */
#define HIST_COUNT	5	/* number of history tables */

/*
 * Flags for chartab[].
 */
#define CT_CELL_MASK	0x07	/* mask: nr of display cells (1, 2 or 4) */
#define CT_PRINT_CHAR	0x10	/* flag: set for printable chars */
#define CT_ID_CHAR	0x20	/* flag: set for ID chars */
#define CT_FNAME_CHAR	0x40	/* flag: set for file name chars */

/*
 * Values for do_tag().
 */
#define DT_TAG		1	/* jump to newer position or same tag again */
#define DT_POP		2	/* jump to older position */
#define DT_NEXT		3	/* jump to next match of same tag */
#define DT_PREV		4	/* jump to previous match of same tag */
#define DT_FIRST	5	/* jump to first match of same tag */
#define DT_LAST		6	/* jump to first match of same tag */
#define DT_SELECT	7	/* jump to selection from list */
#define DT_HELP		8	/* like DT_TAG, but no wildcards */
#define DT_JUMP		9	/* jump to new tag or selection from list */
#define DT_CSCOPE	10	/* cscope find command (like tjump) */

/*
 * flags for find_tags().
 */
#define TAG_HELP	1	/* only search for help tags */
#define TAG_NAMES	2	/* only return name of tag */
#define	TAG_REGEXP	4	/* use tag pattern as regexp */
#define	TAG_NOIC	8	/* don't always ignore case */
#ifdef FEAT_CSCOPE
# define TAG_CSCOPE	16	/* cscope tag */
#endif
#define TAG_VERBOSE	32	/* message verbosity */
#define TAG_INS_COMP	64	/* Currently doing insert completion */
#define TAG_KEEP_LANG	128	/* keep current language */

#define TAG_MANY	200	/* When finding many tags (for completion),
				   find up to this many tags */

/*
 * Types of dialogs passed to do_vim_dialog().
 */
#define VIM_GENERIC	0
#define VIM_ERROR	1
#define VIM_WARNING	2
#define VIM_INFO	3
#define VIM_QUESTION	4
#define VIM_LAST_TYPE	4	/* sentinel value */

/*
 * Return values for functions like gui_yesnocancel()
 */
#define VIM_OK		1
#define VIM_YES		2
#define VIM_NO		3
#define VIM_CANCEL	4
#define VIM_ALL		5
#define VIM_DISCARDALL  6

/*
 * arguments for win_split()
 */
#define WSP_ROOM	1	/* require enough room */
#define WSP_VERT	2	/* split vertically */
#define WSP_TOP		4	/* window at top-left of shell */
#define WSP_BOT		8	/* window at bottom-right of shell */
#define WSP_HELP	16	/* creating the help window */
#define WSP_BELOW	32	/* put new window below/right */
#define WSP_ABOVE	64	/* put new window above/left */

/*
 * "flags" values for option-setting functions.
 * When OPT_GLOBAL and OPT_LOCAL are both missing, set both local and global
 * values, get local value.
 */
#define OPT_FREE	1	/* free old value if it was allocated */
#define OPT_GLOBAL	2	/* use global value */
#define OPT_LOCAL	4	/* use local value */
#define OPT_MODELINE	8	/* option in modeline */

/* Magic chars used in confirm dialog strings */
#define DLG_BUTTON_SEP	'\n'
#define DLG_HOTKEY_CHAR	'&'

/* Values for "starting" */
#define NO_SCREEN	2	/* no screen updating yet */
#define NO_BUFFERS	1	/* not all buffers loaded yet */
/*			0	   not starting anymore */

/* Values for swap_exists_action: what to do when swap file already exists */
#define SEA_NONE	0	/* don't use dialog */
#define SEA_DIALOG	1	/* use dialog when */
#define SEA_QUIT	2	/* quit editing the file */
#define SEA_RECOVER	3	/* recover the file */

/*
 * Minimal size for block 0 of a swap file.
 * NOTE: This depends on size of struct block0! It's not done with a sizeof(),
 * because struct block0 is defined in memline.c (Sorry).
 * The maximal block size is arbitrary.
 */
#define MIN_SWAP_PAGE_SIZE 1048
#define MAX_SWAP_PAGE_SIZE 50000

/* Special values for current_SID. */
#define SID_MODELINE	-1	/* when using a modeline */
#define SID_CMDARG	-2	/* for "--cmd" argument */
#define SID_CARG	-3	/* for "-c" argument */
#define SID_ENV		-4	/* for sourcing environment variable */

/*
 * Events for autocommands.
 */
enum auto_event
{
    EVENT_BUFADD = 0,		/* after adding a buffer to the buffer list */
    EVENT_BUFNEW,		/* after creating any buffer */
    EVENT_BUFDELETE,		/* deleting a buffer from the buffer list */
    EVENT_BUFWIPEOUT,		/* just before really deleting a buffer */
    EVENT_BUFENTER,		/* after entering a buffer */
    EVENT_BUFFILEPOST,		/* after renaming a buffer */
    EVENT_BUFFILEPRE,		/* before renaming a buffer */
    EVENT_BUFLEAVE,		/* before leaving a buffer */
    EVENT_BUFNEWFILE,		/* when creating a buffer for a new file */
    EVENT_BUFREADPOST,		/* after reading a buffer */
    EVENT_BUFREADPRE,		/* before reading a buffer */
    EVENT_BUFREADCMD,		/* read buffer using command */
    EVENT_BUFUNLOAD,		/* just before unloading a buffer */
    EVENT_BUFHIDDEN,		/* just after buffer becomes hidden */
    EVENT_BUFWINENTER,		/* after showing a buffer in a window */
    EVENT_BUFWINLEAVE,		/* just after buffer removed from window */
    EVENT_BUFWRITEPOST,		/* after writing a buffer */
    EVENT_BUFWRITEPRE,		/* before writing a buffer */
    EVENT_BUFWRITECMD,		/* write buffer using command */
    EVENT_CMDWINENTER,		/* after entering the cmdline window */
    EVENT_CMDWINLEAVE,		/* before leaving the cmdline window */
    EVENT_FILEAPPENDPOST,	/* after appending to a file */
    EVENT_FILEAPPENDPRE,	/* before appending to a file */
    EVENT_FILEAPPENDCMD,	/* appende to a file using command */
    EVENT_FILECHANGEDSHELL,	/* after shell command that changed file */
    EVENT_FILECHANGEDRO,	/* before first change to read-only file */
    EVENT_FILEREADPOST,		/* after reading a file */
    EVENT_FILEREADPRE,		/* before reading a file */
    EVENT_FILEREADCMD,		/* read from a file using command */
    EVENT_FILETYPE,		/* new file type detected (user defined) */
    EVENT_FILEWRITEPOST,	/* after writing a file */
    EVENT_FILEWRITEPRE,		/* before writing a file */
    EVENT_FILEWRITECMD,		/* write to a file using command */
    EVENT_FILTERREADPOST,	/* after reading from a filter */
    EVENT_FILTERREADPRE,	/* before reading from a filter */
    EVENT_FILTERWRITEPOST,	/* after writing to a filter */
    EVENT_FILTERWRITEPRE,	/* before writing to a filter */
    EVENT_FOCUSGAINED,		/* got the focus */
    EVENT_FOCUSLOST,		/* lost the focus to another app */
    EVENT_GUIENTER,		/* after starting the GUI */
    EVENT_STDINREADPOST,	/* after reading from stdin */
    EVENT_STDINREADPRE,		/* before reading from stdin */
    EVENT_SYNTAX,		/* syntax selected */
    EVENT_TERMCHANGED,		/* after changing 'term' */
    EVENT_TERMRESPONSE,		/* after setting "v:termresponse" */
    EVENT_USER,			/* user defined autocommand */
    EVENT_VIMENTER,		/* after starting Vim */
    EVENT_VIMLEAVE,		/* before exiting Vim */
    EVENT_VIMLEAVEPRE,		/* before exiting Vim and writing .viminfo */
    EVENT_WINENTER,		/* after entering a window */
    EVENT_WINLEAVE,		/* before leaving a window */
    EVENT_ENCODINGCHANGED,	/* after changing the 'encoding' option */
    EVENT_CURSORHOLD,		/* cursor in same position for a while */
    EVENT_FUNCUNDEFINED,	/* if calling a function which doesn't exist */
    EVENT_REMOTEREPLY,		/* upon string reception from a remote vim */
    NUM_EVENTS			/* MUST be the last one */
};

typedef enum auto_event EVENT_T;

/*
 * Values for index in highlight_attr[].
 * When making changes, also update HL_FLAGS below!  And update the default
 * value of 'highlight' in option.c.
 */
enum hlf_value
{
    HLF_8 = 0	    /* Meta & special keys listed with ":map", text that is
		       displayed different from what it is */
    , HLF_AT	    /* @ and ~ characters at end of screen, characters that
		       don't really exist in the text */
    , HLF_D	    /* directories in CTRL-D listing */
    , HLF_E	    /* error messages */
    , HLF_H	    /* obsolete, ignored */
    , HLF_I	    /* incremental search */
    , HLF_L	    /* last search string */
    , HLF_M	    /* "--More--" message */
    , HLF_CM	    /* Mode (e.g., "-- INSERT --") */
    , HLF_N	    /* line number for ":number" and ":#" commands */
    , HLF_R	    /* return to continue message and yes/no questions */
    , HLF_S	    /* status lines */
    , HLF_SNC	    /* status lines of not-current windows */
    , HLF_C	    /* column to separate vertically split windows */
    , HLF_T	    /* Titles for output from ":set all", ":autocmd" etc. */
    , HLF_V	    /* Visual mode */
    , HLF_VNC	    /* Visual mode, autoselecting and not clipboard owner */
    , HLF_W	    /* warning messages */
    , HLF_WM	    /* Wildmenu highlight */
    , HLF_FL	    /* Folded line */
    , HLF_FC	    /* Fold column */
    , HLF_ADD	    /* Added diff line */
    , HLF_CHD	    /* Changed diff line */
    , HLF_DED	    /* Deleted diff line */
    , HLF_TXD	    /* Text Changed in diff line */
    , HLF_COUNT	    /* MUST be the last one */
};

/* the HL_FLAGS must be in the same order as the HLF_ enums! */
#define HL_FLAGS {'8', '@', 'd', 'e', 'h', 'i', 'l', 'm', 'M', \
		  'n', 'r', 's', 'S', 'c', 't', 'v', 'V', 'w', 'W', \
		  'f', 'F', 'A', 'C', 'D', 'T'}

/*
 * Boolean constants
 */
#ifndef TRUE
# define FALSE	0	    /* note: this is an int, not a long! */
# define TRUE	1
#endif

#define MAYBE	2	    /* sometimes used for a variant on TRUE */

/* May be returned by add_new_completion(): */
#define RET_ERROR		(-1)

/*
 * Operator IDs; The order must correspond to opchars[] in ops.c!
 */
#define OP_NOP		0	/* no pending operation */
#define OP_DELETE	1	/* "d"  delete operator */
#define OP_YANK		2	/* "y"  yank operator */
#define OP_CHANGE	3	/* "c"  change operator */
#define OP_LSHIFT	4	/* "<"  left shift operator */
#define OP_RSHIFT	5	/* ">"  right shift operator */
#define OP_FILTER	6	/* "!"  filter operator */
#define OP_TILDE	7	/* "g~" switch case operator */
#define OP_INDENT	8	/* "="  indent operator */
#define OP_FORMAT	9	/* "gq" format operator */
#define OP_COLON	10	/* ":"  colon operator */
#define OP_UPPER	11	/* "gU" make upper case operator */
#define OP_LOWER	12	/* "gu" make lower case operator */
#define OP_JOIN		13	/* "J"  join operator, only for Visual mode */
#define OP_JOIN_NS	14	/* "gJ"  join operator, only for Visual mode */
#define OP_ROT13	15	/* "g?" rot-13 encoding */
#define OP_REPLACE	16	/* "r"  replace chars, only for Visual mode */
#define OP_INSERT	17	/* "I"  Insert column, only for Visual mode */
#define OP_APPEND	18	/* "A"  Append column, only for Visual mode */
#define OP_FOLD		19	/* "zf" define a fold */
#define OP_FOLDOPEN	20	/* "zo" open folds */
#define OP_FOLDOPENREC	21	/* "zO" open folds recursively */
#define OP_FOLDCLOSE	22	/* "zc" close folds */
#define OP_FOLDCLOSEREC	23	/* "zC" close folds recursively */
#define OP_FOLDDEL	24	/* "zd" delete folds */
#define OP_FOLDDELREC	25	/* "zD" delete folds recursively */
#define OP_FORMAT2	26	/* "gw" format operator, keeps cursor pos */

/*
 * Motion types, used for operators and for yank/delete registers.
 */
#define MCHAR	0		/* character-wise movement/register */
#define MLINE	1		/* line-wise movement/register */
#define MBLOCK	2		/* block-wise register */

#define MAUTO	0xff		/* Decide between MLINE/MCHAR */

/*
 * Minimum screen size
 */
#define MIN_COLUMNS	12	/* minimal columns for screen */
#define MIN_LINES	2	/* minimal lines for screen */
#define STATUS_HEIGHT	1	/* height of a status line under a window */
#define QF_WINHEIGHT	10	/* default height for quickfix window */

/*
 * Buffer sizes
 */
#ifndef CMDBUFFSIZE
# define CMDBUFFSIZE	256	/* size of the command processing buffer */
#endif

#define LSIZE	    512		/* max. size of a line in the tags file */

#define IOSIZE	   (1024+1)	/* file i/o and sprintf buffer size */
#define MSG_BUF_LEN 80		/* length of buffer for small messages */

#if defined(AMIGA) || defined(__linux__) || defined(__QNX__) || defined(__CYGWIN32__) || defined(_AIX)
# define TBUFSZ 2048		/* buffer size for termcap entry */
#else
# define TBUFSZ 1024		/* buffer size for termcap entry */
#endif

/*
 * Maximum length of key sequence to be mapped.
 * Must be able to hold an Amiga resize report.
 */
#define MAXMAPLEN   50

#ifdef BINARY_FILE_IO
# define WRITEBIN   "wb"	/* no CR-LF translation */
# define READBIN    "rb"
# define APPENDBIN  "ab"
#else
# define WRITEBIN   "w"
# define READBIN    "r"
# define APPENDBIN  "a"
#endif

/*
 * EMX doesn't have a global way of making open() use binary I/O.
 * Use O_BINARY for all open() calls.
 */
#if defined(__EMX__) || defined(__CYGWIN32__)
# define O_EXTRA    O_BINARY
#else
# define O_EXTRA    0
#endif

#ifndef W_OK
# define W_OK 2		/* for systems that don't have W_OK in unistd.h */
#endif
#ifndef R_OK
# define R_OK 4		/* for systems that don't have R_OK in unistd.h */
#endif

/*
 * defines to avoid typecasts from (char_u *) to (char *) and back
 * (vim_strchr() and vim_strrchr() are now in alloc.c)
 */
#define STRLEN(s)	    strlen((char *)(s))
#define STRCPY(d, s)	    strcpy((char *)(d), (char *)(s))
#define STRNCPY(d, s, n)    strncpy((char *)(d), (char *)(s), (size_t)(n))
#define STRCMP(d, s)	    strcmp((char *)(d), (char *)(s))
#define STRNCMP(d, s, n)    strncmp((char *)(d), (char *)(s), (size_t)(n))
#ifdef HAVE_STRCASECMP
# define STRICMP(d, s)	    strcasecmp((char *)(d), (char *)(s))
#else
# ifdef HAVE_STRICMP
#  define STRICMP(d, s)	    stricmp((char *)(d), (char *)(s))
# else
#  define STRICMP(d, s)	    vim_stricmp((char *)(d), (char *)(s))
# endif
#endif

#ifdef HAVE_STRNCASECMP
# define STRNICMP(d, s, n)  strncasecmp((char *)(d), (char *)(s), (size_t)(n))
#else
# ifdef HAVE_STRNICMP
#  define STRNICMP(d, s, n) strnicmp((char *)(d), (char *)(s), (size_t)(n))
# else
#  define STRNICMP(d, s, n) vim_strnicmp((char *)(d), (char *)(s), (size_t)(n))
# endif
#endif

#ifdef FEAT_MBYTE
# define MB_STRICMP(d, s)	(has_mbyte ? mb_strnicmp((char_u *)(d), (char_u *)(s), (int)MAXCOL) : STRICMP((d), (s)))
# define MB_STRNICMP(d, s, n)	(has_mbyte ? mb_strnicmp((char_u *)(d), (char_u *)(s), (int)(n)) : STRNICMP((d), (s), (n)))
#else
# define MB_STRICMP(d, s)	STRICMP((d), (s))
# define MB_STRNICMP(d, s, n)	STRNICMP((d), (s), (n))
# define transchar_byte(c)	transchar(c)
#endif

#define STRCAT(d, s)	    strcat((char *)(d), (char *)(s))
#define STRNCAT(d, s, n)    strncat((char *)(d), (char *)(s), (size_t)(n))

#ifdef HAVE_STRPBRK
# define vim_strpbrk(s, cs) (char_u *)strpbrk((char *)(s), (char *)(cs))
#endif

#define MSG(s)			    msg((char_u *)(s))
#define MSG_ATTR(s, attr)	    msg_attr((char_u *)(s), (attr))
#define EMSG(s)			    emsg((char_u *)(s))
#define EMSG2(s, p)		    emsg2((char_u *)(s), (char_u *)(p))
#define EMSG3(s, p, q)		    emsg3((char_u *)(s), (char_u *)(p), (char_u *)(q))
#define EMSGN(s, n)		    emsgn((char_u *)(s), (long)(n))
#define OUT_STR(s)		    out_str((char_u *)(s))
#define OUT_STR_NF(s)		    out_str_nf((char_u *)(s))
#define MSG_PUTS(s)		    msg_puts((char_u *)(s))
#define MSG_PUTS_ATTR(s, a)	    msg_puts_attr((char_u *)(s), (a))
#define MSG_PUTS_TITLE(s)	    msg_puts_title((char_u *)(s))
#define MSG_PUTS_LONG(s)	    msg_puts_long((char_u *)(s))
#define MSG_PUTS_LONG_ATTR(s, a)    msg_puts_long_attr((char_u *)(s), (a))

/* Prefer using emsg3(), because perror() may send the output to the wrong
 * destination and mess up the screen. */
#ifdef HAVE_STRERROR
# define PERROR(msg)		    (void)emsg3((char_u *)"%s: %s", (char_u *)msg, (char_u *)strerror(errno))
#else
# define PERROR(msg)		    perror(msg)
#endif

typedef long	    linenr_T;		/* line number type */
typedef unsigned    colnr_T;		/* column number type */
typedef unsigned short disptick_T;	/* display tick type */

#define MAXLNUM (0x7fffffffL)		/* maximum (invalid) line number */

/*
 * Well, you won't believe it, but some S/390 machines ("host", now also known
 * as zServer) us 31 bit pointers. There are also some newer machines, that
 * use 64 bit pointers. I don't know how to distinguish between 31 and 64 bit
 * machines, so the best way is to assume 31 bits whenever we detect OS/390
 * Unix.
 * With this we restrict the maximum line length to 1073741823. I guess this is
 * not a real problem. BTW:  Longer lines are split.
 */
#if SIZEOF_INT >= 4
# ifdef __MVS__
#  define MAXCOL (0x3fffffffL)		/* maximum column number, 30 bits */
# else
#  define MAXCOL (0x7fffffffL)		/* maximum column number, 31 bits */
# endif
#else
# define MAXCOL	(0x7fff)		/* maximum column number, 15 bits */
#endif

#define SHOWCMD_COLS 10			/* columns needed by shown command */
#define STL_MAX_ITEM 80			/* max nr of %<flag> in statusline */

typedef void	    *vim_acl_T;		/* dummy to pass an ACL to a function */

/*
 * Include a prototype for mch_memmove(), it may not be in alloc.pro.
 */
#ifdef VIM_MEMMOVE
void mch_memmove __ARGS((void *, void *, size_t));
#else
# ifndef mch_memmove
#  define mch_memmove(to, from, len) memmove(to, from, len)
# endif
#endif

/*
 * fnamecmp() is used to compare file names.
 * On some systems case in a file name does not matter, on others it does.
 * (this does not account for maximum name lengths and things like "../dir",
 * thus it is not 100% accurate!)
 */
#ifdef CASE_INSENSITIVE_FILENAME
# ifdef BACKSLASH_IN_FILENAME
#  define fnamecmp(x, y) vim_fnamecmp((x), (y))
#  define fnamencmp(x, y, n) vim_fnamencmp((x), (y), (size_t)(n))
# else
#  define fnamecmp(x, y) MB_STRICMP((x), (y))
#  define fnamencmp(x, y, n) MB_STRNICMP((x), (y), (n))
# endif
#else
# define fnamecmp(x, y) strcmp((char *)(x), (char *)(y))
# define fnamencmp(x, y, n) strncmp((char *)(x), (char *)(y), (size_t)(n))
#endif

#ifdef HAVE_MEMSET
# define vim_memset(ptr, c, size)   memset((ptr), (c), (size))
#else
void *vim_memset __ARGS((void *, int, size_t));
#endif

#ifdef HAVE_MEMCMP
# define vim_memcmp(p1, p2, len)   memcmp((p1), (p2), (len))
#else
# ifdef HAVE_BCMP
#  define vim_memcmp(p1, p2, len)   bcmp((p1), (p2), (len))
# else
int vim_memcmp __ARGS((void *, void *, size_t));
#  define VIM_MEMCMP
# endif
#endif

#if defined(UNIX) || defined(FEAT_GUI) || defined(OS2) || defined(VMS) \
	|| defined(FEAT_CLIENTSERVER)
# define USE_INPUT_BUF
#endif

#ifdef MSWIN
/* On MS-Windows the third argument isn't size_t.  This matters for Win64,
 * where sizeof(size_t)==8, not 4 */
# define vim_read(fd, buf, count)   read((fd), (char *)(buf), (unsigned int)(count))
# define vim_write(fd, buf, count)  write((fd), (char *)(buf), (unsigned int)(count))
#else
# define vim_read(fd, buf, count)   read((fd), (char *)(buf), (size_t) (count))
# define vim_write(fd, buf, count)  write((fd), (char *)(buf), (size_t) (count))
#endif

/*
 * Enums need a typecast to be used as array index (for Ultrix).
 */
#define hl_attr(n)	highlight_attr[(int)(n)]
#define term_str(n)	term_strings[(int)(n)]

/*
 * vim_iswhite() is used for "^" and the like. It differs from isspace()
 * because it doesn't include <CR> and <LF> and the like.
 */
#define vim_iswhite(x)	((x) == ' ' || (x) == '\t')

/* Note that gui.h is included by structs.h */

#include "structs.h"	    /* file that defines many structures */

#ifdef FEAT_MOUSE

/* Codes for mouse button events in lower three bits: */
# define MOUSE_LEFT	0x00
# define MOUSE_MIDDLE	0x01
# define MOUSE_RIGHT	0x02
# define MOUSE_RELEASE	0x03

/* bit masks for modifiers: */
# define MOUSE_SHIFT	0x04
# define MOUSE_ALT	0x08
# define MOUSE_CTRL	0x10

/* mouse buttons that are handled like a key press (GUI only) */
# define MOUSE_4	0x100	/* scroll wheel down */
# define MOUSE_5	0x200	/* scroll wheel up */

# define MOUSE_X1	0x300 /* Mouse-button X1 (6th) */
# define MOUSE_X2	0x400 /* Mouse-button X2 */

/* 0x20 is reserved by xterm */
# define MOUSE_DRAG_XTERM   0x40

# define MOUSE_DRAG	(0x40 | MOUSE_RELEASE)

/* Lowest button code for using the mouse wheel (xterm only) */
# define MOUSEWHEEL_LOW		0x60

# define MOUSE_CLICK_MASK	0x03

# define NUM_MOUSE_CLICKS(code) \
    (((unsigned)((code) & 0xC0) >> 6) + 1)

# define SET_NUM_MOUSE_CLICKS(code, num) \
    (code) = ((code) & 0x3f) | ((((num) - 1) & 3) << 6)

/*
 * jump_to_mouse() returns one of first four these values, possibly with
 * some of the other three added.
 */
# define IN_UNKNOWN		0
# define IN_BUFFER		1
# define IN_STATUS_LINE		2	/* on status or command line */
# define IN_SEP_LINE		4	/* on vertical separator line */
# define IN_OTHER_WIN		8	/* in other window but can't go there */
# define CURSOR_MOVED		0x100
# define MOUSE_FOLD_CLOSE	0x200	/* clicked on '-' in fold column */
# define MOUSE_FOLD_OPEN	0x400	/* clicked on '+' in fold column */

/* flags for jump_to_mouse() */
# define MOUSE_FOCUS		0x01	/* need to stay in this window */
# define MOUSE_MAY_VIS		0x02	/* may start Visual mode */
# define MOUSE_DID_MOVE		0x04	/* only act when mouse has moved */
# define MOUSE_SETPOS		0x08	/* only set current mouse position */
# define MOUSE_MAY_STOP_VIS	0x10	/* may stop Visual mode */
# define MOUSE_RELEASED		0x20	/* button was released */

# if defined(UNIX) && defined(HAVE_GETTIMEOFDAY) && defined(HAVE_SYS_TIME_H)
#  define CHECK_DOUBLE_CLICK 1	/* Checking for double clicks ourselves. */
# endif

#endif /* FEAT_MOUSE */

/* defines for eval_vars() */
#define VALID_PATH		1
#define VALID_HEAD		2

/* Defines for Vim variables.  These must match vimvars[] in eval.c! */
#define VV_COUNT	0
#define VV_COUNT1	1
#define VV_PREVCOUNT	2
#define VV_ERRMSG	3
#define VV_WARNINGMSG	4
#define VV_STATUSMSG	5
#define VV_SHELL_ERROR	6
#define VV_THIS_SESSION	7
#define VV_VERSION	8
#define VV_LNUM		9
#define VV_TERMRESPONSE	10
#define VV_FNAME	11
#define VV_LANG		12
#define VV_LC_TIME	13
#define VV_CTYPE	14
#define VV_CC_FROM	15
#define VV_CC_TO	16
#define VV_FNAME_IN	17
#define VV_FNAME_OUT	18
#define VV_FNAME_NEW	19
#define VV_FNAME_DIFF	20
#define VV_CMDARG	21
#define VV_FOLDSTART	22
#define VV_FOLDEND	23
#define VV_FOLDDASHES	24
#define VV_FOLDLEVEL	25
#define VV_PROGNAME	26
#define VV_SEND_SERVER	27
#define VV_DYING	28
#define VV_EXCEPTION	29
#define VV_THROWPOINT	30
#define VV_REG		31
#define VV_LEN		32	/* number of v: vars */

#ifdef FEAT_CLIPBOARD

/* Vim-specific selection type for X11 */
#define VIM_ATOM_NAME "_VIM_TEXT"

/* Selection states for modeless selection */
# define SELECT_CLEARED		0
# define SELECT_IN_PROGRESS	1
# define SELECT_DONE		2

# define SELECT_MODE_CHAR	0
# define SELECT_MODE_WORD	1
# define SELECT_MODE_LINE	2

# ifdef FEAT_GUI_W32
#  ifdef FEAT_OLE
#   define WM_OLE (WM_APP+0)
#  endif
#  ifdef FEAT_NETBEANS_INTG
    /* message for Netbeans socket event */
#   define WM_NETBEANS (WM_APP+1)
#  endif
# endif

/* Info about selected text */
typedef struct VimClipboard
{
    int		available;	/* Is clipboard available? */
    int		owned;		/* Flag: do we own the selection? */
    pos_T	start;		/* Start of selected area */
    pos_T	end;		/* End of selected area */
    int		vmode;		/* Visual mode character */

    /* Fields for selection that doesn't use Visual mode */
    short_u	origin_row;
    short_u	origin_start_col;
    short_u	origin_end_col;
    short_u	word_start_col;
    short_u	word_end_col;

    pos_T	prev;		/* Previous position */
    short_u	state;		/* Current selection state */
    short_u	mode;		/* Select by char, word, or line. */

# if defined(FEAT_GUI_X11) || defined(FEAT_XCLIPBOARD)
    Atom	sel_atom;	/* PRIMARY/CLIPBOARD selection ID */
# endif

# ifdef FEAT_GUI_GTK
    GdkAtom     gtk_sel_atom;	/* PRIMARY/CLIPBOARD selection ID */
# endif

# ifdef MSWIN
    int_u	format;		/* Vim's own special clipboard format */
    int_u	format_raw;	/* Vim's raw text clipboard format */
# endif
# ifdef FEAT_GUI_BEOS
				/* no clipboard at the moment */
# endif
} VimClipboard;
#else
typedef int VimClipboard;	/* This is required for the prototypes. */
#endif

#ifdef __BORLANDC__
/* work around a bug in the Borland 'stat' function: */
# include <io.h>	    /* for access() */

# define stat(a,b) (access(a,0) ? -1 : stat(a,b))
#endif

/*
 * EXTERN is only defined in main.c.  That's where global variables are
 * actually defined and initialized.
 */
#ifndef EXTERN
# define EXTERN extern
# define INIT(x)
#else
# ifndef INIT
#  define INIT(x) x
#  define DO_INIT
# endif
#endif

#include "option.h"	    /* option variables and defines */
#include "ex_cmds.h"	    /* Ex command defines */
#include "proto.h"	    /* function prototypes */

/* This has to go after the include of proto.h, as proto/gui.pro declares
 * functions of these names. The declarations would break if the defines had
 * been seen at that stage.  But it must be before globals.h, where error_ga
 * is declared. */
#if !defined(FEAT_GUI_W32) && !defined(FEAT_GUI_X11) \
	&& !defined(FEAT_GUI_GTK) && !defined(FEAT_GUI_MAC)
# define mch_errmsg(str)	fprintf(stderr, "%s", (str))
# define display_errors()	fflush(stderr)
# define mch_msg(str)		printf("%s", (str))
#else
# define USE_MCH_ERRMSG
#endif

#include "globals.h"	    /* global variables and messages */

#ifdef FEAT_SNIFF
# include "if_sniff.h"
#endif

#ifndef FEAT_VIRTUALEDIT
# define getvvcol(w, p, s, c, e) getvcol(w, p, s, c, e)
# define virtual_active() 0
# define virtual_op FALSE
#endif

/*
 * If console dialog not supported, but GUI dialog is, use the GUI one.
 */
#if defined(FEAT_GUI_DIALOG) && !defined(FEAT_CON_DIALOG)
# define do_dialog gui_mch_dialog
#endif

/*
 * Default filters for gui_mch_browse().
 * The filters are almost system independent.  Except for the difference
 * between "*" and "*.*" for MSDOS-like systems.
 * NOTE: Motif only uses the very first pattern.  Therefore
 * BROWSE_FILTER_DEFAULT should start with a "*" pattern.
 */
#ifdef FEAT_BROWSE
# ifdef BACKSLASH_IN_FILENAME
#  define BROWSE_FILTER_MACROS \
	(char_u *)"Vim macro files (*.vim)\t*.vim\nAll Files (*.*)\t*.*\n"
#  define BROWSE_FILTER_ALL_FILES (char_u *)"All Files (*.*)\t*.*\n"
#  define BROWSE_FILTER_DEFAULT \
	(char_u *)"All Files (*.*)\t*.*\nC source (*.c, *.h)\t*.c;*.h\nC++ source (*.cpp, *.hpp)\t*.cpp;*.hpp\nVB code (*.bas, *.frm)\t*.bas;*.frm\nVim files (*.vim, _vimrc, _gvimrc)\t*.vim;_vimrc;_gvimrc\n"
# else
#  define BROWSE_FILTER_MACROS \
	(char_u *)"Vim macro files (*.vim)\t*.vim\nAll Files (*)\t*\n"
#  define BROWSE_FILTER_ALL_FILES (char_u *)"All Files (*)\t*\n"
#  define BROWSE_FILTER_DEFAULT \
	(char_u *)"All Files (*)\t*\nC source (*.c, *.h)\t*.c;*.h\nC++ source (*.cpp, *.hpp)\t*.cpp;*.hpp\nVim files (*.vim, _vimrc, _gvimrc)\t*.vim;_vimrc;_gvimrc\n"
# endif
#endif

/* stop using fastcall for Borland */
#if defined(__BORLANDC__) && defined(WIN32) && !defined(DEBUG)
# pragma option -p.
#endif

#if defined(MEM_PROFILE)
# define vim_realloc(ptr, size)  mem_realloc((ptr), (size))
#else
# define vim_realloc(ptr, size)  realloc((ptr), (size))
#endif

/*
 * The following macros stop displat/event loop nesting at the wrong time.
 */
#ifdef ALT_X_INPUT
# define ALT_INPUT_LOCK_OFF	suppress_alternate_input = FALSE
# define ALT_INPUT_LOCK_ON	suppress_alternate_input = TRUE
#endif

#ifdef FEAT_MBYTE
/* Maximum number of bytes in a multi-byte character.  It can be one 32-bit
 * character of up to 6 bytes, or one 16-bit character of up to three bytes
 * plus two following composing characters of three bytes each. */
# define MB_MAXBYTES	9

/*
 * Return byte length of character that starts with byte "b".
 * Returns 1 for a single-byte character.
 * MB_BYTE2LEN_CHECK() can be used to count a special key as one byte.
 * Don't call MB_BYTE2LEN(b) with b < 0 or b > 255!
 */
# define MB_BYTE2LEN(b)		mb_bytelen_tab[b]
# define MB_BYTE2LEN_CHECK(b)	(((b) < 0 || (b) > 255) ? 1 : mb_bytelen_tab[b])
#endif

#if defined(FEAT_MBYTE) || defined(FEAT_POSTSCRIPT)
/* properties used in enc_canon_table[] (first three mutually exclusive) */
# define ENC_8BIT	0x01
# define ENC_DBCS	0x02
# define ENC_UNICODE	0x04

# define ENC_ENDIAN_B	0x10	    /* Unicode: Big endian */
# define ENC_ENDIAN_L	0x20	    /* Unicode: Little endian */

# define ENC_2BYTE	0x40	    /* Unicode: UCS-2 */
# define ENC_4BYTE	0x80	    /* Unicode: UCS-4 */
# define ENC_2WORD	0x100	    /* Unicode: UTF-16 */

# define ENC_LATIN1	0x200	    /* Latin1 */
# define ENC_MACROMAN	0x400	    /* Mac Roman (not Macro Man! :-) */
#endif

#ifdef FEAT_MBYTE
# ifdef USE_ICONV
#  ifndef EILSEQ
#   define EILSEQ 123
#  endif
#  ifdef DYNAMIC_ICONV
/* On Win32 iconv.dll is dynamically loaded. */
#   define ICONV_ERRNO (*iconv_errno())
#   define ICONV_E2BIG  7
#   define ICONV_EINVAL 22
#   define ICONV_EILSEQ 42
#  else
#   define ICONV_ERRNO errno
#   define ICONV_E2BIG  E2BIG
#   define ICONV_EINVAL EINVAL
#   define ICONV_EILSEQ EILSEQ
#  endif
# endif

#endif

/* ISSYMLINK(mode) tests if a file is a symbolic link. */
#if (defined(S_IFMT) && defined(S_IFLNK)) || defined(S_ISLNK)
# define HAVE_ISSYMLINK
# if defined(S_IFMT) && defined(S_IFLNK)
#  define ISSYMLINK(mode) (((mode) & S_IFMT) == S_IFLNK)
# else
#  define ISSYMLINK(mode) S_ISLNK(mode)
# endif
#endif

#define SIGN_BYTE 1	    /* byte value used where sign is displayed;
			       attribute value is sign type */

#ifdef FEAT_NETBEANS_INTG
# define MULTISIGN_BYTE 2   /* byte value used where sign is displayed if
			       multiple signs exist on the line */
#endif

#if defined(FEAT_GUI) && defined(FEAT_XCLIPBOARD)
# ifdef FEAT_GUI_GTK
   /* Avoid using a global variable for the X display.  It's ugly
    * and is likely to cause trouble in multihead environments. */
#  define X_DISPLAY	((gui.in_use) ? gui_mch_get_display() : xterm_dpy)
# else
#  define X_DISPLAY	(gui.in_use ? gui.dpy : xterm_dpy)
# endif
#else
# ifdef FEAT_GUI
#  ifdef FEAT_GUI_GTK
#   define X_DISPLAY	((gui.in_use) ? gui_mch_get_display() : (Display *)NULL)
#  else
#   define X_DISPLAY	gui.dpy
#  endif
# else
#  define X_DISPLAY	xterm_dpy
# endif
#endif

#ifdef NBDEBUG /* Netbeans debugging. */
# include "nbdebug.h"
#else
# define nbdebug(a)
#endif

#ifdef IN_PERL_FILE
  /*
   * Avoid clashes between Perl and Vim namespace.
   */
# undef NORMAL
# undef STRLEN
# undef FF
# undef OP_DELETE
# undef OP_JOIN
# ifdef __BORLANDC__
#  define NOPROTO 1
# endif
  /* remove MAX and MIN, included by glib.h, redefined by sys/param.h */
# ifdef MAX
#  undef MAX
# endif
# ifdef MIN
#  undef MIN
# endif
  /* We use _() for gettext(), Perl uses it for function prototypes... */
# ifdef _
#  undef _
# endif
# ifdef DEBUG
#  undef DEBUG
# endif
# ifdef _DEBUG
#  undef _DEBUG
# endif
# ifdef instr
#  undef instr
# endif
  /* bool causes trouble on MACOS but is required on a few other systems */
# if defined(bool) && defined(MACOS)
#  undef bool
# endif

# ifdef __BORLANDC__
  /* Borland has the structure stati64 but not _stati64 */
#  define _stati64 stati64
# endif

# include <EXTERN.h>
# include <perl.h>
# include <XSUB.h>
#endif

#endif /* VIM__H */
