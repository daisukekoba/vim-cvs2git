/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * option.h: definition of global variables for settable options
 */

#ifndef EXTERN
# define EXTERN extern
# define INIT(x)
#else
# ifndef INIT
#  define INIT(x) x
# endif
#endif

/* default values for p_efm 'errorformat' */
#ifdef AMIGA
# define DFLT_EFM	"%f>%l:%c:%t:%n:%m,%f:%l: %t%*\\D%n: %m,%f %l %t%*\\D%n: %m,%*[^\"]\"%f\"%*\\D%l: %m,%f:%l:%m"
#else
# if defined MSDOS  ||	defined WIN32
#  define DFLT_EFM	"%f(%l) : %t%*\\D%n: %m,%*[^\"]\"%f\"%*\\D%l: %m,%f(%l) : %m,%*[^ ] %f %l: %m,%f:%l:%m"
# else
#  if defined(__EMX__)	/* put most common here (i.e. gcc format) at front */
#   define DFLT_EFM	"%f:%l:%m,%*[^\"]\"%f\"%*\\D%l: %m,\"%f\"%*\\D%l: %m,%f(%l:%c) : %m"
#  else
#   if defined(__QNX__)
#    define DFLT_EFM	"%f(%l):%*[^WE]%t%*\\D%n:%m"
#   else
#    ifdef VMS
#     define DFLT_EFM	"%A%p^,%C%%CC-%t-%m,%Cat line number %l in file %f"
#    else /* Unix, probably */
#     ifdef EBCDIC
#define DFLT_EFM	"%*[^ ] %*[^ ] %f:%l%*[ ]%m,%*[^\"]\"%f\"%*\\D%l: %m,\"%f\"%*\\D%l: %m,%f:%l:%m,\"%f\"\\, line %l%*\\D%c%*[^ ] %m,%D%*\\a[%*\\d]: Entering directory `%f',%X%*\\a[%*\\d]: Leaving directory `%f',%DMaking %*\\a in %f"
#     else
#define DFLT_EFM	"%*[^\"]\"%f\"%*\\D%l: %m,\"%f\"%*\\D%l: %m,%-G%f:%l: (Each undeclared identifier is reported only once,%-G%f:%l: for each function it appears in.),%f:%l:%m,\"%f\"\\, line %l%*\\D%c%*[^ ] %m,%D%*\\a[%*\\d]: Entering directory `%f',%X%*\\a[%*\\d]: Leaving directory `%f',%DMaking %*\\a in %f"
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#define DFLT_GREPFORMAT	"%f:%l:%m,%f:%l%m,%f  %l%m"

/* default values for b_p_ff 'fileformat' and p_ffs 'fileformats' */
#define FF_DOS		"dos"
#define FF_MAC		"mac"
#define FF_UNIX		"unix"

#ifdef USE_CRNL
# define DFLT_FF	"dos"
# define DFLT_FFS_VIM	"dos,unix"
# define DFLT_FFS_VI	"dos,unix"	/* also autodetect in compatible mode */
# define DFLT_TEXTAUTO	TRUE
#else
# ifdef USE_CR
#  define DFLT_FF	"mac"
#  define DFLT_FFS_VIM	"mac,unix,dos"
#  define DFLT_FFS_VI	"mac,unix,dos"
#  define DFLT_TEXTAUTO	TRUE
# else
#  define DFLT_FF	"unix"
#  define DFLT_FFS_VIM	"unix,dos"
#  define DFLT_FFS_VI	""
#  define DFLT_TEXTAUTO	FALSE
# endif
#endif


#ifdef FEAT_MBYTE
/* Possible values for 'encoding' */
# define ENC_UCSBOM	"ucs-bom"	/* check for BOM at start of file */

/* default value for 'encoding' */
# define ENC_DFLT	"latin1"
#endif

/* end-of-line style */
#define EOL_UNKNOWN	-1	/* not defined yet */
#define EOL_UNIX	0	/* NL */
#define EOL_DOS		1	/* CR NL */
#define EOL_MAC		2	/* CR */

/* Formatting options for p_fo 'formatoptions' */
#define FO_WRAP		't'
#define FO_WRAP_COMS	'c'
#define FO_RET_COMS	'r'
#define FO_OPEN_COMS	'o'
#define FO_Q_COMS	'q'
#define FO_Q_NUMBER	'n'
#define FO_Q_SECOND	'2'
#define FO_INS_VI	'v'
#define FO_INS_LONG	'l'
#define FO_INS_BLANK	'b'
#define FO_MULTIBYTE	'm'
#define FO_ONE_LETTER	'1'

