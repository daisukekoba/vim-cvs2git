/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * This file contains various definitions of structures that are used by Vim
 */

/*
 * There is something wrong in the SAS compiler that makes typedefs not
 * valid in include files.  Has been fixed in version 6.58.
 */
#if defined(SASC) && SASC < 658
typedef long		linenr_t;
typedef unsigned	colnr_t;
typedef unsigned short	short_u;
#endif

/*
 * position in file or buffer
 */
typedef struct
{
    linenr_t	    lnum;	    /* line number */
    colnr_t	    col;	    /* column number */
} pos_t;

/*
 * This is here because regexp.h needs pos_t and below regprog_t is used.
 */
#include "regexp.h"

typedef struct window	    win_t;
typedef struct wininfo	    wininfo_t;
typedef struct frame	    frame_t;

/*
 * This is here because gui.h needs the pos_t and win_t, and win_t needs gui.h
 * for scrollbar_t.
 */
#ifdef FEAT_GUI
# include "gui.h"
#else
# ifdef FEAT_XCLIPBOARD
#  include <X11/Intrinsic.h>
# endif
# define guicolor_t int		    /* avoid error in prototypes */
#endif

/*
 * marks: positions in a file
 * (a normal mark is a lnum/col pair, the same as a file position)
 */

/* (Note: for EBCDIC there are more than 26, because there are gaps in the
 * alphabet coding.  To minimize changes to the code, I decided to just
 * increase the number of possible marks. */
#define NMARKS		('z' - 'a' + 1)	    /* max. # of named marks */
#define JUMPLISTSIZE	100	    /* max. # of marks in jump list */
#define TAGSTACKSIZE	20	    /* max. # of tags in tag stack */

typedef struct filemark
{
    pos_t	    mark;	    /* cursor position */
    int		    fnum;	    /* file number */
#ifdef FEAT_VIRTUALEDIT
    int		    coladd;	    /* offset for 'virtualedit' */
#endif
} fmark_t;

/*
 * The taggy struct is used to store the information about a :tag command.
 */
typedef struct taggy
{
    char_u	*tagname;	/* tag name */
    fmark_t	fmark;		/* cursor position BEFORE ":tag" */
    int		cur_match;	/* match number */
} taggy_t;

/*
 * Structure that contains all options that are local to a window.
 * Used twice in a window: for the current buffer and for all buffers.
 * Also used in wininfo_t.
 */
typedef struct
{
#ifdef FEAT_FOLDING
    int		wo_fdc;			/* 'foldcolumn' */
#define w_p_fdc w_onebuf_opt.wo_fdc
    int		wo_fen;			/* 'foldenable' */
#define w_p_fen w_onebuf_opt.wo_fen
    char_u	*wo_fde;		/* 'foldexpr' */
#define w_p_fde w_onebuf_opt.wo_fde
    char_u	*wo_fdi;		/* 'foldignore' */
#define w_p_fdi w_onebuf_opt.wo_fdi
    long	wo_fdl;			/* 'foldlevel' */
#define w_p_fdl w_onebuf_opt.wo_fdl
    char_u	*wo_fdm;		/* 'foldmethod' */
#define w_p_fdm w_onebuf_opt.wo_fdm
    char_u	*wo_fdt;		/* 'foldtext' */
#define w_p_fdt w_onebuf_opt.wo_fdt
    char_u	*wo_fmr;		/* 'foldmarker' */
#define w_p_fmr w_onebuf_opt.wo_fmr
#endif
#ifdef FEAT_LINEBREAK
    int		wo_lbr;			/* 'linebreak' */
#define w_p_lbr w_onebuf_opt.wo_lbr
#endif
    int		wo_list;		/* 'list' */
#define w_p_list w_onebuf_opt.wo_list
    int		wo_nu;			/* 'number' */
#define w_p_nu w_onebuf_opt.wo_nu
#ifdef FEAT_RIGHTLEFT
    int		wo_rl;			/* 'rightleft' */
#define w_p_rl w_onebuf_opt.wo_rl
#endif
    long	wo_scr;			/* 'scroll' */
#define w_p_scr w_onebuf_opt.wo_scr
#ifdef FEAT_SCROLLBIND
    int		wo_scb;			/* 'scrollbind' */
#define w_p_scb w_onebuf_opt.wo_scb
#endif
    int		wo_wrap;		/* 'wrap' */
#define w_p_wrap w_onebuf_opt.wo_wrap
} winopt_t;

/*
 * Window info stored with a buffer.
 *
 * Two types of info are kept for a buffer which are associated with a
 * specific window:
 * 1. Each window can have a different line number associated with a buffer.
 * 2. The window-local options for a buffer work in a similar way.
 * The window-info is kept in a list at b_wininfo.  It is kept in
 * most-recently-used order.
 */
struct wininfo
{
    wininfo_t	*wi_next;	/* next entry or NULL for last entry */
    wininfo_t	*wi_prev;	/* previous entry or NULL for first entry */
    win_t	*wi_win;	/* pointer to window that did set wi_lnum */
    pos_t	wi_fpos;	/* last cursor position in the file */
    int		wi_optset;	/* TRUE wne wi_opt has useful values */
    winopt_t	wi_opt;		/* local window options */
};

/*
 * stuctures used for undo
 */

typedef struct u_entry u_entry_t;
typedef struct u_header u_header_t;
struct u_entry
{
    u_entry_t	*ue_next;	/* pointer to next entry in list */
    linenr_t	ue_top;		/* number of line above undo block */
    linenr_t	ue_bot;		/* number of line below undo block */
    linenr_t	ue_lcount;	/* linecount when u_save called */
    char_u	**ue_array;	/* array of lines in undo block */
    long	ue_size;	/* number of lines in ue_array */
};

struct u_header
{
    u_header_t	*uh_next;	/* pointer to next header in list */
    u_header_t	*uh_prev;	/* pointer to previous header in list */
    u_entry_t	*uh_entry;	/* pointer to first entry */
    pos_t	uh_cursor;	/* cursor position before saving */
    int		uh_flags;	/* see below */
    pos_t	uh_namedm[NMARKS];	/* marks before undo/after redo */
};

/* values for uh_flags */
#define UH_CHANGED  0x01	/* b_changed flag before undo/after redo */
#define UH_EMPTYBUF 0x02	/* buffer was empty */

/*
 * stuctures used in undo.c
 */
#if SIZEOF_INT > 2
# define ALIGN_LONG	/* longword alignment and use filler byte */
# define ALIGN_SIZE (sizeof(long))
#else
# define ALIGN_SIZE (sizeof(short))
#endif

#define ALIGN_MASK (ALIGN_SIZE - 1)

typedef struct m_info minfo_t;

/*
 * stucture used to link chunks in one of the free chunk lists.
 */
struct m_info
{
#ifdef ALIGN_LONG
    long_u	m_size;		/* size of the chunk (including m_info) */
#else
    short_u	m_size;		/* size of the chunk (including m_info) */
#endif
    minfo_t	*m_next;	/* pointer to next free chunk in the list */
};

/*
 * structure used to link blocks in the list of allocated blocks.
 */
typedef struct m_block mblock_t;
struct m_block
{
    mblock_t	*mb_next;	/* pointer to next allocated block */
    size_t	mb_size;	/* total size of all chunks in this block */
    minfo_t	mb_info;	/* head of free chuck list for this block */
};

/*
 * Structure used for growing arrays.
 * This is used to store information that only grows, is deleted all at
 * once, and needs to be accessed by index.  See ga_clear() and ga_grow().
 */
typedef struct growarray
{
    int	    ga_len;		    /* current number of items used */
    int	    ga_room;		    /* number of unused items at the end */
    int	    ga_itemsize;	    /* sizeof one item */
    int	    ga_growsize;	    /* number of items to grow each time */
    void    *ga_data;		    /* pointer to the first item */
} garray_t;

/*
 * things used in memfile.c
 */

typedef struct block_hdr    bhdr_t;
typedef struct memfile	    memfile_t;
typedef long		    blocknr_t;

/*
 * for each (previously) used block in the memfile there is one block header.
 *
 * The block may be linked in the used list OR in the free list.
 * The used blocks are also kept in hash lists.
 *
 * The used list is a doubly linked list, most recently used block first.
 *	The blocks in the used list have a block of memory allocated.
 *	mf_used_count is the number of pages in the used list.
 * The hash lists are used to quickly find a block in the used list.
 * The free list is a single linked list, not sorted.
 *	The blocks in the free list have no block of memory allocated and
 *	the contents of the block in the file (if any) is irrelevant.
 */

struct block_hdr
{
    bhdr_t	*bh_next;	    /* next block_hdr in free or used list */
    bhdr_t	*bh_prev;	    /* previous block_hdr in used list */
    bhdr_t	*bh_hash_next;	    /* next block_hdr in hash list */
    bhdr_t	*bh_hash_prev;	    /* previous block_hdr in hash list */
    blocknr_t	bh_bnum;		/* block number */
    char_u	*bh_data;	    /* pointer to memory (for used block) */
    int		bh_page_count;	    /* number of pages in this block */

#define BH_DIRTY    1
#define BH_LOCKED   2
    char	bh_flags;	    /* BH_DIRTY or BH_LOCKED */
};

/*
 * when a block with a negative number is flushed to the file, it gets
 * a positive number. Because the reference to the block is still the negative
 * number, we remember the translation to the new positive number in the
 * double linked trans lists. The structure is the same as the hash lists.
 */
typedef struct nr_trans NR_TRANS;

struct nr_trans
{
    NR_TRANS	*nt_next;		/* next nr_trans in hash list */
    NR_TRANS	*nt_prev;		/* previous nr_trans in hash list */
    blocknr_t	nt_old_bnum;		/* old, negative, number */
    blocknr_t	nt_new_bnum;		/* new, positive, number */
};

/*
 * Command modifiers ":vertical", ":browse", ":confirm" and ":hide" set a flag.
 * This needs to be saved for recursive commands, put them in a structure for
 * easy manipulation.
 */
typedef struct
{
    int		hide;			/* TRUE when ":hide" was used */
# ifdef FEAT_BROWSE
    int		browse;			/* TRUE to invoke file dialog */
# endif
# ifdef FEAT_WINDOWS
    int		split;			/* flags for win_split() */
# endif
# if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
    int		confirm;		/* TRUE to invoke yes/no dialog */
# endif
} cmdmod_t;

/*
 * Simplistic hashing scheme to quickly locate the blocks in the used list.
 * 64 blocks are found directly (64 * 4K = 256K, most files are smaller).
 */
#define MEMHASHSIZE	64
#define MEMHASH(nr)	((nr) & (MEMHASHSIZE - 1))

struct memfile
{
    char_u	*mf_fname;		/* name of the file */
    char_u	*mf_ffname;		/* idem, full path */
    int		mf_fd;			/* file descriptor */
    bhdr_t	*mf_free_first;		/* first block_hdr in free list */
    bhdr_t	*mf_used_first;		/* mru block_hdr in used list */
    bhdr_t	*mf_used_last;		/* lru block_hdr in used list */
    unsigned	mf_used_count;		/* number of pages in used list */
    unsigned	mf_used_count_max;	/* maximum number of pages in memory */
    bhdr_t	*mf_hash[MEMHASHSIZE];	/* array of hash lists */
    NR_TRANS	*mf_trans[MEMHASHSIZE];	/* array of trans lists */
    blocknr_t	mf_blocknr_max;		/* highest positive block number + 1*/
    blocknr_t	mf_blocknr_min;		/* lowest negative block number - 1 */
    blocknr_t	mf_neg_count;		/* number of negative blocks numbers */
    blocknr_t	mf_infile_count;	/* number of pages in the file */
    unsigned	mf_page_size;		/* number of bytes in a page */
    int		mf_dirty;		/* TRUE if there are dirty blocks */
};

/*
 * things used in memline.c
 */
/*
 * When searching for a specific line, we remember what blocks in the tree
 * are the branches leading to that block. This is stored in ml_stack.  Each
 * entry is a pointer to info in a block (may be data block or pointer block)
 */
typedef struct info_pointer
{
    blocknr_t	ip_bnum;	/* block number */
    linenr_t	ip_low;		/* lowest lnum in this block */
    linenr_t	ip_high;	/* highest lnum in this block */
    int		ip_index;	/* index for block with current lnum */
} infoptr_t;	/* block/index pair */

#ifdef FEAT_BYTEOFF
typedef struct ml_chunksize
{
    int		mlcs_numlines;
    long	mlcs_totalsize;
} chunksize_t;

 /* Flags when calling ml_updatechunk() */

#define ML_CHNK_ADDLINE 1
#define ML_CHNK_DELLINE 2
#define ML_CHNK_UPDLINE 3
#endif

/*
 * the memline structure holds all the information about a memline
 */