#define DFLT_FO_VI	"vt"
#define DFLT_FO_VIM	"tcq"
#define FO_ALL		"tcroq2vlb1mn,"	/* for do_set() */

/* characters for the p_cpo option: */
#define CPO_ALTREAD	'a'	/* ":read" sets alternate file name */
#define CPO_ALTWRITE	'A'	/* ":write" sets alternate file name */
#define CPO_BAR		'b'	/* "\|" ends a mapping */
#define CPO_BSLASH	'B'	/* backslash in mapping is not special */
#define CPO_SEARCH	'c'
#define CPO_CONCAT	'C'	/* Don't concatenate sourced lines */
#define CPO_DOTTAG	'd'	/* "./tags" in 'tags' is in current dir */
#define CPO_DIGRAPH	'D'	/* No digraph after "r", "f", etc. */
#define CPO_EXECBUF	'e'
#define CPO_EMPTYREGION	'E'	/* operating on empty region is an error */
#define CPO_FNAMER	'f'	/* set file name for ":r file" */
#define CPO_FNAMEW	'F'	/* set file name for ":w file" */
#define CPO_JOINSP	'j'	/* only use two spaces for join after '.' */
#define CPO_ENDOFSENT	'J'	/* need two spaces to detect end of sentence */
#define CPO_KEYCODE	'k'	/* don't recognize raw key code in mappings */
#define CPO_KOFFSET	'K'	/* don't wait for key code in mappings */
#define CPO_LITERAL	'l'	/* take char after backslash in [] literal */
#define CPO_LISTWM	'L'	/* 'list' changes wrapmargin */
#define CPO_SHOWMATCH	'm'
#define CPO_NUMCOL	'n'	/* 'number' column also used for text */
#define CPO_LINEOFF	'o'
#define CPO_OVERNEW	'O'	/* silently overwrite new file */
#define CPO_LISP	'p'	/* 'lisp' indenting */
#define CPO_REDO	'r'
#define CPO_BUFOPT	's'
#define CPO_BUFOPTGLOB	'S'
#define CPO_TAGPAT	't'
#define CPO_UNDO	'u'	/* "u" undoes itself */
#define CPO_CW		'w'	/* "cw" only changes one blank */
#define CPO_FWRITE	'W'	/* "w!" doesn't overwrite readonly files */
#define CPO_ESC		'x'
#define CPO_YANK	'y'
#define CPO_DOLLAR	'$'
#define CPO_FILTER	'!'
#define CPO_MATCH	'%'
#define CPO_STAR	'*'	/* ":*" means ":@" */
#define CPO_SPECI	'<'	/* don't recognize <> in mappings */
#define CPO_DEFAULT	"aABceFs"
#define CPO_ALL		"aAbBcCdDeEfFjJkKlLmnoOprsStuwWxy$!%*<"

/* characters for p_ww option: */
#define WW_ALL		"bshl<>[],~"

/* characters for p_mouse option: */
#define MOUSE_NORMAL	'n'		/* use mouse in Normal mode */
#define MOUSE_VISUAL	'v'		/* use mouse in Visual/Select mode */
#define MOUSE_INSERT	'i'		/* use mouse in Insert mode */
#define MOUSE_COMMAND	'c'		/* use mouse in Command-line mode */
#define MOUSE_HELP	'h'		/* use mouse in help buffers */
#define MOUSE_RETURN	'r'		/* use mouse for hit-return message */
#define MOUSE_A		"nvich"		/* used for 'a' flag */
#define MOUSE_ALL	"anvichr"	/* all possible characters */
#define MOUSE_NONE	' '		/* don't use Visual selection */
#define MOUSE_NONEF	'x'		/* forced modeless selection */