typedef struct memline
{
    linenr_t	ml_line_count;	/* number of lines in the buffer */

    memfile_t	*ml_mfp;	/* pointer to associated memfile */

#define ML_EMPTY	1	/* empty buffer */
#define ML_LINE_DIRTY	2	/* cached line was changed and allocated */
#define ML_LOCKED_DIRTY	4	/* ml_locked was changed */
#define ML_LOCKED_POS	8	/* ml_locked needs positive block number */
    int		ml_flags;

    infoptr_t	*ml_stack;	/* stack of pointer blocks (array of IPTRs) */
    int		ml_stack_top;	/* current top if ml_stack */
    int		ml_stack_size;	/* total number of entries in ml_stack */

    linenr_t	ml_line_lnum;	/* line number of cached line, 0 if not valid */
    char_u	*ml_line_ptr;	/* pointer to cached line */

    bhdr_t	*ml_locked;	/* block used by last ml_get */
    linenr_t	ml_locked_low;	/* first line in ml_locked */
    linenr_t	ml_locked_high;	/* last line in ml_locked */
    int		ml_locked_lineadd;  /* number of lines inserted in ml_locked */
#ifdef FEAT_BYTEOFF
    chunksize_t *ml_chunksize;
    int		ml_numchunks;
    int		ml_usedchunks;
#endif
} memline_t;

#if defined(FEAT_SIGNS) || defined(PROTO)
typedef struct signlist signlist_t;

struct signlist
{
    int		id;		/* unique identifier for each sign */
    int		lineno;		/* line number which has this sign */
    int		type;		/* type of sign (index into signtab) */
    signlist_t *next;		/* next signlist entry */
};
#endif

#ifdef FEAT_EVAL
/*
 * For conditional commands a stack is kept of nested conditionals.
 * When cs_idx < 0, there is no conditional command.
 */
# define CSTACK_LEN	50

struct condstack
{
    char	cs_flags[CSTACK_LEN];	/* CSF_ flags */
    int		cs_line[CSTACK_LEN];	/* line number of ":while" line */
    int		cs_idx;			/* current entry, or -1 if none */
    int		cs_whilelevel;		/* number of nested ":while"s */
    char	cs_had_while;		/* just found ":while" */
    char	cs_had_continue;	/* just found ":continue" */
    char	cs_had_endwhile;	/* just found ":endwhile" */
};

# define CSF_TRUE	1	/* condition was TRUE */
# define CSF_ACTIVE	2	/* current state is active */
# define CSF_WHILE	4	/* is a ":while" */
#endif

#ifdef FEAT_SYN_HL
/* struct passed to in_id_list() */
struct sp_syn
{
    int		inc_tag;	/* ":syn include" unique tag */
    short	id;		/* highlight group ID of item */
};

/*
 * Each keyword has one keyentry, which is linked in a hash list.
 */
typedef struct keyentry keyentry_t;

struct keyentry
{
    keyentry_t	*next;		/* next keyword in the hash list */
    struct sp_syn k_syn;	/* struct passed to in_id_list() */
    short	*next_list;	/* ID list for next match (if non-zero) */
    short	flags;		/* see syntax.c */
    char_u	keyword[1];	/* actually longer */
};

/*
 * Struct used to store one state of the state stack.
 */
typedef struct buf_state
{
    int		    bs_idx;	 /* index of pattern */
    int		    bs_flags;	 /* flags for pattern */
    reg_extmatch_t *bs_extmatch; /* external matches from start pattern */
} bufstate_t;

/*
 * syn_state contains the syntax state stack for the start of one line.
 * Used by b_sst_array[].
 */
typedef struct syn_state synstate_t;

struct syn_state
{
    synstate_t	*sst_next;	/* next entry in used or free list */
    linenr_t	sst_lnum;	/* line number for this state */
    union
    {
	bufstate_t	sst_stack[SST_FIX_STATES]; /* short state stack */
	garray_t	sst_ga;	/* growarray for long state stack */
    } sst_union;
    int		sst_next_flags;	/* flags for sst_next_list */
    short	*sst_next_list;	/* "nextgroup" list in this state
				 * (this is a copy, don't free it! */
    short	sst_stacksize;	/* number of states on the stack */
    disptick_t	sst_tick;	/* tick when last displayed */
    linenr_t	sst_change_lnum;/* when non-zero, change in this line
				 * may have made the state invalid */
};
#endif /* FEAT_SYN_HL */

/*
 * Structure shared between syntax.c, screen.c and gui_x11.c.
 */
typedef struct attr_entry
{
    short	    ae_attr;		/* HL_BOLD, etc. */
    union
    {
	struct
	{
	    char_u	    *start;	/* start escape sequence */
	    char_u	    *stop;	/* stop escape sequence */
	} term;
	struct
	{
	    char_u	    fg_color;	/* foreground color number */
	    char_u	    bg_color;	/* background color number */
	} cterm;
# ifdef FEAT_GUI
	struct
	{
	    guicolor_t	    fg_color;	/* foreground color handle */
	    guicolor_t	    bg_color;	/* background color handle */
	    GuiFont	    font;	/* font handle */
#  ifdef FEAT_XFONTSET
	    GuiFontset	    fontset;	/* fontset handle */
#  endif
#  ifdef FEAT_SIGNS
	    int		    sign;	/* breakpoint and error markers */
#  endif
	} gui;
# endif
    } ae_u;
} attrentry_t;

/*
 * Structure used for mappings and abbreviations.
 */
typedef struct mapblock mapblock_t;
struct mapblock
{
    mapblock_t	*m_next;	/* next mapblock in list */
    char_u	*m_keys;	/* mapped from */
    int		m_keylen;	/* strlen(m_keys) */
    char_u	*m_str;		/* mapped to */
    int		m_mode;		/* valid mode */
    int		m_noremap;	/* if non-zero no re-mapping for m_str */
};

/*
 * buffer: structure that holds information about one file
 *
 * Several windows can share a single Buffer
 * A buffer is unallocated if there is no memfile for it.
 * A buffer is new if the associated file has never been loaded yet.
 */

typedef struct file_buffer buf_t;

struct file_buffer
{
    memline_t	b_ml;		/* associated memline (also contains line
				   count) */

    buf_t	*b_next;	/* links in list of buffers */
    buf_t	*b_prev;

    int		b_nwindows;	/* nr of windows open on this buffer */

    int		b_flags;	/* various BF_ flags */