/* characters for p_shm option: */
#define SHM_RO		'r'		/* readonly */
#define SHM_MOD		'm'		/* modified */
#define SHM_FILE	'f'		/* (file 1 of 2) */
#define SHM_LAST	'i'		/* last line incomplete */
#define SHM_TEXT	'x'		/* tx instead of textmode */
#define SHM_LINES	'l'		/* "L" instead of "lines" */
#define SHM_NEW		'n'		/* "[New]" instead of "[New file]" */
#define SHM_WRI		'w'		/* "[w]" instead of "written" */
#define SHM_A		"rmfixlnw"	/* represented by 'a' flag */
#define SHM_WRITE	'W'		/* don't use "written" at all */
#define SHM_TRUNC	't'		/* trunctate file messages */
#define SHM_TRUNCALL	'T'		/* trunctate all messages */
#define SHM_OVER	'o'		/* overwrite file messages */
#define SHM_OVERALL	'O'		/* overwrite more messages */
#define SHM_SEARCH	's'		/* no search hit bottom messages */
#define SHM_ATTENTION	'A'		/* no ATTENTION messages */
#define SHM_INTRO	'I'		/* intro messages */
#define SHM_ALL		"rmfixlnwaWtToOsAI" /* all possible flags for 'shm' */

/* characters for p_go: */
#define GO_ASEL		'a'		/* autoselect */
#define GO_BOT		'b'		/* use bottom scrollbar */
#define GO_FORG		'f'		/* start GUI in foreground */
#define GO_GREY		'g'		/* use grey menu items */
#define GO_ICON		'i'		/* use Vim icon */
#define GO_LEFT		'l'		/* use left scrollbar */
#define GO_MENUS	'm'		/* use menu bar */
#define GO_NOSYSMENU	'M'		/* don't source system menu */
#define GO_POINTER	'p'		/* pointer enter/leave callbacks */
#define GO_RIGHT	'r'		/* use right scrollbar */
#define GO_TEAROFF	't'		/* add tear-off menu items */
#define GO_TOOLBAR	'T'		/* add toolbar */
#define GO_FOOTER	'F'		/* add footer */
#define GO_VERTICAL	'v'		/* arrange dialog buttons vertically */
#define GO_ALL		"abfFgilmMprtTv" /* all possible flags for 'go' */

/* flags for 'comments' option */
#define COM_NEST	'n'		/* comments strings nest */
#define COM_BLANK	'b'		/* needs blank after string */
#define COM_START	's'		/* start of comment */
#define COM_MIDDLE	'm'		/* middle of comment */
#define COM_END		'e'		/* end of comment */
#define COM_AUTO_END	'x'		/* last char of end closes comment */
#define COM_FIRST	'f'		/* first line comment only */
#define COM_LEFT	'l'		/* left adjusted */
#define COM_RIGHT	'r'		/* right adjusted */
#define COM_NOBACK	'O'		/* don't use for "O" command */
#define COM_ALL		"nbsmexflrO"	/* all flags for 'comments' option */
#define COM_MAX_LEN	50		/* maximum length of a part */

/* flags for 'statusline' option */
#define STL_FILEPATH	'f'		/* path of file in buffer */
#define STL_FULLPATH	'F'		/* full path of file in buffer */
#define STL_FILENAME	't'		/* last part (tail) of file path */
#define STL_COLUMN	'c'		/* column og cursor*/
#define STL_VIRTCOL	'v'		/* virtual column */
#define STL_VIRTCOL_ALT	'V'		/* - with 'if different' display */
#define STL_LINE	'l'		/* line number of cursor */
#define STL_NUMLINES	'L'		/* number of lines in buffer */
#define STL_BUFNO	'n'		/* current buffer number */
#define STL_KEYMAP	'k'		/* 'keymap' when active */
#define STL_OFFSET	'o'		/* offset of character under cursor*/
#define STL_OFFSET_X	'O'		/* - in hexadecimal */
#define STL_BYTEVAL	'b'		/* byte value of character */
#define STL_BYTEVAL_X	'B'		/* - in hexadecimal */
#define STL_ROFLAG	'r'		/* readonly flag */
#define STL_ROFLAG_ALT	'R'		/* - other display */
#define STL_HELPFLAG	'h'		/* window is showing a help file */
#define STL_HELPFLAG_ALT 'H'		/* - other display */
#define STL_FILETYPE	'y'		/* 'filetype' */
#define STL_FILETYPE_ALT 'Y'		/* - other display */
#define STL_PREVIEWFLAG	'w'		/* window is showing the preview buf */
#define STL_PREVIEWFLAG_ALT 'W'		/* - other display */
#define STL_MODIFIED	'm'		/* modified flag */
#define STL_MODIFIED_ALT 'M'		/* - other display */
#define STL_PERCENTAGE	'p'		/* percentage through file */
#define STL_ALTPERCENT	'P'		/* percentage as TOP BOT ALL or NN% */
#define STL_ARGLISTSTAT	'a'		/* argument list status as (x of y) */
#define STL_VIM_EXPR	'{'		/* start of expression to substitute */
#define STL_MIDDLEMARK	'='		/* separation between left and right */
#define STL_TRUNCMARK	'<'		/* truncation mark if line is too long*/
#define STL_HIGHLIGHT	'*'		/* highlight from (User)1..9 or 0 */
#define STL_ALL		((char_u *) "fFtcvVlLknoObBrRhHmYyWwMpPa{")