    /*
     * b_ffname has the full path of the file (NULL for no name).
     * b_sfname is the name as the user typed it (or NULL).
     * b_fname is the same as b_sfname, unless ":cd" has been done,
     *		then it is the same as b_ffname (NULL for no name).
     */
    char_u	*b_ffname;	/* full path file name */
    char_u	*b_sfname;	/* short file name */
    char_u	*b_fname;	/* current file name */

#ifdef UNIX
    int		b_dev;		/* device number (-1 if not set) */
    ino_t	b_ino;		/* inode number */
#endif
#ifdef macintosh
    FSSpec	b_FSSpec;	/* MacOS File Identification */
#endif
#ifdef FEAT_SNIFF
    int		b_sniff;	/* file was loaded through Sniff */
#endif

    int		b_fnum;		/* file number for this file. */

    int		b_changed;	/* 'modified': Set to TRUE if something in the
				   file has been changed and not written out. */
    int		b_changedtick;	/* incremented for each change, also for undo */

    /*
     * Changes to a buffer require updating of the display.  To minimize the
     * work, remember changes made and update everything at once.
     */
    int		b_mod_set;	/* TRUE when there are changes since the last
				   time the display was updated */
    linenr_t	b_mod_top;	/* topmost lnum that was changed */
    linenr_t	b_mod_bot;	/* lnum below last changed line, AFTER the
				   change */
    long	b_mod_xlines;	/* number of extra buffer lines inserted;
				   negative when lines were deleted */

    wininfo_t	*b_wininfo;	/* list of last used info for each window */

    long	b_mtime;	/* last change time of original file */
    long	b_mtime_read;	/* last change time when reading */

    pos_t	b_namedm[NMARKS]; /* current named marks (mark.c) */
#ifdef FEAT_VIRTUALEDIT
    int		b_namedmc[NMARKS]; /* idem, offset for 'virtualedit' */
#endif

#ifdef FEAT_VISUAL
    /* These variables are set when VIsual_active becomes FALSE */
    pos_t	b_visual_start;	/* start pos of last VIsual */
    pos_t	b_visual_end;	/* end position of last VIsual */
    int		b_visual_mode;	/* VIsual_mode of last VIsual */
# ifdef FEAT_VIRTUALEDIT
    int		b_visual_start_coladd;
    int		b_visual_end_coladd;
# endif
#endif

    pos_t	b_last_cursor;	/* cursor position when last unloading this
				   buffer */

    /*
     * Character table, only used in charset.c for 'iskeyword'
     */
    char	b_chartab[256];

#ifdef FEAT_LOCALMAP
    /* Table used for mappings local to a buffer. */
    mapblock_t	*(b_maphash[256]);

    /* First abbreviation local to a buffer. */
    mapblock_t	*b_first_abbr;
#endif
#ifdef FEAT_USR_CMDS
    /* User commands local to the buffer. */
    garray_t	b_ucmds;
#endif
    /*
     * start and end of an operator, also used for '[ and ']
     */
    pos_t	b_op_start;
    pos_t	b_op_end;

#ifdef FEAT_VIMINFO
    int		b_marks_read;	/* Have we read viminfo marks yet? */
#endif

    /*
     * The following only used in undo.c.
     */
    u_header_t	*b_u_oldhead;	/* pointer to oldest header */
    u_header_t	*b_u_newhead;	/* pointer to newest header */
    u_header_t	*b_u_curhead;	/* pointer to current header */
    int		b_u_numhead;	/* current number of headers */
    int		b_u_synced;	/* entry lists are synced */

    /*
     * variables for "U" command in undo.c
     */
    char_u	*b_u_line_ptr;	/* saved line for "U" command */
    linenr_t	b_u_line_lnum;	/* line number of line in u_line */
    colnr_t	b_u_line_colnr;	/* optional column number */

    /*
     * The following only used in undo.c
     */
    mblock_t	b_block_head;	/* head of allocated memory block list */
    minfo_t	*b_m_search;	/* pointer to chunk before previously
				   allocated/freed chunk */
    mblock_t	*b_mb_current;	/* block where m_search points in */
#ifdef FEAT_INS_EXPAND
    int		b_scanned;	/* ^N/^P have scanned this buffer */
#endif

    /*
     * Options local to a buffer.
     * They are here because their value depends on the type of file
     * or contents of the file being edited.
     */
    int		b_p_initialized;	/* set when options initialized */

    int		b_p_ai;		/* 'autoindent' */
    int		b_p_ai_nopaste;	/* b_p_ai saved for paste mode */
    int		b_p_bin;	/* 'binary' */
#if defined(FEAT_QUICKFIX)
    char_u	*b_p_bt;	/* 'buftype' */
#endif
#ifdef FEAT_CINDENT
    int		b_p_cin;	/* 'cindent' */
    char_u	*b_p_cino;	/* 'cinoptions' */
    char_u	*b_p_cink;	/* 'cinkeys' */
#endif
#if defined(FEAT_CINDENT) || defined(FEAT_SMARTINDENT)
    char_u	*b_p_cinw;	/* 'cinwords' */
#endif
#ifdef FEAT_COMMENTS
    char_u	*b_p_com;	/* 'comments' */
#endif
#ifdef FEAT_INS_EXPAND
    char_u	*b_p_cpt;	/* 'complete' */
#endif
    int		b_p_eol;	/* 'endofline' */
    int		b_p_et;		/* 'expandtab' */
    int		b_p_et_nobin;	/* b_p_et saved for binary mode */
#ifdef FEAT_MBYTE
    char_u	*b_p_fcc;	/* 'filecharcode' */
#endif
    char_u	*b_p_ff;	/* 'fileformat' */
#ifdef FEAT_AUTOCMD
    char_u	*b_p_ft;	/* 'filetype' */
#endif
    char_u	*b_p_fo;	/* 'formatoptions' */
    int		b_p_inf;	/* 'infercase' */
    char_u	*b_p_isk;	/* 'iskeyword' */
#ifdef FEAT_FIND_ID
    char_u	*b_p_inc;	/* 'include' */
# ifdef FEAT_EVAL
    char_u	*b_p_inex;	/* 'includeexpr' */
# endif
#endif
#if defined(FEAT_CINDENT) && defined(FEAT_EVAL)
    char_u	*b_p_inde;	/* 'indentexpr' */
    char_u	*b_p_indk;	/* 'indentkeys' */
#endif
#ifdef FEAT_CRYPT
    char_u	*b_p_key;	/* 'key' */
#endif
#ifdef FEAT_LISP
    int		b_p_lisp;	/* 'lisp' */
#endif
    char_u	*b_p_mps;	/* 'matchpairs' */
    int		b_p_ml;		/* 'modeline' */
    int		b_p_ml_nobin;	/* b_p_ml saved for binary mode */
    char_u	*b_p_nf;	/* 'nrformats' */
#ifdef FEAT_OSFILETYPE
    char_u	*b_p_oft;	/* 'osfiletype' */
#endif
    int		b_p_ro;		/* 'readonly' */
    long	b_p_sw;		/* 'shiftwidth' */
#ifndef SHORT_FNAME
    int		b_p_sn;		/* 'shortname' */
#endif
#ifdef FEAT_SMARTINDENT
    int		b_p_si;		/* 'smartindent' */
#endif
    long	b_p_sts;	/* 'softtabstop' */
    long	b_p_sts_nopaste; /* b_p_sts saved for paste mode */
#ifdef FEAT_SEARCHPATH
    char_u	*b_p_sua;	/* 'suffixesadd' */
#endif
    int		b_p_swf;	/* 'swapfile' */
#ifdef FEAT_SYN_HL
    char_u	*b_p_syn;	/* 'syntax' */
#endif
    long	b_p_ts;		/* 'tabstop' */
    int		b_p_tx;		/* 'textmode' */
    long	b_p_tw;		/* 'textwidth' */
    long	b_p_tw_nobin;	/* b_p_tw saved for binary mode */
    long	b_p_tw_nopaste;	/* b_p_tw saved for paste mode */
    long	b_p_wm;		/* 'wrapmargin' */
    long	b_p_wm_nobin;	/* b_p_wm saved for binary mode */
    long	b_p_wm_nopaste;	/* b_p_wm saved for paste mode */
#ifdef FEAT_KEYMAP
    char_u	*b_p_keymap;	/* 'keymap' */
#endif