/* flags used for parsed 'wildmode' */
#define WIM_FULL	1
#define WIM_LONGEST	2
#define WIM_LIST	4

/* arguments for can_bs() */
#define BS_INDENT	'i'	/* "Indent" */
#define BS_EOL		'o'	/* "eOl" */
#define BS_START	's'	/* "Start" */

/*
 * The following are actual variabables for the options
 */

#ifdef FEAT_RIGHTLEFT
EXTERN long	p_aleph;	/* 'aleph' */
#endif
EXTERN int	p_ar;		/* 'autoread' */
EXTERN int	p_aw;		/* 'autowrite' */
EXTERN int	p_awa;		/* 'autowriteall' */
EXTERN char_u	*p_bs;		/* 'backspace' */
EXTERN char_u	*p_bg;		/* 'background' */
EXTERN int	p_bk;		/* 'backup' */
EXTERN char_u	*p_bkc;		/* 'backupcopy' */
EXTERN char_u	*p_bdir;	/* 'backupdir' */
EXTERN char_u	*p_bex;		/* 'backupext' */
#ifdef FEAT_WILDIGN
EXTERN char_u	*p_bsk;		/* 'backupskip' */
#endif
#ifdef FEAT_BEVAL
EXTERN int	p_bdlay;	/* 'balloondelay' */
#endif
#ifdef FEAT_SUN_WORKSHOP
EXTERN int	p_beval;	/* 'ballooneval' */
#endif
EXTERN char_u	*p_bsdir;	/* 'browsedir' */
#ifdef MSDOS
EXTERN int	p_biosk;	/* 'bioskey' */
EXTERN int	p_consk;	/* 'conskey' */
#endif
#ifdef FEAT_LINEBREAK
EXTERN char_u	*p_breakat;	/* 'breakat' */
#endif
#ifdef FEAT_MBYTE
EXTERN char_u	*p_enc;		/* 'encoding' */
EXTERN int	p_deco;		/* 'delcombine' */
# ifdef FEAT_EVAL
EXTERN char_u	*p_ccv;		/* 'charconvert' */
# endif
#endif
#ifdef FEAT_CMDWIN
EXTERN char_u	*p_cedit;	/* 'cedit' */
EXTERN long	p_cwh;		/* 'cmdwinheight' */
#endif
#ifdef FEAT_CLIPBOARD
EXTERN char_u	*p_cb;		/* 'clipboard' */
EXTERN int	clip_unnamed INIT(= FALSE);
EXTERN int	clip_autoselect INIT(= FALSE);
EXTERN regprog_T *clip_exclude_prog INIT(= NULL);
#endif
EXTERN long	p_ch;		/* 'cmdheight' */
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
EXTERN int	p_confirm;	/* 'confirm' */
#endif
EXTERN int	p_cp;		/* 'compatible' */
EXTERN char_u	*p_cpo;		/* 'cpoptions' */
#ifdef FEAT_CSCOPE
EXTERN char_u	*p_csprg;	/* 'cscopeprg' */
EXTERN int	p_cst;		/* 'cscopetag' */
EXTERN long	p_csto;		/* 'cscopetagorder' */
EXTERN long	p_cspc;		/* 'cscopepathcomp' */
EXTERN int	p_csverbose;	/* 'cscopeverbose' */
#endif
EXTERN char_u	*p_debug;	/* 'debug' */
#ifdef FEAT_FIND_ID
EXTERN char_u	*p_def;		/* 'define' */
#endif
#ifdef FEAT_DIFF
EXTERN char_u	*p_dip;		/* 'diffopt' */
# ifdef FEAT_EVAL
EXTERN char_u	*p_dex;		/* 'diffexpr' */
# endif
#endif
#ifdef FEAT_INS_EXPAND
EXTERN char_u	*p_dict;	/* 'dictionary' */
#endif
#ifdef FEAT_DIGRAPHS
EXTERN int	p_dg;		/* 'digraph' */
#endif
EXTERN char_u	*p_dir;		/* 'directory' */
EXTERN char_u	*p_dy;		/* 'display' */
EXTERN unsigned	dy_flags;
#ifdef IN_OPTION_C
static char *(p_dy_values[]) = {"lastline", "uhex", NULL};
#endif
#define DY_LASTLINE		0x001
#define DY_UHEX			0x002
EXTERN int	p_ed;		/* 'edcompatible' */
#ifdef FEAT_VERTSPLIT
EXTERN char_u	*p_ead;		/* 'eadirection' */
#endif
EXTERN int	p_ea;		/* 'equalalways' */
EXTERN char_u	*p_ep;		/* 'equalprg' */
EXTERN int	p_eb;		/* 'errorbells' */
#ifdef FEAT_QUICKFIX
EXTERN char_u	*p_ef;		/* 'errorfile' */
EXTERN char_u	*p_efm;		/* 'errorformat' */
EXTERN char_u	*p_gefm;	/* 'grepformat' */
EXTERN char_u	*p_gp;		/* 'grepprg' */
#endif
#ifdef FEAT_AUTOCMD
EXTERN char_u	*p_ei;		/* 'eventignore' */
#endif
EXTERN int	p_ek;		/* 'esckeys' */
EXTERN int	p_exrc;		/* 'exrc' */
#ifdef FEAT_MBYTE
EXTERN char_u	*p_fencs;	/* 'fileencodings' */
#endif
EXTERN char_u	*p_ffs;		/* 'fileformats' */
#ifdef FEAT_FOLDING
EXTERN char_u	*p_fcl;		/* 'foldclose' */
EXTERN long	p_fdls;		/* 'foldlevelstart' */
EXTERN char_u	*p_fdo;		/* 'foldopen' */
EXTERN unsigned	fdo_flags;
# ifdef IN_OPTION_C
static char *(p_fdo_values[]) = {"all", "block", "hor", "mark", "percent",
					   "quickfix", "search", "tag", NULL};