    /* end of buffer options */

    int		b_start_ffc;	/* first char of 'ff' when edit started */
#ifdef FEAT_MBYTE
    char_u	*b_start_fcc;	/* value of 'filecharcode' when edit started */
#endif

#ifdef FEAT_EVAL
    garray_t	b_vars;		/* internal variables, local to buffer */
#endif

    /* When a buffer is created, it starts without a swap file.  b_may_swap is
     * then set to indicate that a swap file may be opened later.  It is reset
     * if a swap file could not be opened.
     */
    int		b_may_swap;
    int		b_did_warn;	/* Set to 1 if user has been warned on first
				   change of a read-only file */
    int		b_help;		/* buffer for help file */

#ifndef SHORT_FNAME
    int		b_shortname;	/* this file has an 8.3 file name */
#endif

#ifdef FEAT_PERL
    void	*perl_private;
#endif

#ifdef FEAT_PYTHON
    void	*python_ref;	/* The Python value referring to this buffer */
#endif

#ifdef FEAT_TCL
    void	*tcl_ref;
#endif

#ifdef FEAT_RUBY
    void	*ruby_ref;
#endif

#ifdef FEAT_SYN_HL
    keyentry_t	**b_keywtab;		/* syntax keywords hash table */
    keyentry_t	**b_keywtab_ic;		/* idem, ignore case */
    int		b_syn_ic;		/* ignore case for :syn cmds */
    garray_t	b_syn_patterns;		/* table for syntax patterns */
    garray_t	b_syn_clusters;		/* table for syntax clusters */
    int		b_syn_sync_flags;	/* flags about how to sync */
    short	b_syn_sync_id;		/* group to sync on */
    long	b_syn_sync_minlines;	/* minimal sync lines offset */
    long	b_syn_sync_maxlines;	/* maximal sync lines offset */
    char_u	*b_syn_linecont_pat;	/* line continuation pattern */
    regprog_t	*b_syn_linecont_prog;	/* line continuation program */
    int		b_syn_linecont_ic;	/* ignore-case flag for above */
    int		b_syn_topgrp;		/* for ":syntax include" */
# ifdef FEAT_FOLDING
    int		b_syn_folditems;	/* number of patterns with the HL_FOLD
					   flag set */
# endif
/*
 * b_sst_array[] contains the state stack for a number of lines, for the start
 * of that line (col == 0).  This avoids having to recompute the syntax state
 * too often.
 * b_sst_array[] is allocated to hold the state for all displayed lines, and
 * states for 1 out of about 20 other lines.
 * b_sst_array		pointer to an array of synstate_t
 * b_sst_len		number of entries in b_sst_array[]
 * b_sst_first		pointer to first used entry in b_sst_array[] or NULL
 * b_sst_firstfree	pointer to first free entry in b_sst_array[] or NULL
 * b_sst_freecount	number of free entries in b_sst_array[]
 * b_sst_check_lnum	entries after this lnum need to be checked for
 *			validity (MAXLNUM means no check needed)
 */
    synstate_t	*b_sst_array;
    int		b_sst_len;
    synstate_t	*b_sst_first;
    synstate_t	*b_sst_firstfree;
    int		b_sst_freecount;
    linenr_t	b_sst_check_lnum;
    short_u	b_sst_lasttick;	/* last display tick */
#endif /* FEAT_SYN_HL */

#ifdef FEAT_SIGNS
    signlist_t	*b_signlist;	/* list of signs to draw */
#endif

};

/*
 * Structure to cache info for displayed lines in w_lines[].
 * Each logical line has one entry.
 * The entry tells how the logical line is currently displayed in the window.
 * This is updated when displaying the window.
 * When the display is changed (e.g., when clearing the screen) w_lines_valid
 * is changed to exclude invalid entries.
 * When making changes to the buffer, wl_valid is reset to indicate wl_size
 * may not reflect what is actually in the buffer.  When wl_valid is FALSE,
 * the entries can only be used to count the number of displayed lines used.
 * wl_lnum and wl_lastlnum are invalid too.
 */
typedef struct w_line
{
    linenr_t	wl_lnum;	/* buffer line number for logical line */
    short_u	wl_size;	/* height in screen lines */
    char	wl_valid;	/* TRUE values are valid for text in buffer */
#ifdef FEAT_FOLDING
    char	wl_folded;	/* TRUE when this is a range of folded lines */
    linenr_t	wl_lastlnum;	/* last buffer line number for logical line */
#endif
} wline_t;

/*
 * Windows are kept in a tree of frames.  Each frame has a column (FR_COL)
 * or row (FR_ROW) layout or is a leaf, which has a window.
 */