# endif
# define FDO_ALL		0x001
# define FDO_BLOCK		0x002
# define FDO_HOR		0x004
# define FDO_MARK		0x008
# define FDO_PERCENT		0x010
# define FDO_QUICKFIX		0x020
# define FDO_SEARCH		0x040
# define FDO_TAG		0x080
#endif
EXTERN char_u	*p_fp;		/* 'formatprg' */
EXTERN int	p_gd;		/* 'gdefault' */
#ifdef FEAT_GUI
EXTERN char_u	*p_guifont;	/* 'guifont' */
# ifdef FEAT_XFONTSET
EXTERN char_u	*p_guifontset;	/* 'guifontset' */
# endif
# ifdef FEAT_MBYTE
EXTERN char_u	*p_guifontwide;	/* 'guifontwide' */
# endif
EXTERN int	p_guipty;	/* 'guipty' */
#endif
#if defined(FEAT_GUI_GTK) || defined(FEAT_GUI_X11)
EXTERN long	p_ghr;		/* 'guiheadroom' */
#endif
#ifdef CURSOR_SHAPE
EXTERN char_u	*p_guicursor;	/* 'guicursor' */
#endif
#ifdef FEAT_MOUSESHAPE
EXTERN char_u	*p_mouseshape;	/* 'mouseshape' */
#endif
#if defined(FEAT_GUI)
EXTERN char_u	*p_go;		/* 'guioptions' */
#endif
EXTERN char_u	*p_hf;		/* 'helpfile' */
#ifdef FEAT_WINDOWS
EXTERN long	p_hh;		/* 'helpheight' */
#endif
EXTERN int	p_hid;		/* 'hidden' */
/* Use P_HID to check if a buffer is to be hidden when it is no longer
 * visible in a window. */