struct frame
{
    char	fr_layout;	/* FR_LEAF, FR_COL or FR_ROW */
#ifdef FEAT_VERTSPLIT
    int		fr_width;
#endif
    int		fr_height;
    frame_t	*fr_parent;	/* containing frame or NULL */
    frame_t	*fr_next;	/* frame right or below in same parent, NULL
				   for first */
    frame_t	*fr_prev;	/* frame left or above in same parent, NULL
				   for last */
    /* fr_child and fr_win are mutually exclusive */
    frame_t	*fr_child;	/* first contained frame */
    win_t	*fr_win;	/* window that fills this frame */
};

#define FR_LEAF	0	/* frame is a leaf */
#define FR_ROW	1	/* frame with a row of windows */
#define FR_COL	2	/* frame with a column of windows */

/*
 * Structure which contains all information that belongs to a window
 *
 * All row numbers are relative to the start of the window, except w_winrow.
 */
struct window
{
    buf_t	*w_buffer;	    /* buffer we are a window into (used
				       often, keep it the first item!) */

    win_t	*w_prev;	    /* link to previous window */
    win_t	*w_next;	    /* link to next window */

    frame_t	*w_frame;	    /* frame containing this window */

    pos_t	w_cursor;	    /* cursor position in buffer */

    colnr_t	w_curswant;	    /* The column we'd like to be at.  This is
				       used to try to stay in the same column
				       for up/down cursor motions. */

    int		w_set_curswant;	    /* If set, then update w_curswant the next
				       time through cursupdate() to the
				       current virtual column */

#ifdef FEAT_VISUAL
    /*
     * the next six are used to update the visual part
     */
    char	w_old_visual_mode;  /* last known VIsual_mode */
    linenr_t	w_old_cursor_lnum;  /* last known end of visual part */
    colnr_t	w_old_cursor_fcol;  /* first column for block visual part */
    colnr_t	w_old_cursor_lcol;  /* last column for block visual part */
    linenr_t	w_old_visual_lnum;  /* last known start of visual part */
    colnr_t	w_old_curswant;	    /* last known value of Curswant */
#endif

    /*
     * The next three specify the offsets for displaying the buffer:
     */
    linenr_t	w_topline;	    /* buffer line number of the line at the
				       top of the window */
    colnr_t	w_leftcol;	    /* window column number of the left most
				       character in the window; used when
				       'wrap' is off */
    colnr_t	w_skipcol;	    /* starting column when a single line
				       doesn't fit in the window */

    /*
     * Layout of the window in the screen.
     * May need to add "msg_scrolled" to "w_winrow" in rare situations.
     */
#ifdef FEAT_WINDOWS
    int		w_winrow;	    /* first row of window in screen */
#endif
    int		w_height;	    /* number of rows in window, excluding
				       status/command line(s) */
#ifdef FEAT_WINDOWS
    int		w_status_height;    /* number of status lines (0 or 1) */
#endif
#ifdef FEAT_VERTSPLIT
    int		w_wincol;	    /* Leftmost column of window in screen.
				       use W_WINCOL() */
    int		w_width;	    /* Width of window, excluding separation.
				       use W_WIDTH() */
    int		w_vsep_width;	    /* Number of separator columns (0 or 1).
				       use W_VSEP_WIDTH() */
#endif

    /*
     * === start of cached values ====
     */
    /*
     * Recomputing is minimized by storing the result of computations.
     * Use functions in screen.c to check if they are valid and to update.
     * w_valid is a bitfield of flags, which indicate if specific values are
     * valid or need to be recomputed.	See screen.c for values.
     */
    int		w_valid;
    pos_t	w_valid_cursor;	    /* last known position of w_cursor, used
				       to adjust w_valid */
    colnr_t	w_valid_leftcol;    /* last known w_leftcol */
#ifdef FEAT_VIRTUALEDIT
    int		w_valid_coladd;	    /* last known w_coladd */
#endif

    /*
     * w_cline_height is the number of physical lines taken by the buffer line
     * that the cursor is on.  We use this to avoid extra calls to plines().
     */
    int		w_cline_height;	    /* current size of cursor line */
#ifdef FEAT_FOLDING
    int		w_cline_folded;	    /* cursor line is folded */
#endif

    int		w_cline_row;	    /* starting row of the cursor line */

    colnr_t	w_virtcol;	    /* column number of the cursor in the
				       buffer line, as opposed to the column
				       number we're at on the screen.  This
				       makes a difference on lines which span
				       more than one screen line or when
				       w_leftcol is non-zero */

    /*
     * w_wrow and w_wcol specify the cursor position in the window.
     * This is related to positions in the window, not in the display or
     * buffer, thus w_wrow is relative to w_winrow.
     */
    int		w_wrow, w_wcol;	    /* cursor position in window */
#ifdef FEAT_VIRTUALEDIT
    int		w_coladd;	    /* offset for 'virtualedit' */
#endif

    linenr_t	w_botline;	    /* number of the line below the bottom of
				       the screen */
    int		w_empty_rows;	    /* number of ~ rows in window */

    /*
     * Info about the lines currently in the window is remembered to avoid
     * recomputing it every time.  The allocated size of w_lines[] is Rows.
     * Only the w_lines_valid entries are actually valid.
     * When the display is up-to-date w_lines[0].wl_lnum is equal to w_topline
     * and w_lines[w_lines_valid - 1].wl_lnum is equal to w_botline.
     * Between changing text and updating the display w_lines[] represents
     * what is currently displayed.  wl_valid is reset to indicated this.
     * This is used for efficient redrawing.
     */
    int		w_lines_valid;	    /* number of valid entries */
    wline_t	*w_lines;

#ifdef FEAT_FOLDING
    garray_t	w_folds;	    /* array of nested folds */
    char	w_fold_manual;	    /* when TRUE: some folds are opened/closed
				       manually */
    char	w_foldinvalid;	    /* when TRUE: folding needs to be
				       recomputed */
#endif

    /*
     * === end of cached values ===
     */

    int		w_redr_type;	    /* type of redraw to be performed on win */
    int		w_upd_rows;	    /* number of window lines to update when
				       w_redr_type is REDRAW_TOP */
    linenr_t	w_redraw_top;	    /* when != 0: first line needing redraw */
    linenr_t	w_redraw_bot;	    /* when != 0: last line needing redraw */
#ifdef FEAT_WINDOWS
    int		w_redr_status;	    /* if TRUE status line must be redrawn */
#endif