#ifndef FEAT_QUICKFIX
# define P_HID(dummy) (p_hid || cmdmod.hide)
#else
# define P_HID(buf) (buf_hide(buf))
#endif
EXTERN char_u	*p_hl;		/* 'highlight' */
EXTERN int	p_hls;		/* 'hlsearch' */
EXTERN long	p_hi;		/* 'history' */
#ifdef FEAT_RIGHTLEFT
EXTERN int	p_hkmap;	/* 'hkmap' */
EXTERN int	p_hkmapp;	/* 'hkmapp' */
# ifdef FEAT_FKMAP
EXTERN int	p_fkmap;	/* 'fkmap' */
EXTERN int	p_altkeymap;	/* 'altkeymap' */
# endif
#endif
#ifdef FEAT_TITLE
EXTERN int	p_icon;		/* 'icon' */
EXTERN char_u	*p_iconstring;	/* 'iconstring' */
#endif
EXTERN int	p_ic;		/* 'ignorecase' */
EXTERN int	p_is;		/* 'incsearch' */
EXTERN int	p_im;		/* 'insertmode' */
EXTERN char_u	*p_isf;		/* 'isfname' */
EXTERN char_u	*p_isi;		/* 'isident' */
EXTERN char_u	*p_isp;		/* 'isprint' */
EXTERN int	p_js;		/* 'joinspaces' */
EXTERN char_u	*p_kp;		/* 'keywordprg' */
#ifdef FEAT_VISUAL
EXTERN char_u	*p_km;		/* 'keymodel' */
#endif
#ifdef FEAT_LANGMAP
EXTERN char_u	*p_langmap;	/* 'langmap'*/
#endif
#if defined(FEAT_MENU) && defined(FEAT_MULTI_LANG)
EXTERN char_u	*p_lm;		/* 'langmenu' */
#endif
#ifdef FEAT_GUI
EXTERN long	p_linespace;	/* 'linespace' */
#endif
#ifdef FEAT_WINDOWS
EXTERN long	p_ls;		/* 'laststatus' */
#endif
EXTERN char_u	*p_lcs;		/* 'listchars' */

EXTERN int	p_lz;		/* 'lazyredraw' */
EXTERN int	p_lpl;		/* 'loadplugins' */
EXTERN int	p_magic;	/* 'magic' */
#ifdef FEAT_QUICKFIX
EXTERN char_u	*p_mef;		/* 'makeef' */
EXTERN char_u	*p_mp;		/* 'makeprg' */
#endif
EXTERN long	p_mat;		/* 'matchtime' */
#ifdef FEAT_EVAL
EXTERN long	p_mfd;		/* 'maxfuncdepth' */
#endif
EXTERN long	p_mmd;		/* 'maxmapdepth' */
EXTERN long	p_mm;		/* 'maxmem' */
EXTERN long	p_mmt;		/* 'maxmemtot' */
#ifdef FEAT_MENU
EXTERN long	p_mis;		/* 'menuitems' */
#endif
EXTERN long	p_mls;		/* 'modelines' */
EXTERN char_u	*p_mouse;	/* 'mouse' */
#ifdef FEAT_GUI
EXTERN int	p_mousef;	/* 'mousefocus' */
EXTERN int	p_mh;		/* 'mousehide' */
#endif
EXTERN char_u	*p_mousem;	/* 'mousemodel' */
EXTERN long	p_mouset;	/* 'mousetime' */
EXTERN int	p_more;		/* 'more' */
EXTERN char_u	*p_para;	/* 'paragraphs' */
EXTERN int	p_paste;	/* 'paste' */
EXTERN char_u	*p_pt;		/* 'pastetoggle' */
#if defined(FEAT_EVAL) && defined(FEAT_DIFF)
EXTERN char_u	*p_pex;		/* 'patchexpr' */
#endif
EXTERN char_u	*p_pm;		/* 'patchmode' */
EXTERN char_u	*p_path;	/* 'path' */
#ifdef FEAT_SEARCHPATH
EXTERN char_u	*p_cdpath;	/* 'cdpath' */
#endif
EXTERN int	p_remap;	/* 'remap' */
EXTERN long	p_report;	/* 'report' */
#if defined(FEAT_WINDOWS) && defined(FEAT_QUICKFIX)
EXTERN long	p_pvh;		/* 'previewheight' */
#endif
#ifdef WIN32
EXTERN int	p_rs;		/* 'restorescreen' */
#endif
#ifdef FEAT_RIGHTLEFT
EXTERN int	p_ari;		/* 'allowrevins' */
EXTERN int	p_ri;		/* 'revins' */
#endif
#ifdef FEAT_CMDL_INFO
EXTERN int	p_ru;		/* 'ruler' */
#endif
#ifdef FEAT_STL_OPT
EXTERN char_u	*p_ruf;		/* 'rulerformat' */
#endif
EXTERN char_u	*p_rtp;		/* 'runtimepath' */
EXTERN long	p_sj;		/* 'scrolljump' */
EXTERN long	p_so;		/* 'scrolloff' */
#ifdef FEAT_SCROLLBIND
EXTERN char_u	*p_sbo;		/* 'scrollopt' */
#endif
EXTERN char_u	*p_sections;	/* 'sections' */
EXTERN int	p_secure;	/* 'secure' */
#ifdef FEAT_VISUAL
EXTERN char_u	*p_sel;		/* 'selection' */
EXTERN char_u	*p_slm;		/* 'selectmode' */
#endif
#ifdef FEAT_SESSION
EXTERN char_u	*p_ssop;	/* 'sessionoptions' */
EXTERN unsigned	ssop_flags;
# ifdef IN_OPTION_C
/* Also used for 'viewoptions'! */
static char *(p_ssop_values[]) = {"buffers", "winpos", "resize", "winsize",
    "localoptions", "options", "help", "blank", "globals", "slash", "unix",
    "sesdir", "curdir", "folds", "cursor", NULL};