    /* remember what is shown in the ruler for this window (if 'ruler' set) */
    pos_t	w_ru_cursor;	    /* cursor position shown in ruler */
    colnr_t	w_ru_virtcol;	    /* virtcol shown in ruler */
#ifdef FEAT_VIRTUALEDIT
    int		w_ru_coladd;	    /* coladd used in ruler */
#endif
    linenr_t	w_ru_topline;	    /* topline shown in ruler */
    char	w_ru_empty;	    /* TRUE if ruler shows 0-1 (empty line) */

    int		w_alt_fnum;	    /* alternate file (for # and CTRL-^) */

    int		w_arg_idx;	    /* current index in argument list (can be
				       out of range!) */
    int		w_arg_idx_invalid;  /* editing another file then w_arg_idx */

    char_u	*w_localdir;	    /* absolute path of local directory or
				       NULL */
    /*
     * Options local to a window.
     * They are local because they influence the layout of the window or
     * depend on the window layout.
     * There are two values: w_onebuf_opt is local to the buffer currently in
     * this window, w_allbuf_opt is for all buffers in this window.
     */
    winopt_t	w_onebuf_opt;
    winopt_t	w_allbuf_opt;

    /* transfor a pointer to a "onebuf" option to a "allbuf" option */
#define GLOBAL_WO(p)	((char *)p + sizeof(winopt_t))

#ifdef FEAT_SCROLLBIND
    long	w_scbind_pos;
#endif

    int		w_preview;	/* Flag to indicate a preview window */

#ifdef FEAT_EVAL
    garray_t	w_vars;		/* internal variables, local to window */
#endif

#if defined(FEAT_RIGHTLEFT) && defined(FEAT_FKMAP)
    int		w_farsi;	/* for the window dependent Farsi functions */
#endif

    /*
     * The w_prev_pcmark field is used to check whether we really did jump to
     * a new line after setting the w_pcmark.  If not, then we revert to
     * using the previous w_pcmark.
     */
    pos_t	w_pcmark;	    /* previous context mark */
    pos_t	w_prev_pcmark;	    /* previous w_pcmark */

    /*
     * the jumplist contains old cursor positions
     */
    fmark_t	w_jumplist[JUMPLISTSIZE];
    int		w_jumplistlen;		/* number of active entries */
    int		w_jumplistidx;		/* current position */

    /*
     * the tagstack grows from 0 upwards:
     * entry 0: older
     * entry 1: newer
     * entry 2: newest
     */
    taggy_t	w_tagstack[TAGSTACKSIZE];	/* the tag stack */
    int		w_tagstackidx;		/* idx just below activ entry */
    int		w_tagstacklen;		/* number of tags on stack */

    /*
     * w_fraction is the fractional row of the cursor within the window, from
     * 0 at the top row to FRACTION_MULT at the last row.
     * w_prev_fraction_row was the actual cursor row when w_fraction was last
     * calculated.
     */
    int		    w_fraction;
    int		    w_prev_fraction_row;

#ifdef FEAT_GUI
    scrollbar_t	    w_scrollbars[2];	/* vert. Scrollbars for this window */
#endif

#ifdef FEAT_PERL
    void	    *perl_private;
#endif

#ifdef FEAT_PYTHON
    void	    *python_ref;	/* The Python value referring to this
					   window */
#endif

#ifdef FEAT_TCL
    void	    *tcl_ref;
#endif

#ifdef FEAT_RUBY
    void	    *ruby_ref;
#endif
};

/*
 * Arguments for operators.
 */
typedef struct oparg
{
    int		op_type;	/* current pending operator type */
    int		regname;	/* register to use for the operator */
    int		motion_type;	/* type of the current cursor motion */
    int		motion_force;	/* force motion type: 'v', 'V' or CTRL-V */
    int		inclusive;	/* TRUE if char motion is inclusive (only
				   valid when motion_type is MCHAR */
    int		end_adjusted;	/* backuped b_op_end one char (only used by
				   do_format()) */
    pos_t	start;		/* start of the operator */
    pos_t	end;		/* end of the operator */
    long	line_count;	/* number of lines from op_start to op_end
				   (inclusive) */
    int		empty;		/* op_start and op_end the same (only used by
				   do_change()) */
#ifdef FEAT_VISUAL
    int		is_VIsual;	/* operator on Visual area */
#endif
    int		block_mode;	/* current operator is Visual block mode */
    colnr_t	start_vcol;	/* start col for block mode operator */
    colnr_t	end_vcol;	/* end col for block mode operator */
} oparg_t;

/*
 * Arguments for Normal mode commands.
 */
typedef struct cmdarg
{
    oparg_t	*oap;		/* Operator arguments */
    int		prechar;	/* prefix character (optional, always 'g') */
    int		cmdchar;	/* command character */
    int		nchar;		/* next command character (optional) */
    int		extra_char;	/* yet another character (optional) */
    long	opcount;	/* count before an operator */
    long	count0;		/* count before command, default 0 */
    long	count1;		/* count before command, default 1 */
    int		arg;		/* extra argument from nv_cmds[] */
    int		retval;		/* return: CA_* values */
    char_u	*searchbuf;	/* return: pointer to search pattern or NULL */
} cmdarg_t;

/* values for retval: */
#define CA_COMMAND_BUSY	    1	/* skip restarting edit() once */
#define CA_NO_ADJ_OP_END    2	/* don't adjust operator end */

#ifdef CURSOR_SHAPE
/*
 * struct to store values from 'guicursor' and 'mouseshape'
 */
/* Indexes in shape_table[] */
#define SHAPE_IDX_N	0	/* Normal mode */
#define SHAPE_IDX_V	1	/* Visual mode */
#define SHAPE_IDX_I	2	/* Insert mode */
#define SHAPE_IDX_R	3	/* Replace mode */
#define SHAPE_IDX_C	4	/* Command line Normal mode */
#define SHAPE_IDX_CI	5	/* Command line Insert mode */
#define SHAPE_IDX_CR	6	/* Command line Replace mode */
#define SHAPE_IDX_O	7	/* Operator-pending mode */
#define SHAPE_IDX_VE	8	/* Visual mode with 'seleciton' exclusive */
#define SHAPE_IDX_CLINE	9	/* On command line */
#define SHAPE_IDX_STATUS 10	/* A status line */
#define SHAPE_IDX_SDRAG 11	/* dragging a status line */
#define SHAPE_IDX_VSEP	12	/* A vertical separator line */
#define SHAPE_IDX_VDRAG 13	/* dragging a vertical separator line */
#define SHAPE_IDX_MORE	14	/* Hit-return or More */
#define SHAPE_IDX_SM	15	/* showing matching paren */
#define SHAPE_IDX_COUNT	16