# endif
# define SSOP_BUFFERS		0x001
# define SSOP_WINPOS		0x002
# define SSOP_RESIZE		0x004
# define SSOP_WINSIZE		0x008
# define SSOP_LOCALOPTIONS	0x010
# define SSOP_OPTIONS		0x020
# define SSOP_HELP		0x040
# define SSOP_BLANK		0x080
# define SSOP_GLOBALS		0x100
# define SSOP_SLASH		0x200
# define SSOP_UNIX		0x400
# define SSOP_SESDIR		0x800
# define SSOP_CURDIR		0x1000
# define SSOP_FOLDS		0x2000
# define SSOP_CURSOR		0x4000
#endif
EXTERN char_u	*p_sh;		/* 'shell' */
EXTERN char_u	*p_shcf;	/* 'shellcmdflag' */
#ifdef FEAT_QUICKFIX
EXTERN char_u	*p_sp;		/* 'shellpipe' */
#endif
EXTERN char_u	*p_shq;		/* 'shellquote' */
EXTERN char_u	*p_sxq;		/* 'shellxquote' */
EXTERN char_u	*p_srr;		/* 'shellredir' */
#ifdef AMIGA
EXTERN long	p_st;		/* 'shelltype' */
#endif
#ifdef BACKSLASH_IN_FILENAME
EXTERN int	p_ssl;		/* 'shellslash' */
#endif
#ifdef FEAT_STL_OPT
EXTERN char_u	*p_stl;		/* 'statusline' */
#endif
EXTERN int	p_sr;		/* 'shiftround' */
EXTERN char_u	*p_shm;		/* 'shortmess' */
#ifdef FEAT_LINEBREAK
EXTERN char_u	*p_sbr;		/* 'showbreak' */
#endif
#ifdef FEAT_CMDL_INFO
EXTERN int	p_sc;		/* 'showcmd' */
#endif
EXTERN int	p_sft;		/* 'showfulltag' */
EXTERN int	p_sm;		/* 'showmatch' */
EXTERN int	p_smd;		/* 'showmode' */
EXTERN long	p_ss;		/* 'sidescroll' */
EXTERN long	p_siso;		/* 'sidescrolloff' */
EXTERN int	p_scs;		/* 'smartcase' */
EXTERN int	p_sta;		/* 'smarttab' */
#ifdef FEAT_WINDOWS
EXTERN int	p_sb;		/* 'splitbelow' */
#endif
#ifdef FEAT_VERTSPLIT
EXTERN int	p_spr;		/* 'splitright' */
#endif
EXTERN int	p_sol;		/* 'startofline' */
EXTERN char_u	*p_su;		/* 'suffixes' */
EXTERN char_u	*p_sws;		/* 'swapsync' */
EXTERN char_u	*p_swb;		/* 'switchbuf' */
EXTERN int	p_tbs;		/* 'tagbsearch' */
EXTERN long	p_tl;		/* 'taglength' */
EXTERN int	p_tr;		/* 'tagrelative' */
EXTERN char_u	*p_tags;	/* 'tags' */
EXTERN int	p_tgst;		/* 'tagstack' */
#ifdef FEAT_MBYTE
EXTERN char_u	*p_tenc;	/* 'termencoding' */
#endif
EXTERN int	p_terse;	/* 'terse' */
EXTERN int	p_ta;		/* 'textauto' */
EXTERN int	p_to;		/* 'tildeop' */
EXTERN int	p_timeout;	/* 'timeout' */
EXTERN long	p_tm;		/* 'timeoutlen' */
#ifdef FEAT_TITLE
EXTERN int	p_title;	/* 'title' */
EXTERN long	p_titlelen;	/* 'titlelen' */
EXTERN char_u	*p_titleold;	/* 'titleold' */
EXTERN char_u	*p_titlestring;	/* 'titlestring' */
#endif
#ifdef FEAT_INS_EXPAND
EXTERN char_u	*p_tsr;		/* 'thesaurus' */
#endif
EXTERN int	p_ttimeout;	/* 'ttimeout' */
EXTERN long	p_ttm;		/* 'ttimeoutlen' */
EXTERN int	p_tbi;		/* 'ttybuiltin' */
EXTERN int	p_tf;		/* 'ttyfast' */
#if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_W32)
EXTERN char_u	*p_toolbar;	/* 'toolbar' */
#endif
EXTERN long	p_ttyscroll;	/* 'ttyscroll' */
#if defined(FEAT_MOUSE) && (defined(UNIX) || defined(VMS))
EXTERN char_u	*p_ttym;	/* 'ttymouse' */
EXTERN unsigned ttym_flags;
# ifdef IN_OPTION_C
static char *(p_ttym_values[]) = {"xterm", "xterm2", "dec", "netterm", "jsbterm", NULL};
# endif
# define TTYM_XTERM	0x01
# define TTYM_XTERM2	0x02
# define TTYM_DEC	0x04
# define TTYM_NETTERM	0x08
# define TTYM_JSBTERM	0x10
# define TTYM_PTERM	0x20
#endif
EXTERN long	p_ul;		/* 'undolevels' */
EXTERN long	p_uc;		/* 'updatecount' */
EXTERN long	p_ut;		/* 'updatetime' */
#if defined(FEAT_WINDOWS) || defined(FEAT_FOLDING)
EXTERN char_u	*p_fcs;		/* 'fillchar' */
#endif
#ifdef FEAT_VIMINFO
EXTERN char_u	*p_viminfo;	/* 'viminfo' */
#endif
#ifdef FEAT_SESSION
EXTERN char_u	*p_vdir;	/* 'viewdir' */
EXTERN char_u	*p_vop;		/* 'viewoptions' */
EXTERN unsigned	vop_flags;	/* uses SSOP_ flags */
#endif
EXTERN int	p_vb;		/* 'visualbell' */
#ifdef FEAT_VIRTUALEDIT
EXTERN char_u	*p_ve;		/* 'virtualedit' */
EXTERN unsigned ve_flags;
# ifdef IN_OPTION_C
static char *(p_ve_values[]) = {"block", "insert", "all", NULL};
# endif
# define VE_BLOCK	5	/* includes "all" */
# define VE_INSERT	6	/* includes "all" */
# define VE_ALL		4
#endif
EXTERN long	p_verbose;	/* 'verbose' */
EXTERN int	p_warn;		/* 'warn' */
#if defined(FEAT_GUI_MSWIN) || defined(FEAT_GUI_MOTIF) || defined(LINT) \
	|| defined (FEAT_GUI_GTK) || defined(FEAT_GUI_PHOTON)
#define FEAT_WAK
EXTERN char_u	*p_wak;		/* 'winaltkeys' */
#endif
#ifdef FEAT_WILDIGN
EXTERN char_u	*p_wig;		/* 'wildignore' */
#endif
EXTERN int	p_wiv;		/* 'weirdinvert' */
EXTERN char_u	*p_ww;		/* 'whichwrap' */
EXTERN long	p_wc;		/* 'wildchar' */
EXTERN long	p_wcm;		/* 'wildcharm' */
EXTERN char_u	*p_wim;		/* 'wildmode' */
#ifdef FEAT_WILDMENU
EXTERN int	p_wmnu;		/* 'wildmenu' */
#endif
#ifdef FEAT_WINDOWS
EXTERN long	p_wh;		/* 'winheight' */
EXTERN long	p_wmh;		/* 'winminheight' */
#endif
#ifdef FEAT_VERTSPLIT
EXTERN long	p_wmw;		/* 'winminwidth' */
EXTERN long	p_wiw;		/* 'winwidth' */
#endif
EXTERN int	p_ws;		/* 'wrapscan' */
EXTERN int	p_write;	/* 'write' */
EXTERN int	p_wa;		/* 'writeany' */
EXTERN int	p_wb;		/* 'writebackup' */
EXTERN long	p_wd;		/* 'writedelay' */