#define SHAPE_BLOCK	0	/* block cursor */
#define SHAPE_HOR	1	/* horizontal bar cursor */
#define SHAPE_VER	2	/* vertical bar cursor */

#define MSHAPE_NUMBERED	1000	/* offset for shapes identified by number */
#define MSHAPE_HIDE	1	/* hide mouse pointer */

#define SHAPE_MOUSE	1	/* used for mouse pointer shape */
#define SHAPE_CURSOR	2	/* used for text cursor shape */

typedef struct cursor_entry
{
    int		shape;		/* one of the SHAPE_ defines */
    int		mshape;		/* one of the MSHAPE defines */
    int		percentage;	/* percentage of cell for bar */
    long	blinkwait;	/* blinking, wait time before blinking starts */
    long	blinkon;	/* blinking, on time */
    long	blinkoff;	/* blinking, off time */
    int		id;		/* highlight group ID */
    char	*name;		/* mode name (fixed) */
    char	used_for;	/* SHAPE_MOUSE and/or SHAPE_CURSOR */
} cursorentry_t;
#endif /* CURSOR_SHAPE */

#ifdef FEAT_MENU

/* Indices into vimmenu_t->strings[] and vimmenu_t->noremap[] for each mode */
#define MENU_INDEX_INVALID	-1
#define MENU_INDEX_NORMAL	0
#define MENU_INDEX_VISUAL	1
#define MENU_INDEX_OP_PENDING	2
#define MENU_INDEX_INSERT	3
#define MENU_INDEX_CMDLINE	4
#define MENU_INDEX_TIP		5
#define MENU_MODES		6

/* Menu modes */
#define MENU_NORMAL_MODE	(1 << MENU_INDEX_NORMAL)
#define MENU_VISUAL_MODE	(1 << MENU_INDEX_VISUAL)
#define MENU_OP_PENDING_MODE	(1 << MENU_INDEX_OP_PENDING)
#define MENU_INSERT_MODE	(1 << MENU_INDEX_INSERT)
#define MENU_CMDLINE_MODE	(1 << MENU_INDEX_CMDLINE)
#define MENU_TIP_MODE		(1 << MENU_INDEX_TIP)
#define MENU_ALL_MODES		((1 << MENU_INDEX_TIP) - 1)
/*note MENU_INDEX_TIP is not a 'real' mode*/

/* Start a menu name with this to not include it on the main menu bar */
#define MNU_HIDDEN_CHAR		']'

typedef struct VimMenu vimmenu_t;

struct VimMenu
{
    int		modes;		    /* Which modes is this menu visible for? */
    int		enabled;	    /* for which modes the menu is enabled */
    char_u	*name;		    /* Name of menu */
    char_u	*dname;		    /* Displayed Name (without '&') */
    int		mnemonic;	    /* mnemonic key (after '&') */
    char_u	*actext;	    /* accelerator text (after TAB) */
    int		priority;	    /* Menu order priority */
#ifdef FEAT_GUI
    void	(*cb)();	    /* Call-back routine */
#endif
    char_u	*strings[MENU_MODES]; /* Mapped string for each mode */
    int		noremap[MENU_MODES]; /* A REMAP_ flag for each mode */
    vimmenu_t	*children;	    /* Children of sub-menu */
    vimmenu_t	*parent;	    /* Parent of menu */
    vimmenu_t	*next;		    /* Next item in menu */
#ifdef FEAT_GUI_X11
    Widget	id;		    /* Manage this to enable item */
    Widget	submenu_id;	    /* If this is submenu, add children here */
#endif
#ifdef FEAT_GUI_GTK
    GtkWidget	*id;		    /* Manage this to enable item */
    GtkWidget	*submenu_id;	    /* If this is submenu, add children here */
    GtkWidget	*tearoff_handle;
    GtkWidget   *label;		    /* Used by "set wak=" code. */
#endif
#ifdef FEAT_GUI_MOTIF
    int		sensitive;	    /* turn button on/off */
#endif
#ifdef FEAT_GUI_W16
    UINT	id;		    /* Id of menu item */
    HMENU	submenu_id;	    /* If this is submenu, add children here */
#endif
#ifdef FEAT_GUI_W32
    UINT	id;		    /* Id of menu item */
    HMENU	submenu_id;	    /* If this is submenu, add children here */
    HWND	tearoff_handle;	    /* hWnd of tearoff if created */
#endif
#if FEAT_GUI_BEOS
    BMenuItem	*id;		    /* Id of menu item */
    BMenu	*submenu_id;	    /* If this is submenu, add children here */
#endif
#ifdef macintosh
    MenuHandle	id;
    short	index;		    /* the item index within the father menu */
    short	menu_id;	    /* the menu id to which this item belong */
    short	submenu_id;	    /* the menu id of the children (could be
				       get throught some tricks) */
    MenuHandle	menu_handle;
    MenuHandle	submenu_handle;
#endif
#if defined(FEAT_GUI_AMIGA)
				    /* only one of these will ever be set, but
				     * they are used to allow the menu routine
				     * to easily get a hold of the parent menu
				     * pointer which is needed by all items to
				     * form the chain correctly */
    int		    id;		    /* unused by the amiga, but used in the
				     * code kept for compatibility */
    struct Menu	    *menuPtr;
    struct MenuItem *menuItemPtr;
#endif
#ifdef RISCOS
    int		*id;		    /* Not used, but gui.c needs it */
    int		greyed_out;	    /* Flag */
    int		hidden;
#endif
};
#else
/* For generating prototypes when FEAT_MENU isn't defined. */
typedef int vimmenu_t;

#endif /* FEAT_MENU */

/*
 * Struct to save values in before executing autocommands for a buffer that is
 * not the current buffer.
 */
typedef struct
{
    buf_t	*save_buf;	/* saved curbuf */
    buf_t	*new_curbuf;	/* buffer to be used */
    win_t	*save_curwin;	/* saved curwin, NULL if it didn't change */
    win_t	*new_curwin;	/* new curwin if save_curwin != NULL */
    pos_t	save_cursor;	/* saved cursor pos of save_curwin */
    linenr_t	save_topline;	/* saved topline of save_curwin */
} aco_save_t;
