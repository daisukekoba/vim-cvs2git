/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * screen.c: code for displaying on the screen
 *
 * Output to the screen (console, terminal emulator or GUI window) is minized
 * by remembering what is already on the screen, and only updating the parts
 * that changed.
 *
 * ScreenLines[] contains a copy of the whole screen, as it is currently
 * displayed (excluding text written by external commands).
 * ScreenAttrs[] contains the associated attributes.
 * LineOffset[] contains the offset into ScreenLines[] for each line.
 * For double-byte characters, two consecutive bytes in ScreenLines[] can form
 * one character which occupies two display cells.
 * For UTF-8 a multi-byte character is converted to Unicode and stored in
 * ScreenLinesUC[].  ScreenLines[] contains the first byte only.  For an ASCII
 * character without composing chars ScreenLinesUC[] will be 0.  When the
 * character occupies two display cells the next byte in ScreenLines[] is 0.
 * ScreenLinesC1[] and ScreenLinesC2[] contain up to two composing characters
 * (drawn on top of the first character).  They are 0 when not used.
 * ScreenLines2[] is only used for euc-jp to store the second byte if the
 * first byte is 0x8e (single-width character).
 *
 * The screen_*() functions write to the screen and handle updating
 * ScreenLines[].
 *
 * update_screen() is the function that updates all windows and status lines.
 * It is called form the main loop when must_redraw is non-zero.  It may be
 * called from other places when an immediated screen update is needed.
 *
 * The part of the buffer that is displayed in a window is set with:
 * - w_topline (first buffer line in window)
 * - w_topfill (filler line above the first line)
 * - w_leftcol (leftmost window cell in window),
 * - w_skipcol (skipped window cells of first line)
 *
 * Commands that only move the cursor around in a window, do not need to take
 * action to update the display.  The main loop will check if w_topline is
 * valid and update it (scroll the window) when needed.
 *
 * Commands that scroll a window change w_topline and must call
 * check_cursor() to move the cursor into the visible part of the window, and
 * call redraw_later(VALID) to have the window displayed by update_screen()
 * later.
 *
 * Commands that change text in the buffer must call changed_bytes() or
 * changed_lines() to mark the area that changed and will require updating
 * later.  The main loop will call update_screen(), which will update each
 * window that shows the changed buffer.  This assumes text above the change
 * can remain displayed as it is.  Text after the change may need updating for
 * scrolling, folding and syntax highlighting.
 *
 * Commands that change how a window is displayed (e.g., setting 'list') or
 * invalidate the contents of a window in another way (e.g., change fold
 * settings), must call redraw_later(NOT_VALID) to have the whole window
 * redisplayed by update_screen() later.
 *
 * Commands that change how a buffer is displayed (e.g., setting 'tabstop')
 * must call redraw_curbuf_later(NOT_VALID) to have all the windows for the
 * buffer redisplayed by update_screen() later.
 *
 * Commands that move the window position must call redraw_later(NOT_VALID).
 * TODO: should minimize redrawing by scrolling when possible.
 *
 * Commands that change everything (e.g., resizing the screen) must call
 * redraw_all_later(NOT_VALID) or redraw_all_later(CLEAR).
 *
 * Things that are handled indirectly:
 * - When messages scroll the screen up, msg_scrolled will be set and
 *   update_screen() called to redraw.
 */

#include "vim.h"

/*
 * The attributes that are actually active for writing to the screen.
 */
static int	screen_attr = 0;

/*
 * Positioning the cursor is reduced by remembering the last position.
 * Mostly used by windgoto() and screen_char().
 */
static int	screen_cur_row, screen_cur_col;	/* last known cursor position */

#ifdef FEAT_SEARCH_EXTRA
/*
 * Struct used for highlighting 'hlsearch' matches for the last use search
 * pattern or a ":match" item.
 * For 'hlsearch' there is one pattern for all windows.  For ":match" there is
 * a different pattern for each window.
 */
typedef struct
{
    regmmatch_T	rm;	/* points to the regexp program; contains last found
			   match (may continue in next line) */
    buf_T	*buf;	/* the buffer to search for a match */
    linenr_T	lnum;	/* the line to search for a match */
    int		attr;	/* attributes to be used for a match */
    int		attr_cur; /* attributes currently active in win_line() */
    linenr_T	first_lnum;	/* first lnum to search for multi-line pat */
    char_u	*startp; /* in win_line() points to char where HL starts */
    char_u	*endp;	 /* in win_line() points to char where HL ends */
} match_T;

static match_T search_hl;	/* used for 'hlsearch' highlight matching */
static match_T match_hl;	/* used for ":match" highlight matching */
#endif

#ifdef FEAT_FOLDING
static foldinfo_T win_foldinfo;	/* info for 'foldcolumn' */
#endif

/*
 * Buffer for one screen line (characters and attributes).
 */
static schar_T	*current_ScreenLine;

static void win_update __ARGS((win_T *wp));
static void win_draw_end __ARGS((win_T *wp, int c, int row));
#ifdef FEAT_FOLDING
static void fold_line __ARGS((win_T *wp, long fold_count, foldinfo_T *foldinfo, linenr_T lnum, int row));
static void fill_foldcolumn __ARGS((char_u *p, win_T *wp, int closed, linenr_T lnum));
static void copy_text_attr __ARGS((int off, char_u *buf, int len, int attr));
#endif
static int win_line __ARGS((win_T *, linenr_T, int, int));
static int char_needs_redraw __ARGS((int off_from, int off_to, int len));
#ifdef FEAT_RIGHTLEFT
static void screen_line __ARGS((int row, int coloff, int endcol, int clear_width, int rlflag));
# define SCREEN_LINE(r, o, e, c, rl)    screen_line((r), (o), (e), (c), (rl))
#else
static void screen_line __ARGS((int row, int coloff, int endcol, int clear_width));
# define SCREEN_LINE(r, o, e, c, rl)    screen_line((r), (o), (e), (c))
#endif
#if defined(FEAT_WILDMENU) && defined(FEAT_VERTSPLIT)
static void win_redraw_last_status __ARGS((frame_T *frp));
#endif
#ifdef FEAT_VERTSPLIT
static void draw_vsep_win __ARGS((win_T *wp, int row));
#endif
#ifdef FEAT_SEARCH_EXTRA
static void start_search_hl __ARGS((void));
static void end_search_hl __ARGS((void));
static void prepare_search_hl __ARGS((win_T *wp, linenr_T lnum));
static void next_search_hl __ARGS((win_T *win, match_T *shl, linenr_T lnum, colnr_T mincol));
#endif
static void screen_start_highlight __ARGS((int attr));
static void screen_char __ARGS((unsigned off, int row, int col));
#ifdef FEAT_MBYTE
static void screen_char_2 __ARGS((unsigned off, int row, int col));
#endif
static void screenclear2 __ARGS((void));
static void lineclear __ARGS((unsigned off, int width));
static void lineinvalid __ARGS((unsigned off, int width));
#ifdef FEAT_VERTSPLIT
static void linecopy __ARGS((int to, int from, win_T *wp));
static void redraw_block __ARGS((int row, int end, win_T *wp));
#endif
static int win_do_lines __ARGS((win_T *wp, int row, int line_count, int mayclear, int del));
static void win_rest_invalid __ARGS((win_T *wp));
static int screen_ins_lines __ARGS((int, int, int, int, win_T *wp));
static void msg_pos_mode __ARGS((void));
#if defined(FEAT_WINDOWS) || defined(FEAT_WILDMENU) || defined(FEAT_STL_OPT)
static int fillchar_status __ARGS((int *attr, int is_curwin));
#endif
#ifdef FEAT_VERTSPLIT
static int fillchar_vsep __ARGS((int *attr));
#endif
#ifdef FEAT_STL_OPT
static void win_redr_custom __ARGS((win_T *wp, int Ruler));
#endif
#ifdef FEAT_CMDL_INFO
static void win_redr_ruler __ARGS((win_T *wp, int always));
#endif

#if defined(FEAT_CLIPBOARD) || defined(FEAT_VERTSPLIT)
/* Ugly global: overrule attribute used by screen_char() */
static int screen_char_attr = 0;
#endif

/*
 * Redraw the current window later, with update_screen(type).
 * Set must_redraw only if not already set to a higher value.
 * e.g. if must_redraw is CLEAR, type NOT_VALID will do nothing.
 */
    void
redraw_later(type)
    int		type;
{
    redraw_win_later(curwin, type);
}

    void
redraw_win_later(wp, type)
    win_T	*wp;
    int		type;
{
    if (wp->w_redr_type < type)
    {
	wp->w_redr_type = type;
	if (type >= NOT_VALID)
	    wp->w_lines_valid = 0;
	if (must_redraw < type)	/* must_redraw is the maximum of all windows */
	    must_redraw = type;
    }
}

/*
 * Force a complete redraw later.  Also resets the highlighting.  To be used
 * after executing a shell command that messes up the screen.
 */
    void
redraw_later_clear()
{
    redraw_all_later(CLEAR);
    screen_attr = HL_BOLD | HL_UNDERLINE;
}

/*
 * Mark all windows to be redrawn later.
 */
    void
redraw_all_later(type)
    int		type;
{
    win_T	*wp;

    FOR_ALL_WINDOWS(wp)
    {
	redraw_win_later(wp, type);
    }
}

/*
 * Mark all windows that are editing the current buffer to be udpated later.
 */
    void
redraw_curbuf_later(type)
    int		type;
{
    redraw_buf_later(curbuf, type);
}

    void
redraw_buf_later(buf, type)
    buf_T	*buf;
    int		type;
{
    win_T	*wp;

    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_buffer == buf)
	    redraw_win_later(wp, type);
    }
}

/*
 * Changed something in the current window, at buffer line "lnum", that
 * requires that line and possibly other lines to be redrawn.
 * Used when entering/leaving Insert mode with the cursor on a folded line.
 * Used to remove the "$" from a change command.
 * Note that when also inserting/deleting lines w_redraw_top and w_redraw_bot
 * may become invalid and the whole window will have to be redrawn.
 */
/*ARGSUSED*/
    void
redrawWinline(lnum, invalid)
    linenr_T	lnum;
    int		invalid;	/* window line height is invalid now */
{
#ifdef FEAT_FOLDING
    int		i;
#endif

    if (curwin->w_redraw_top == 0 || curwin->w_redraw_top > lnum)
	curwin->w_redraw_top = lnum;
    if (curwin->w_redraw_bot == 0 || curwin->w_redraw_bot < lnum)
	curwin->w_redraw_bot = lnum;
    redraw_later(VALID);

#ifdef FEAT_FOLDING
    if (invalid)
    {
	/* A w_lines[] entry for this lnum has become invalid. */
	i = find_wl_entry(curwin, lnum);
	if (i >= 0)
	    curwin->w_lines[i].wl_valid = FALSE;
    }
#endif
}

/*
 * update all windows that are editing the current buffer
 */
    void
update_curbuf(type)
    int		type;
{
    redraw_curbuf_later(type);
    update_screen(type);
}

/*
 * update_screen()
 *
 * Based on the current value of curwin->w_topline, transfer a screenfull
 * of stuff from Filemem to ScreenLines[], and update curwin->w_botline.
 */
    void
update_screen(type)
    int		type;
{
    win_T	*wp;
    static int	did_intro = FALSE;
#if defined(FEAT_SEARCH_EXTRA) || defined(FEAT_CLIPBOARD)
    int		did_one;
#endif

    if (!screen_valid(TRUE))
	return;

    if (must_redraw)
    {
	if (type < must_redraw)	    /* use maximal type */
	    type = must_redraw;
	must_redraw = 0;
    }

    /* Need to update w_lines[]. */
    if (curwin->w_lines_valid == 0 && type < NOT_VALID)
	type = NOT_VALID;

    if (!redrawing())
    {
	redraw_later(type);		/* remember type for next time */
	must_redraw = type;
	if (type > INVERTED_ALL)
	    curwin->w_lines_valid = 0;	/* don't use w_lines[].wl_size now */
	return;
    }

#ifdef FEAT_GUI
    updating_screen = TRUE;
#endif
#ifdef FEAT_SYN_HL
    ++display_tick;	    /* let syntax code know we're in a next round of
			     * display updating */
#endif

    /*
     * if the screen was scrolled up when displaying a message, scroll it down
     */
    if (msg_scrolled)
    {
	clear_cmdline = TRUE;
	if (msg_scrolled > Rows - 5)	    /* clearing is faster */
	    type = CLEAR;
	else if (type != CLEAR)
	{
	    check_for_delay(FALSE);
	    if (screen_ins_lines(0, 0, msg_scrolled, (int)Rows, NULL) == FAIL)
		type = CLEAR;
	    FOR_ALL_WINDOWS(wp)
	    {
		if (W_WINROW(wp) < msg_scrolled)
		{
		    if (W_WINROW(wp) + wp->w_height > msg_scrolled
			    && wp->w_redr_type < REDRAW_TOP)
		    {
			wp->w_upd_rows = msg_scrolled - W_WINROW(wp);
			wp->w_redr_type = REDRAW_TOP;
		    }
		    else
		    {
			wp->w_redr_type = NOT_VALID;
#ifdef FEAT_WINDOWS
			if (W_WINROW(wp) + wp->w_height + W_STATUS_HEIGHT(wp)
				<= msg_scrolled)
			    wp->w_redr_status = TRUE;
#endif
		    }
		}
	    }
	    redraw_cmdline = TRUE;
	}
	msg_scrolled = 0;
	need_wait_return = FALSE;
    }

    /* reset cmdline_row now (may have been changed temporarily) */
    compute_cmdrow();

    /* Check for changed highlighting */
    if (need_highlight_changed)
	highlight_changed();

    if (type == CLEAR)		/* first clear screen */
    {
	screenclear();		/* will reset clear_cmdline */
	type = NOT_VALID;
    }

    if (clear_cmdline)		/* first clear cmdline */
    {
	check_for_delay(FALSE);
	msg_clr_cmdline();	/* will reset clear_cmdline */
    }

    /*
     * Only start redrawing if there is really something to do.
     */
    if (type == INVERTED)
	update_curswant();
    if (curwin->w_redr_type < type
	    && !((type == VALID
		    && curwin->w_lines[0].wl_valid
#ifdef FEAT_DIFF
		    && curwin->w_topfill == curwin->w_old_topfill
#endif
		    && curwin->w_topline == curwin->w_lines[0].wl_lnum)
#ifdef FEAT_VISUAL
		|| (type == INVERTED
		    && curwin->w_old_cursor_lnum == curwin->w_cursor.lnum
		    && curwin->w_old_visual_mode == VIsual_mode
		    && (curwin->w_valid & VALID_VIRTCOL)
		    && curwin->w_old_curswant == curwin->w_curswant)
#endif
		))
	curwin->w_redr_type = type;

#ifdef FEAT_SYN_HL
    /*
     * Correct stored syntax highlighting info for changes in each displayed
     * buffer.  Each buffer must only be done once.
     */
    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_buffer->b_mod_set)
	{
# ifdef FEAT_WINDOWS
	    win_T	*wwp;

	    /* Check if we already did this buffer. */
	    for (wwp = firstwin; wwp != wp; wwp = wwp->w_next)
		if (wwp->w_buffer == wp->w_buffer)
		    break;
# endif
	    if (
# ifdef FEAT_WINDOWS
		    wwp == wp &&
# endif
		    syntax_present(wp->w_buffer))
		syn_stack_apply_changes(wp->w_buffer);
	}
    }
#endif

    /*
     * Go from top to bottom through the windows, redrawing the ones that need
     * it.
     */
#ifdef FEAT_GUI
    /* Remove the cursor before starting to do anything, because scrolling may
     * make it difficult to redraw the text under it. */
    if (gui.in_use)
	gui_undraw_cursor();
#endif
#if defined(FEAT_SEARCH_EXTRA) || defined(FEAT_CLIPBOARD)
    did_one = FALSE;
#endif
#ifdef FEAT_SEARCH_EXTRA
    search_hl.rm.regprog = NULL;
#endif
    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_redr_type != 0)
	{
	    cursor_off();
#if defined(FEAT_SEARCH_EXTRA) || defined(FEAT_CLIPBOARD)
	    if (!did_one)
	    {
		did_one = TRUE;
# ifdef FEAT_SEARCH_EXTRA
		start_search_hl();
# endif
# ifdef FEAT_CLIPBOARD
		/* When Visual area changed, may have to update selection. */
		if (clip_star.available && clip_isautosel())
		    clip_update_selection();
# endif
	    }
#endif
	    win_update(wp);
	}

#ifdef FEAT_WINDOWS
	/* redraw status line after each window to minimize cursor movement */
	if (wp->w_redr_status)
	{
	    cursor_off();
	    win_redr_status(wp);
	}
#endif
    }
#if defined(FEAT_SEARCH_EXTRA)
    end_search_hl();
#endif

#ifdef FEAT_WINDOWS
    /* Reset b_mod_set flags.  Going through all windows is probably faster
     * than going through all buffers (there could be many buffers). */
    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	wp->w_buffer->b_mod_set = FALSE;
#else
	curbuf->b_mod_set = FALSE;
#endif

#ifdef FEAT_GUI
    updating_screen = FALSE;
    gui_may_resize_shell();
#endif

    if (redraw_cmdline)
	showmode();

    /* May put up an introductory message when not editing a file */
    if (!did_intro && bufempty()
	    && curbuf->b_fname == NULL
#ifdef FEAT_WINDOWS
	    && firstwin->w_next == NULL
#endif
	    && vim_strchr(p_shm, SHM_INTRO) == NULL)
	intro_message();
    did_intro = TRUE;

#ifdef FEAT_GUI
    /* Redraw the cursor and update the scrollbars when all screen updating is
     * done. */
    if (gui.in_use)
    {
	out_flush();	/* required before updating the cursor */
	gui_update_cursor(FALSE, FALSE);
	gui_update_scrollbars(FALSE);
    }
#endif
}

#if defined(FEAT_SIGNS) || defined(PROTO)
    static int
lnum2row(wp, lnum)
    win_T	*wp;
    linenr_T	lnum;
{
    int		row = 0;
    int		j;
    linenr_T	i;

    for (i = wp->w_topline, j = 0;
			      i < wp->w_botline && j < wp->w_lines_valid; i++)
    {
	row += wp->w_lines[j++].wl_size;
#ifdef FEAT_FOLDING
	(void)hasFolding(i, NULL, &i);
#endif
	if (i >= lnum)
	    break;
    }
    return row - 1;
}

    void
update_debug_sign(buf, lnum)
    buf_T	*buf;
    linenr_T	lnum;
{
    win_T	*wp;
    int		row;

#ifdef FEAT_FOLDING
    win_foldinfo.fi_level = 0;
#endif

#ifdef FEAT_GUI
    /* Remove the cursor before starting to do anything, because scrolling may
     * make it difficult to redraw the text under it. */
    if (gui.in_use)
	gui_undraw_cursor();
#endif
    if (buf != NULL && lnum > 0)
    {					/* update/delete a specific mark */
	FOR_ALL_WINDOWS(wp)
	{
	    if (wp->w_buffer == buf && lnum < wp->w_botline)
	    {
		/* update mark */
		row = lnum2row(wp, lnum);
		if (row > 0)
		{
		    screen_start();	/* not sure of screen cursor */
		    win_line(wp, lnum, row, row + 1);
		}
	    }
	}
    }
    else
    {					/* delete all marks */
#ifdef FEAT_WINDOWS
	for (wp = firstwin; wp; wp = wp->w_next)
	    win_update(wp);
#else
	    win_update(curwin);
#endif
    }
#ifdef FEAT_GUI
    /* Redraw the cursor and update the scrollbars when all screen updating is
     * done. */
    if (gui.in_use)
    {
	out_flush();	/* required before updating the cursor */
	gui_update_cursor(FALSE, FALSE);
	gui_update_scrollbars(FALSE);
    }
#endif
}
#endif


#ifdef FEAT_GUI
/*
 * Update a single window, its status line and maybe the command line msg.
 * Used for the GUI scrollbar.
 */
    void
updateWindow(wp)
    win_T	*wp;
{
    cursor_off();
    updating_screen = TRUE;
#ifdef FEAT_GUI
    /* Remove the cursor before starting to do anything, because scrolling may
     * make it difficult to redraw the text under it. */
    if (gui.in_use)
	gui_undraw_cursor();
#endif
#ifdef FEAT_SEARCH_EXTRA
    start_search_hl();
#endif
#ifdef FEAT_CLIPBOARD
    /* When Visual area changed, may have to update selection. */
    if (clip_star.available && clip_isautosel())
	clip_update_selection();
#endif
    win_update(wp);
#ifdef FEAT_WINDOWS
    if (wp->w_redr_status
# ifdef FEAT_CMDL_INFO
	    || p_ru
# endif
# ifdef FEAT_STL_OPT
	    || *p_stl
# endif
	    )
	win_redr_status(wp);
#endif
    if (redraw_cmdline)
	showmode();
#ifdef FEAT_SEARCH_EXTRA
    end_search_hl();
#endif
    updating_screen = FALSE;
    gui_may_resize_shell();

#ifdef FEAT_GUI
    /* Redraw the cursor and update the scrollbars when all screen updating is
     * done. */
    if (gui.in_use)
    {
	out_flush();	/* required before updating the cursor */
	gui_update_cursor(FALSE, FALSE);
	gui_update_scrollbars(FALSE);
    }
#endif
}
#endif

/*
 * Update a single window.
 *
 * This may cause the windows below it also to be redrawn (when clearing the
 * screen or scrolling lines).
 *
 * How the window is redrawn depends on wp->w_redr_type.  Each type also
 * implies the one below it.
 * NOT_VALID	redraw the whole window
 * REDRAW_TOP	redraw the top w_upd_rows window lines, otherwise like VALID
 * INVERTED	redraw the changed part of the Visual area
 * INVERTED_ALL	redraw the whole Visual area
 * VALID	1. scroll up/down to adjust for a changed w_topline
 *		2. update lines at the top when scrolled down
 *		3. redraw changed text:
 *		   - if wp->w_buffer->b_mod_set set, udpate lines between
 *		     b_mod_top and b_mod_bot.
 *		   - if wp->w_redraw_top non-zero, redraw lines between
 *		     wp->w_redraw_top and wp->w_redr_bot.
 *		   - continue redrawing when syntax status is invalid.
 *		4. if scrolled up, update lines at the bottom.
 * This results in three areas that may need updating:
 * top:	from first row to top_end (when scrolled down)
 * mid: from mid_start to mid_end (update inversion or changed text)
 * bot: from bot_start to last row (when scrolled up)
 */
    static void
win_update(wp)
    win_T	*wp;
{
    buf_T	*buf = wp->w_buffer;
    int		type;
    int		top_end = 0;	/* Below last row of the top area that needs
				   updating.  0 when no top area updating. */
    int		mid_start = 999;/* first row of the mid area that needs
				   updating.  999 when no mid area updating. */
    int		mid_end = 0;	/* Below last row of the mid area that needs
				   updating.  0 when no mid area updating. */
    int		bot_start = 999;/* first row of the bot area that needs
				   updating.  999 when no bot area updating */
    int		scrolled_down = FALSE;	/* TRUE when scrolled down when
					   w_topline got smaller a bit */
#ifdef FEAT_SEARCH_EXTRA
    int		top_to_mod = FALSE;    /* redraw above mod_top */
#endif

    int		row;		/* current window row to display */
    linenr_T	lnum;		/* current buffer lnum to display */
    int		idx;		/* current index in w_lines[] */
    int		srow;		/* starting row of the current line */

    int		eof = FALSE;	/* if TRUE, we hit the end of the file */
    int		didline = FALSE; /* if TRUE, we finished the last line */
    int		i;
    long	j;
    static int	recursive = FALSE;	/* being called recursively */
    int		old_botline = wp->w_botline;
#ifdef FEAT_FOLDING
    long	fold_count;
#endif
#ifdef FEAT_SYN_HL
    /* remember what happened to the previous line, to know if
     * check_visual_highlight() can be used */
#define DID_NONE 1	/* didn't update a line */
#define DID_LINE 2	/* updated a normal line */
#define DID_FOLD 3	/* updated a folded line */
    int		did_update = DID_NONE;
    linenr_T	syntax_last_parsed = 0;		/* last parsed text line */
#endif
    linenr_T	mod_top = 0;
    linenr_T	mod_bot = 0;
#if defined(FEAT_SYN_HL) || defined(FEAT_SEARCH_EXTRA)
    int		save_got_int;
#endif

    type = wp->w_redr_type;

    if (type == NOT_VALID)
    {
#ifdef FEAT_WINDOWS
	wp->w_redr_status = TRUE;
#endif
	wp->w_lines_valid = 0;
    }

    /* Window is zero-height: nothing to draw. */
    if (wp->w_height == 0)
	return;

#ifdef FEAT_VERTSPLIT
    /* Window is zero-width: Only need to draw the separator. */
    if (wp->w_width == 0)
    {
	/* draw the vertical separator right of this window */
	draw_vsep_win(wp, 0);
	return;
    }
#endif

#ifdef FEAT_SEARCH_EXTRA
    /* Setup for ":match" highlighting.  Disable any previous match */
    match_hl.rm = wp->w_match;
    match_hl.attr = syn_id2attr(wp->w_match_id);
    match_hl.buf = buf;
    match_hl.lnum = 0;
    search_hl.buf = buf;
    search_hl.lnum = 0;
    search_hl.first_lnum = 0;
#endif

    if (buf->b_mod_set && buf->b_mod_xlines != 0 && wp->w_redraw_top != 0)
    {
	/*
	 * When there are both inserted/deleted lines and specific lines to be
	 * redrawn,, w_redraw_top and w_redraw_bot may be invalid, just redraw
	 * everything (only happens when redrawing is off for while).
	 */
	type = NOT_VALID;
    }
    else
    {
	/*
	 * Set mod_top to the first line that needs displaying because of
	 * changes.  Set mod_bot to the first line after the changes.
	 */
	mod_top = wp->w_redraw_top;
	if (wp->w_redraw_bot != 0)
	    mod_bot = wp->w_redraw_bot + 1;
	else
	    mod_bot = 0;
	wp->w_redraw_top = 0;	/* reset for next time */
	wp->w_redraw_bot = 0;
	if (buf->b_mod_set)
	{
	    if (mod_top == 0 || mod_top > buf->b_mod_top)
		mod_top = buf->b_mod_top;
	    if (mod_bot == 0 || mod_bot < buf->b_mod_bot)
		mod_bot = buf->b_mod_bot;

#ifdef FEAT_SEARCH_EXTRA
	    /* When 'hlsearch' is on and using a multi-line search pattern, a
	     * change in one line may make the Search highlighting in a
	     * previous line invalid.  Simple solution: redraw all visible
	     * lines above the change.
	     * Same for a ":match" pattern.
	     */
	    if ((search_hl.rm.regprog != NULL
			&& re_multiline(search_hl.rm.regprog))
		    || (match_hl.rm.regprog != NULL
			&& re_multiline(match_hl.rm.regprog)))
		top_to_mod = TRUE;
#endif
	}
#ifdef FEAT_FOLDING
	if (mod_top != 0 && hasAnyFolding(wp))
	{
	    linenr_T	lnumt, lnumb;

	    /*
	     * A change in a line can cause lines above it to become folded or
	     * unfolded.  Find the top most buffer line that may be affected.
	     * If the line was previously folded and displayed, get the first
	     * line of that fold.  If the line is folded now, get the first
	     * folded line.  Use the minimum of these two.
	     */

	    /* Find last valid w_lines[] entry above mod_top.  Set lnumt to
	     * the line below it.  If there is no valid entry, use w_topline.
	     * Find the first valid w_lines[] entry below mod_bot.  Set lnumb
	     * to this line.  If there is no valid entry, use MAXLNUM. */
	    lnumt = wp->w_topline;
	    lnumb = MAXLNUM;
	    for (i = 0; i < wp->w_lines_valid; ++i)
		if (wp->w_lines[i].wl_valid)
		{
		    if (wp->w_lines[i].wl_lastlnum < mod_top)
			lnumt = wp->w_lines[i].wl_lastlnum + 1;
		    if (lnumb == MAXLNUM && wp->w_lines[i].wl_lnum >= mod_bot)
		    {
			lnumb = wp->w_lines[i].wl_lnum;
			/* When there is a fold column it might need updating
			 * in the next line ("J" just above an open fold). */
			if (wp->w_p_fdc > 0)
			    ++lnumb;
		    }
		}

	    (void)hasFoldingWin(wp, mod_top, &mod_top, NULL, TRUE, NULL);
	    if (mod_top > lnumt)
		mod_top = lnumt;

	    /* Now do the same for the bottom line (one above mod_bot). */
	    --mod_bot;
	    (void)hasFoldingWin(wp, mod_bot, NULL, &mod_bot, TRUE, NULL);
	    ++mod_bot;
	    if (mod_bot < lnumb)
		mod_bot = lnumb;
	}
#endif

	/* When a change starts above w_topline and the end is below
	 * w_topline, start redrawing at w_topline.
	 * If the end of the change is above w_topline: do like no changes was
	 * maded, but redraw the first line to find changes in syntax. */
	if (mod_top != 0 && mod_top < wp->w_topline)
	{
	    if (mod_bot > wp->w_topline)
		mod_top = wp->w_topline;
#ifdef FEAT_SYN_HL
	    else if (syntax_present(buf))
		top_end = 1;
#endif
	}
    }

    /*
     * When only displaying the lines at the top, set top_end.  Used when
     * window has scrolled down for msg_scrolled.
     */
    if (type == REDRAW_TOP)
    {
	j = 0;
	for (i = 0; i < wp->w_lines_valid; ++i)
	{
	    j += wp->w_lines[i].wl_size;
	    if (j >= wp->w_upd_rows)
	    {
		top_end = j;
		break;
	    }
	}
	if (top_end == 0)
	    /* not found (cannot happen?): redraw everything */
	    type = NOT_VALID;
	else
	    /* top area defined, the rest is VALID */
	    type = VALID;
    }

    /*
     * If there are no changes on the screen that require a complete redraw,
     * handle three cases:
     * 1: we are off the top of the screen by a few lines: scroll down
     * 2: wp->w_topline is below wp->w_lines[0].wl_lnum: may scroll up
     * 3: wp->w_topline is wp->w_lines[0].wl_lnum: find first entry in
     *    w_lines[] that needs updating.
     */
    if ((type == VALID || type == INVERTED || type == INVERTED_ALL))
    {
	if (buf->b_mod_set && wp->w_topline == mod_top)
	{
	    /*
	     * w_topline is the first changed line, the scrolling will be done
	     * further down.
	     */
	}
	else if (wp->w_lines[0].wl_valid
		&& (wp->w_topline < wp->w_lines[0].wl_lnum
#ifdef FEAT_DIFF
		    || (wp->w_topline == wp->w_lines[0].wl_lnum
			&& wp->w_topfill > wp->w_old_topfill)
#endif
		   ))
	{
	    /*
	     * New topline is above old topline: May scroll down.
	     */
#ifdef FEAT_FOLDING
	    if (hasAnyFolding(wp))
	    {
		linenr_T ln;

		/* count the number of lines we are off, counting a sequence
		 * of folded lines as one */
		j = 0;
		for (ln = wp->w_topline; ln < wp->w_lines[0].wl_lnum; ++ln)
		{
		    ++j;
		    if (j >= wp->w_height - 2)
			break;
		    (void)hasFoldingWin(wp, ln, NULL, &ln, TRUE, NULL);
		}
	    }
	    else
#endif
		j = wp->w_lines[0].wl_lnum - wp->w_topline;
	    if (j < wp->w_height - 2)		/* not too far off */
	    {
		i = plines_m_win(wp, wp->w_topline, wp->w_lines[0].wl_lnum - 1);
#ifdef FEAT_DIFF
		/* don't insert lines for invisible filler lines */
		i -= diff_check_fill(wp, wp->w_topline) - wp->w_topfill;
		/* do insert lines for previously invisible filler lines */
		i += diff_check_fill(wp, wp->w_lines[0].wl_lnum)
							  - wp->w_old_topfill;
#endif
		if (i < wp->w_height - 2)	/* less than a screen off */
		{
		    /*
		     * Try to insert the correct number of lines.
		     * If not the last window, delete the lines at the bottom.
		     * win_ins_lines may fail when the terminal can't do it.
		     */
		    if (i > 0)
			check_for_delay(FALSE);
		    if (win_ins_lines(wp, 0, i, FALSE, wp == firstwin) == OK)
		    {
			if (wp->w_lines_valid != 0)
			{
			    /* Need to update rows that are new, stop at the
			     * first one that scrolled down. */
			    top_end = i;
			    scrolled_down = TRUE;

			    /* Move the entries that were scrolled, disable
			     * the entries for the lines to be redrawn. */
			    if ((wp->w_lines_valid += j) > wp->w_height)
				wp->w_lines_valid = wp->w_height;
			    for (idx = wp->w_lines_valid; idx - j >= 0; idx--)
				wp->w_lines[idx] = wp->w_lines[idx - j];
			    while (idx >= 0)
				wp->w_lines[idx--].wl_valid = FALSE;
			}
		    }
		    else
			mid_start = 0;		/* redraw all lines */
		}
		else
		    mid_start = 0;		/* redraw all lines */
	    }
	    else
		mid_start = 0;		/* redraw all lines */
	}
	else
	{
	    /*
	     * New topline is at or below old topline: May scroll up.
	     * When topline didn't change, find first entry in w_lines[] that
	     * needs updating.
	     */

	    /* try to find wp->w_topline in wp->w_lines[].wl_lnum */
	    j = -1;
	    row = 0;
	    for (i = 0; i < wp->w_lines_valid; i++)
	    {
		if (wp->w_lines[i].wl_valid
			&& wp->w_lines[i].wl_lnum == wp->w_topline)
		{
		    j = i;
		    break;
		}
		row += wp->w_lines[i].wl_size;
	    }
	    if (j == -1)
	    {
		/* if wp->w_topline is not in wp->w_lines[].wl_lnum redraw all
		 * lines */
		mid_start = 0;
	    }
	    else
	    {
		/*
		 * Try to delete the correct number of lines.
		 * wp->w_topline is at wp->w_lines[i].wl_lnum.
		 */
#ifdef FEAT_DIFF
		/* If the topline didn't change, delete old filler lines,
		 * otherwise delete filler lines of the new topline... */
		if (wp->w_lines[0].wl_lnum == wp->w_topline)
		    row += wp->w_old_topfill;
		else
		    row += diff_check_fill(wp, wp->w_topline);
		/* ... but don't delete new filler lines. */
		row -= wp->w_topfill;
#endif
		if (row > 0)
		{
		    check_for_delay(FALSE);
		    if (win_del_lines(wp, 0, row, FALSE, wp == firstwin) == OK)
			bot_start = wp->w_height - row;
		    else
			mid_start = 0;		/* redraw all lines */
		}
		if ((row == 0 || bot_start < 999) && wp->w_lines_valid != 0)
		{
		    /*
		     * Skip the lines (below the deleted lines) that are still
		     * valid and don't need redrawing.	Copy their info
		     * upwards, to compensate for the deleted lines.  Set
		     * bot_start to the first row that needs redrawing.
		     */
		    bot_start = 0;
		    idx = 0;
		    for (;;)
		    {
			wp->w_lines[idx] = wp->w_lines[j];
			/* stop at line that didn't fit */
			if (bot_start + row + (int)wp->w_lines[j].wl_size
							       > wp->w_height)
			{
			    wp->w_lines_valid = idx + 1;
			    break;
			}
			bot_start += wp->w_lines[idx++].wl_size;

			/* stop at the last valid entry in w_lines[].wl_size */
			if (++j >= wp->w_lines_valid)
			{
			    wp->w_lines_valid = idx;
			    break;
			}
		    }
#ifdef FEAT_DIFF
		    /* Correct the first entry for filler lines at the top
		     * when it won't get updated below. */
		    if (wp->w_p_diff && bot_start > 0)
			wp->w_lines[0].wl_size =
			    plines_win_nofill(wp, wp->w_topline, TRUE)
							      + wp->w_topfill;
#endif
		}
	    }
	}

	/* When starting redraw in the first line, redraw all lines.  When
	 * there is only one window it's probably faster to clear the screen
	 * first. */
	if (mid_start == 0)
	{
	    mid_end = wp->w_height;
	    if (lastwin == firstwin)
		screenclear();
	}
    }
    else
    {
	/* Not VALID or INVERTED: redraw all lines. */
	mid_start = 0;
	mid_end = wp->w_height;
    }

#ifdef FEAT_VISUAL
    /* check if we are updating or removing the inverted part */
    if ((VIsual_active && buf == curwin->w_buffer)
	    || (wp->w_old_cursor_lnum != 0 && type != NOT_VALID))
    {
	linenr_T    from, to;

	if (VIsual_active)
	{
	    if (VIsual_active
		    && (VIsual_mode != wp->w_old_visual_mode
			|| type == INVERTED_ALL))
	    {
		/*
		 * If the type of Visual selection changed, redraw the whole
		 * selection.  Also when the ownership of the X selection is
		 * gained or lost.
		 */
		if (curwin->w_cursor.lnum < VIsual.lnum)
		{
		    from = curwin->w_cursor.lnum;
		    to = VIsual.lnum;
		}
		else
		{
		    from = VIsual.lnum;
		    to = curwin->w_cursor.lnum;
		}
		/* redraw more when the cursor moved as well */
		if (wp->w_old_cursor_lnum < from)
		    from = wp->w_old_cursor_lnum;
		if (wp->w_old_cursor_lnum > to)
		    to = wp->w_old_cursor_lnum;
	    }
	    else
	    {
		/*
		 * Find the line numbers that need to be updated: The lines
		 * between the old cursor position and the current cursor
		 * position.  Also check if the Visual position changed.
		 */
		if (curwin->w_cursor.lnum < wp->w_old_cursor_lnum)
		{
		    from = curwin->w_cursor.lnum;
		    to = wp->w_old_cursor_lnum;
		}
		else
		{
		    from = wp->w_old_cursor_lnum;
		    to = curwin->w_cursor.lnum;
		    if (from == 0)	/* Visual mode just started */
			from = to;
		}

		if (VIsual.lnum != wp->w_old_visual_lnum)
		{
		    if (wp->w_old_visual_lnum < from
						&& wp->w_old_visual_lnum != 0)
			from = wp->w_old_visual_lnum;
		    if (wp->w_old_visual_lnum > to)
			to = wp->w_old_visual_lnum;
		    if (VIsual.lnum < from)
			from = VIsual.lnum;
		    if (VIsual.lnum > to)
			to = VIsual.lnum;
		}
	    }

	    /*
	     * If in block mode and changed column or curwin->w_curswant:
	     * update all lines.
	     * First compute the actual start and end column.
	     */
	    if (VIsual_mode == Ctrl_V)
	    {
		colnr_T	fromc, toc;

		getvcols(wp, &VIsual, &curwin->w_cursor, &fromc, &toc);
		++toc;
		if (curwin->w_curswant == MAXCOL)
		    toc = MAXCOL;

		if (fromc != wp->w_old_cursor_fcol
			|| toc != wp->w_old_cursor_lcol)
		{
		    if (from > VIsual.lnum)
			from = VIsual.lnum;
		    if (to < VIsual.lnum)
			to = VIsual.lnum;
		}
		wp->w_old_cursor_fcol = fromc;
		wp->w_old_cursor_lcol = toc;
	    }
	}
	else
	{
	    /* Use the line numbers of the old Visual area. */
	    if (wp->w_old_cursor_lnum < wp->w_old_visual_lnum)
	    {
		from = wp->w_old_cursor_lnum;
		to = wp->w_old_visual_lnum;
	    }
	    else
	    {
		from = wp->w_old_visual_lnum;
		to = wp->w_old_cursor_lnum;
	    }
	}

	/*
	 * There is no need to update lines above the top of the window.
	 */
	if (from < wp->w_topline)
	    from = wp->w_topline;

	/*
	 * If we know the value of w_botline, use it to restrict the update to
	 * the lines that are visible in the window.
	 */
	if (wp->w_valid & VALID_BOTLINE)
	{
	    if (from >= wp->w_botline)
		from = wp->w_botline - 1;
	    if (to >= wp->w_botline)
		to = wp->w_botline - 1;
	}

	/*
	 * Find the minimal part to be updated.
	 * Watch out for scrolling that made entries in w_lines[] invalid.
	 * E.g., CTRL-U makes the first half of w_lines[] invalid and sets
	 * top_end; need to redraw from top_end to the "to" line.
	 */
	if (mid_start > 0)
	{
	    lnum = wp->w_topline;
	    idx = 0;
	    if (scrolled_down)
		mid_start = top_end;
	    else
		mid_start = 0;
	    while (lnum < from && idx < wp->w_lines_valid)	/* find start */
	    {
		if (wp->w_lines[idx].wl_valid)
		    mid_start += wp->w_lines[idx].wl_size;
		++idx;
#ifdef FEAT_FOLDING
		if (idx < wp->w_lines_valid && wp->w_lines[idx].wl_valid)
		    lnum = wp->w_lines[idx].wl_lnum;
		else
#endif
		    ++lnum;
	    }
	    srow = mid_start;
	    mid_end = wp->w_height;
	    for ( ; idx < wp->w_lines_valid; ++idx)		/* find end */
	    {
		if (wp->w_lines[idx].wl_valid
			&& wp->w_lines[idx].wl_lnum >= to + 1)
		{
		    /* Only update until first row of this line */
		    mid_end = srow;
		    break;
		}
		srow += wp->w_lines[idx].wl_size;
	    }
	}
    }

    if (VIsual_active && buf == curwin->w_buffer)
    {
	wp->w_old_visual_mode = VIsual_mode;
	wp->w_old_cursor_lnum = curwin->w_cursor.lnum;
	wp->w_old_visual_lnum = VIsual.lnum;
	wp->w_old_curswant = curwin->w_curswant;
    }
    else
    {
	wp->w_old_visual_mode = 0;
	wp->w_old_cursor_lnum = 0;
	wp->w_old_visual_lnum = 0;
    }
#endif /* FEAT_VISUAL */

#if defined(FEAT_SYN_HL) || defined(FEAT_SEARCH_EXTRA)
    /* reset got_int, otherwise regexp won't work */
    save_got_int = got_int;
    got_int = 0;
#endif
#ifdef FEAT_FOLDING
    win_foldinfo.fi_level = 0;
#endif

    /*
     * Update all the window rows.
     */
    idx = 0;		/* first entry in w_lines[].wl_size */
    row = 0;
    srow = 0;
    lnum = wp->w_topline;	/* first line shown in window */
    for (;;)
    {
	/* stop updating when reached the end of the window (check for _past_
	 * the end of the window is at the end of the loop) */
	if (row == wp->w_height)
	{
	    didline = TRUE;
	    break;
	}

	/* stop updating when hit the end of the file */
	if (lnum > buf->b_ml.ml_line_count)
	{
	    eof = TRUE;
	    break;
	}

	/* Remember the starting row of the line that is going to be dealt
	 * with.  It is used further down when the line doesn't fit. */
	srow = row;

	/*
	 * Update a line when it is in an area that needs updating, when it
	 * has changes or w_lines[idx] is invalid.
	 * bot_start may be halfway a wrapped line after using
	 * win_del_lines(), check if the current line includes it.
	 * When syntax folding is being used, the saved syntax states will
	 * already have been updated, we can't see where the syntax state is
	 * the same again, just update until the end of the window.
	 */
	if (row < top_end
		|| (row >= mid_start && row < mid_end)
#ifdef FEAT_SEARCH_EXTRA
		|| top_to_mod
#endif
		|| idx >= wp->w_lines_valid
		|| (row + wp->w_lines[idx].wl_size > bot_start)
		|| (mod_top != 0
		    && (lnum == mod_top
			|| (lnum >= mod_top
			    && (lnum < mod_bot
#ifdef FEAT_SYN_HL
				|| did_update == DID_FOLD
				|| (did_update == DID_LINE
				    && syntax_present(buf)
				    && (
# ifdef FEAT_FOLDING
					(foldmethodIsSyntax(wp)
						      && hasAnyFolding(wp)) ||
# endif
					syntax_check_changed(lnum)))
#endif
				)))))
	{
#ifdef FEAT_SEARCH_EXTRA
	    if (lnum == mod_top)
		top_to_mod = FALSE;
#endif

	    /*
	     * When at start of changed lines: May scroll following lines
	     * up or down to minimize redrawing.
	     * Don't do this when the change continues until the end.
	     * Don't scroll when dollar_vcol is non-zero, keep the "$".
	     */
	    if (lnum == mod_top
		    && mod_bot != MAXLNUM
		    && !(dollar_vcol != 0 && mod_bot == mod_top + 1))
	    {
		int		old_rows = 0;
		int		new_rows = 0;
		int		xtra_rows;
		linenr_T	l;

		/* Count the old number of window rows, using w_lines[], which
		 * should still contain the sizes for the lines as they are
		 * currently displayed. */
		for (i = idx; i < wp->w_lines_valid; ++i)
		{
		    /* Only valid lines have a meaningful wl_lnum.  Invalid
		     * lines are part of the changed area. */
		    if (wp->w_lines[i].wl_valid
			    && wp->w_lines[i].wl_lnum == mod_bot)
			break;
		    old_rows += wp->w_lines[i].wl_size;
#ifdef FEAT_FOLDING
		    if (wp->w_lines[i].wl_valid
			    && wp->w_lines[i].wl_lastlnum + 1 == mod_bot)
		    {
			/* Must have found the last valid entry above mod_bot.
			 * Add following invalid entries. */
			++i;
			while (i < wp->w_lines_valid
						  && !wp->w_lines[i].wl_valid)
			    old_rows += wp->w_lines[i++].wl_size;
			break;
		    }
#endif
		}

		if (i >= wp->w_lines_valid)
		{
		    /* When buffer lines have been inserted/deleted, and
		     * insering/deleting window lines is not possible, need to
		     * check for redraw until the end of the window.  This is
		     * also required when w_topline changed. */
		    if (buf->b_mod_xlines != 0
			    || (wp->w_topline == mod_top
				&& wp->w_lines_valid > 0
				&& wp->w_lines[0].wl_lnum != mod_top))
			bot_start = 0;
		}
		else
		{
		    /* Able to count old number of rows: Count new window
		     * rows, and may insert/delete lines */
		    j = idx;
		    for (l = lnum; l < mod_bot; ++l)
		    {
#ifdef FEAT_FOLDING
			if (hasFoldingWin(wp, l, NULL, &l, TRUE, NULL))
			    ++new_rows;
			else
#endif
#ifdef FEAT_DIFF
			    if (l == wp->w_topline)
			    new_rows += plines_win_nofill(wp, l, TRUE)
							      + wp->w_topfill;
			else
#endif
			    new_rows += plines_win(wp, l, TRUE);
			++j;
			if (new_rows > wp->w_height - row - 2)
			{
			    /* it's getting too much, must redraw the rest */
			    new_rows = 9999;
			    break;
			}
		    }
		    xtra_rows = new_rows - old_rows;
		    if (xtra_rows < 0)
		    {
			/* May scroll text up.  If there is not enough
			 * remaining text or scrolling fails, must redraw the
			 * rest.  If scrolling works, must redraw the text
			 * below the scrolled text. */
			if (row - xtra_rows >= wp->w_height - 2)
			    mod_bot = MAXLNUM;
			else
			{
			    check_for_delay(FALSE);
			    if (win_del_lines(wp, row,
					    -xtra_rows, FALSE, FALSE) == FAIL)
				mod_bot = MAXLNUM;
			    else
				bot_start = wp->w_height + xtra_rows;
			}
		    }
		    else if (xtra_rows > 0)
		    {
			/* May scroll text down.  If there is not enough
			 * remaining text of scrolling fails, must redraw the
			 * rest. */
			if (row + xtra_rows >= wp->w_height - 2)
			    mod_bot = MAXLNUM;
			else
			{
			    check_for_delay(FALSE);
			    if (win_ins_lines(wp, row + old_rows,
					     xtra_rows, FALSE, FALSE) == FAIL)
				mod_bot = MAXLNUM;
			}
		    }

		    /* When not updating the rest, may need to move w_lines[]
		     * entries. */
		    if (mod_bot != MAXLNUM && i != j)
		    {
			if (j < i)
			{
			    int x = row + new_rows;

			    /* move entries in w_lines[] upwards */
			    for (;;)
			    {
				/* stop at last valid entry in w_lines[] */
				if (i >= wp->w_lines_valid)
				{
				    wp->w_lines_valid = j;
				    break;
				}
				wp->w_lines[j] = wp->w_lines[i];
				/* stop at a line that won't fit */
				if (x + (int)wp->w_lines[j].wl_size
							   > wp->w_height)
				{
				    wp->w_lines_valid = j + 1;
				    break;
				}
				x += wp->w_lines[j++].wl_size;
				++i;
			    }
			    if (bot_start > x)
				bot_start = x;
			}
			else /* j > i */
			{
			    /* move entries in w_lines[] downwards */
			    j -= i;
			    wp->w_lines_valid += j;
			    if (wp->w_lines_valid > wp->w_height)
				wp->w_lines_valid = wp->w_height;
			    for (i = wp->w_lines_valid; i - j >= idx; --i)
				wp->w_lines[i] = wp->w_lines[i - j];
			}
		    }
		}

		/* When inserting or deleting lines and 'number' is set:
		 * Redraw all lines below the change to update the line
		 * numbers. */
		if (buf->b_mod_xlines != 0 && wp->w_p_nu)
		    bot_start = 0;
	    }

#ifdef FEAT_FOLDING
	    /*
	     * When lines are folded, display one line for all of them.
	     * Otherwise, display normally (can be several display lines when
	     * 'wrap' is on).
	     */
	    fold_count = foldedCount(wp, lnum, &win_foldinfo);
	    if (fold_count != 0)
	    {
		fold_line(wp, fold_count, &win_foldinfo, lnum, row);
		++row;
		--fold_count;
		wp->w_lines[idx].wl_folded = TRUE;
		wp->w_lines[idx].wl_lastlnum = lnum + fold_count;
#ifdef FEAT_SYN_HL
		did_update = DID_FOLD;
#endif
	    }
	    else
#endif
	    {
#ifdef FEAT_SEARCH_EXTRA
		prepare_search_hl(wp, lnum);
#endif
#ifdef FEAT_SYN_HL
		/* Let the syntax stuff know we skipped a few lines. */
		if (syntax_last_parsed != 0 && syntax_last_parsed + 1 < lnum
						       && syntax_present(buf))
		    syntax_end_parsing(syntax_last_parsed + 1);
#endif

		/*
		 * Display one line.
		 */
		row = win_line(wp, lnum, srow, wp->w_height);

#ifdef FEAT_FOLDING
		wp->w_lines[idx].wl_folded = FALSE;
		wp->w_lines[idx].wl_lastlnum = lnum;
#endif
#ifdef FEAT_SYN_HL
		did_update = DID_LINE;
		syntax_last_parsed = lnum;
#endif
	    }

	    wp->w_lines[idx].wl_lnum = lnum;
	    wp->w_lines[idx].wl_valid = TRUE;
	    if (row > wp->w_height)	/* past end of screen */
	    {
		/* we may need the size of that too long line later on */
		if (dollar_vcol == 0)
		    wp->w_lines[idx].wl_size = plines_win(wp, lnum, TRUE);
		++idx;
		break;
	    }
	    if (dollar_vcol == 0)
		wp->w_lines[idx].wl_size = row - srow;
	    ++idx;
#ifdef FEAT_FOLDING
	    lnum += fold_count + 1;
#else
	    ++lnum;
#endif
	}
	else
	{
	    /* This line does not need updating, advance to the next one */
	    row += wp->w_lines[idx++].wl_size;
	    if (row > wp->w_height)	/* past end of screen */
		break;
#ifdef FEAT_FOLDING
	    lnum = wp->w_lines[idx - 1].wl_lastlnum + 1;
#else
	    ++lnum;
#endif
#ifdef FEAT_SYN_HL
	    did_update = DID_NONE;
#endif
	}

	if (lnum > buf->b_ml.ml_line_count)
	{
	    eof = TRUE;
	    break;
	}
    }
    /*
     * End of loop over all window lines.
     */


    if (idx > wp->w_lines_valid)
	wp->w_lines_valid = idx;

#ifdef FEAT_SYN_HL
    /*
     * Let the syntax stuff know we stop parsing here.
     */
    if (syntax_last_parsed != 0 && syntax_present(buf))
	syntax_end_parsing(syntax_last_parsed + 1);
#endif

    /*
     * If we didn't hit the end of the file, and we didn't finish the last
     * line we were working on, then the line didn't fit.
     */
    wp->w_empty_rows = 0;
#ifdef FEAT_DIFF
    wp->w_filler_rows = 0;
#endif
    if (!eof && !didline)
    {
	if (lnum == wp->w_topline)
	{
	    /*
	     * Single line that does not fit!
	     * Don't overwrite it, it can be edited.
	     */
	    wp->w_botline = lnum + 1;
	}
#ifdef FEAT_DIFF
	else if (diff_check_fill(wp, lnum) >= wp->w_height - srow)
	{
	    /* Window ends in filler lines. */
	    wp->w_botline = lnum;
	    wp->w_filler_rows = wp->w_height - srow;
	}
#endif
	else if (dy_flags & DY_LASTLINE)	/* 'display' has "lastline" */
	{
	    /*
	     * Last line isn't finished: Display "@@@" at the end.
	     */
	    screen_fill(W_WINROW(wp) + wp->w_height - 1,
		    W_WINROW(wp) + wp->w_height,
		    (int)W_ENDCOL(wp) - 3, (int)W_ENDCOL(wp),
		    '@', '@', hl_attr(HLF_AT));
	    wp->w_botline = lnum;
	}
	else
	{
	    win_draw_end(wp, '@', srow);
	    wp->w_botline = lnum;
	}
    }
    else
    {
#ifdef FEAT_VERTSPLIT
	draw_vsep_win(wp, row);
#endif
	/* make sure the rest of the screen is blank */
	/* put '~'s on rows that aren't part of the file. */
	win_draw_end(wp, '~', row);

	if (eof)		/* we hit the end of the file */
	    wp->w_botline = buf->b_ml.ml_line_count + 1;
	else if (dollar_vcol == 0)
	    wp->w_botline = lnum;
    }

    /* Reset the type of redrawing required, the window has been updated. */
    wp->w_redr_type = 0;
#ifdef FEAT_DIFF
    wp->w_old_topfill = wp->w_topfill;
#endif

    if (dollar_vcol == 0)
    {
	/*
	 * There is a trick with w_botline.  If we invalidate it on each
	 * change that might modify it, this will cause a lot of expensive
	 * calls to plines() in update_topline() each time.  Therefore the
	 * value of w_botline is often approximated, and this value is used to
	 * compute the value of w_topline.  If the value of w_botline was
	 * wrong, check that the value of w_topline is correct (cursor is on
	 * the visible part of the text).  If it's not, we need to redraw
	 * again.  Mostly this just means scrolling up a few lines, so it
	 * doesn't look too bad.  Only do this for the current window (where
	 * changes are relevant).
	 */
	wp->w_valid |= VALID_BOTLINE;
	if (wp == curwin && wp->w_botline != old_botline && !recursive)
	{
	    recursive = TRUE;
	    curwin->w_valid &= ~VALID_TOPLINE;
	    update_topline();	/* may invalidate w_botline again */
	    if (must_redraw != 0)
	    {
		/* Don't update for changes in buffer again. */
		i = curbuf->b_mod_set;
		curbuf->b_mod_set = FALSE;
		win_update(curwin);
		must_redraw = 0;
		curbuf->b_mod_set = i;
	    }
	    recursive = FALSE;
	}
    }

#if defined(FEAT_SYN_HL) || defined(FEAT_SEARCH_EXTRA)
    /* restore got_int, unless CTRL-C was hit while redrawing */
    if (!got_int)
	got_int = save_got_int;
#endif
}

/*
 * Clear the rest of the window and mark the unused lines with "c".
 */
    static void
win_draw_end(wp, c, row)
    win_T	*wp;
    int		c;
    int		row;
{
#if defined(FEAT_FOLDING) || defined(FEAT_CMDWIN)
    int		n = 0;
# define FDC_OFF n
#else
# define FDC_OFF 0
#endif

#ifdef FEAT_RIGHTLEFT
    if (wp->w_p_rl)
    {
	/* No check for cmdline window: should never be right-left. */
# ifdef FEAT_FOLDING
	n = wp->w_p_fdc;

	if (n > 0)
	{
	    /* draw the fold column at the right */
	    if (n > wp->w_width)
		n = wp->w_width;
	    screen_fill(W_WINROW(wp) + row, W_WINROW(wp) + wp->w_height,
		    W_ENDCOL(wp) - n, (int)W_ENDCOL(wp),
		    ' ', ' ', hl_attr(HLF_FC));
	}
# endif
	screen_fill(W_WINROW(wp) + row, W_WINROW(wp) + wp->w_height,
		W_WINCOL(wp), W_ENDCOL(wp) - 1 - FDC_OFF,
		' ', ' ', hl_attr(HLF_AT));
	screen_fill(W_WINROW(wp) + row, W_WINROW(wp) + wp->w_height,
		W_ENDCOL(wp) - 1 - FDC_OFF, W_ENDCOL(wp) - FDC_OFF,
		c, ' ', hl_attr(HLF_AT));
    }
    else
#endif
    {
#ifdef FEAT_CMDWIN
	if (cmdwin_type != 0 && wp == curwin)
	{
	    /* draw the cmdline character in the leftmost column */
	    n = 1;
	    if (n > wp->w_width)
		n = wp->w_width;
	    screen_fill(W_WINROW(wp) + row, W_WINROW(wp) + wp->w_height,
		    W_WINCOL(wp), (int)W_WINCOL(wp) + n,
		    cmdwin_type, ' ', hl_attr(HLF_AT));
	}
#endif
#ifdef FEAT_FOLDING
	if (wp->w_p_fdc > 0)
	{
	    int	    nn = n + wp->w_p_fdc;

	    /* draw the fold column at the left */
	    if (nn > W_WIDTH(wp))
		nn = W_WIDTH(wp);
	    screen_fill(W_WINROW(wp) + row, W_WINROW(wp) + wp->w_height,
		    W_WINCOL(wp) + n, (int)W_WINCOL(wp) + nn,
		    ' ', ' ', hl_attr(HLF_FC));
	    n = nn;
	}
#endif
	screen_fill(W_WINROW(wp) + row, W_WINROW(wp) + wp->w_height,
		W_WINCOL(wp) + FDC_OFF, (int)W_ENDCOL(wp),
		c, ' ', hl_attr(HLF_AT));
    }
    set_empty_rows(wp, row);
}

#ifdef FEAT_FOLDING
/*
 * Display one folded line.
 */
    static void
fold_line(wp, fold_count, foldinfo, lnum, row)
    win_T	*wp;
    long	fold_count;
    foldinfo_T	*foldinfo;
    linenr_T	lnum;
    int		row;
{
    char_u	buf[51];
    char_u	dashes[51];
    pos_T	*top, *bot;
    linenr_T	lnume = lnum + fold_count - 1;
    int		len;
    char_u	*p;
    char_u	*text = NULL;
    int		fdc;
    win_T	*save_curwin;
    int		level;
    int		col;
    int		txtcol;
    int		off = current_ScreenLine - ScreenLines;

    /* Build the fold line:
     * 1. Add the cmdwin_type for the command-line window
     * 2. Add the 'foldcolumn'
     * 3. Add the 'number' column
     * 4. Compose the text
     * 5. Add the text
     * 6. set highlighting for the Visual area an other text
     */
    col = 0;

    /*
     * 1. Add the cmdwin_type for the command-line window
     * Ignores 'rightleft', this window is never right-left.
     */
#ifdef FEAT_CMDWIN
    if (cmdwin_type != 0 && wp == curwin)
    {
	ScreenLines[off] = cmdwin_type;
	ScreenAttrs[off] = hl_attr(HLF_AT);
#ifdef FEAT_MBYTE
	if (enc_utf8)
	    ScreenLinesUC[off] = 0;
#endif
	++col;
    }
#endif

    /*
     * 2. Add the 'foldcolumn'
     */
    fdc = wp->w_p_fdc;
    if (fdc > W_WIDTH(wp) - col)
	fdc = W_WIDTH(wp) - col;
    if (fdc > 0)
    {
	fill_foldcolumn(buf, wp, TRUE, lnum);
#ifdef FEAT_RIGHTLEFT
	if (wp->w_p_rl)
	{
	    int		i;

	    copy_text_attr(off + W_WIDTH(wp) - fdc - col, buf, fdc,
							     hl_attr(HLF_FC));
	    /* reverse the fold column */
	    for (i = 0; i < fdc; ++i)
		ScreenLines[off + W_WIDTH(wp) - i - 1 - col] = buf[i];
	}
	else
#endif
	    copy_text_attr(off + col, buf, fdc, hl_attr(HLF_FC));
	col += fdc;
    }

#ifdef FEAT_RIGHTLEFT
# define RL_MEMSET(p, v, l) vim_memset(ScreenAttrs + off + (wp->w_p_rl ? (W_WIDTH(wp) - (p) - (l)) : (p)), v, l)
#else
# define RL_MEMSET(p, v, l) vim_memset(ScreenAttrs + off + p, v, l)
#endif

    /* Set all attributes of the 'number' column and the text */
    RL_MEMSET(col, hl_attr(HLF_FL), (size_t)(W_WIDTH(wp) - col));

    /*
     * 3. Add the 'number' column
     */
    if (wp->w_p_nu)
    {
	len = W_WIDTH(wp) - col;
	if (len > 0)
	{
	    if (len > 8)
		len = 8;
	    sprintf((char *)buf, "%7ld ", (long)lnum);
#ifdef FEAT_RIGHTLEFT
	    if (wp->w_p_rl)
		/* the line number isn't reversed */
		copy_text_attr(off + W_WIDTH(wp) - len - col, buf, len,
							     hl_attr(HLF_FL));
	    else
#endif
		copy_text_attr(off + col, buf, len, hl_attr(HLF_FL));
	    col += len;
	}
    }

    /*
     * 4. Compose the folded-line string with 'foldtext', if set.
     */
    if (*wp->w_p_fdt != NUL)
    {
	/* Set "v:foldstart" and "v:foldend". */
	set_vim_var_nr(VV_FOLDSTART, lnum);
	set_vim_var_nr(VV_FOLDEND, lnume);

	/* Set "v:folddashes" to a string of "level" dashes. */
	level = foldinfo->fi_level;
	if (level > 50)
	    level = 50;
	vim_memset(dashes, '-', (size_t)level);
	dashes[level] = NUL;
	set_vim_var_string(VV_FOLDDASHES, dashes, -1);
	save_curwin = curwin;
	curwin = wp;
	curbuf = wp->w_buffer;

	++emsg_off;
	text = eval_to_string_safe(wp->w_p_fdt, NULL);
	--emsg_off;

	curwin = save_curwin;
	curbuf = curwin->w_buffer;
	set_vim_var_string(VV_FOLDDASHES, NULL, -1);

	if (text != NULL)
	{
	    /* Replace unprintable characters, if there are any.  But
	     * replace a TAB with a space. */
	    for (p = text; *p != NUL; ++p)
	    {
#ifdef FEAT_MBYTE
		if (has_mbyte && (len = (*mb_ptr2len_check)(p)) > 1)
		    p += len - 1;
		else
#endif
		    if (*p == TAB)
			*p = ' ';
		    else if (ptr2cells(p) > 1)
			break;
	    }
	    if (*p != NUL)
	    {
		p = transstr(text);
		vim_free(text);
		text = p;
	    }
	}
    }
    if (text == NULL)
    {
	sprintf((char *)buf, _("+--%3ld lines folded "), fold_count);
	text = buf;
    }

    txtcol = col;	/* remember where text starts */

    /*
     * 5. move the text to current_ScreenLine.  Fill up with "fill_fold".
     *    Right-left text is put in columns 0 - number-col, normal text is put
     *    in columns number-col - window-width.
     */
#ifdef FEAT_MBYTE
    if (has_mbyte)
    {
	int	cells;
	int	u8c, u8c_c1, u8c_c2;
	int	idx;

# ifdef FEAT_RIGHTLEFT
	if (wp->w_p_rl)
	    idx = off;
	else
# endif
	    idx = off + col;

	/* Store multibyte characters in ScreenLines[] et al. correctly. */
	for (p = text; *p != NUL; )
	{
	    cells = (*mb_ptr2cells)(p);
	    if (col + cells > W_WIDTH(wp)
# ifdef FEAT_RIGHTLEFT
		    - (wp->w_p_rl ? col : 0)
# endif
		    )
		break;
	    ScreenLines[idx] = *p;
	    if (enc_utf8)
	    {
		u8c = utfc_ptr2char(p, &u8c_c1, &u8c_c2);
		if (*p < 0x80 && u8c_c1 == 0 && u8c_c2 == 0)
		    ScreenLinesUC[idx] = 0;
		else
		{
		    ScreenLinesUC[idx] = u8c;
		    ScreenLinesC1[idx] = u8c_c1;
		    ScreenLinesC2[idx] = u8c_c2;
		}
		if (cells > 1)
		    ScreenLines[idx + 1] = 0;
	    }
	    else if (cells > 1)	    /* double-byte character */
	    {
		if (enc_dbcs == DBCS_JPNU && *p == 0x8e)
		    ScreenLines2[idx] = p[1];
		else
		    ScreenLines[idx + 1] = p[1];
	    }
	    col += cells;
	    idx += cells;
	    p += (*mb_ptr2len_check)(p);
	}
    }
    else
#endif
    {
	len = STRLEN(text);
	if (len > W_WIDTH(wp) - col)
	    len = W_WIDTH(wp) - col;
	if (len > 0)
	{
#ifdef FEAT_RIGHTLEFT
	    if (wp->w_p_rl)
		STRNCPY(current_ScreenLine, text, len);
	    else
#endif
		STRNCPY(current_ScreenLine + col, text, len);
	    col += len;
	}
    }

    /* Fill the rest of the line with the fold filler */
#ifdef FEAT_RIGHTLEFT
    if (wp->w_p_rl)
	col -= txtcol;
#endif
    while (col < W_WIDTH(wp)
#ifdef FEAT_RIGHTLEFT
		    - (wp->w_p_rl ? txtcol : 0)
#endif
	    )
    {
#ifdef FEAT_MBYTE
	if (enc_utf8)
	    ScreenLinesUC[off + col] = 0;
#endif
	ScreenLines[off + col++] = fill_fold;
    }

    if (text != buf)
	vim_free(text);

    /*
     * 6. set highlighting for the Visual area an other text.
     * If all folded lines are in the Visual area, highlight the line.
     */
#ifdef FEAT_VISUAL
    if (VIsual_active && wp->w_buffer == curwin->w_buffer)
    {
	if (ltoreq(curwin->w_cursor, VIsual))
	{
	    /* Visual is after curwin->w_cursor */
	    top = &curwin->w_cursor;
	    bot = &VIsual;
	}
	else
	{
	    /* Visual is before curwin->w_cursor */
	    top = &VIsual;
	    bot = &curwin->w_cursor;
	}
	if (lnum >= top->lnum
		&& lnume <= bot->lnum
		&& (VIsual_mode != 'v'
		    || ((lnum > top->lnum
			    || (lnum == top->lnum
				&& top->col == 0))
			&& (lnume < bot->lnum
			    || (lnume == bot->lnum
				&& (bot->col - (*p_sel == 'e'))
		>= STRLEN(ml_get_buf(wp->w_buffer, lnume, FALSE)))))))
	{
	    if (VIsual_mode == Ctrl_V)
	    {
		/* Visual block mode: highlight the chars part of the block */
		if (wp->w_old_cursor_fcol + txtcol < (colnr_T)W_WIDTH(wp))
		{
		    if (wp->w_old_cursor_lcol + txtcol < (colnr_T)W_WIDTH(wp))
			len = wp->w_old_cursor_lcol;
		    else
			len = W_WIDTH(wp) - txtcol;
		    RL_MEMSET(wp->w_old_cursor_fcol + txtcol, hl_attr(HLF_V),
				       (size_t)(len - wp->w_old_cursor_fcol));
		}
	    }
	    else
		/* Set all attributes of the text */
		RL_MEMSET(txtcol, hl_attr(HLF_V),
					      (size_t)(W_WIDTH(wp) - txtcol));
	}
    }
#endif


    SCREEN_LINE(row + W_WINROW(wp), W_WINCOL(wp), (int)W_WIDTH(wp),
						     (int)W_WIDTH(wp), FALSE);

    /*
     * Update w_cline_height and w_cline_folded if the cursor line was
     * updated (saves a call to plines() later).
     */
    if (wp == curwin
	    && lnum <= curwin->w_cursor.lnum
	    && lnume >= curwin->w_cursor.lnum)
    {
	curwin->w_cline_row = row;
	curwin->w_cline_height = 1;
	curwin->w_cline_folded = TRUE;
	curwin->w_valid |= (VALID_CHEIGHT|VALID_CROW);
    }
}

/*
 * Copy "buf[len]" to ScreenLines["off"] and set attributes to "attr".
 */
    static void
copy_text_attr(off, buf, len, attr)
    int		off;
    char_u	*buf;
    int		len;
    int		attr;
{
    mch_memmove(ScreenLines + off, buf, (size_t)len);
# ifdef FEAT_MBYTE
    if (enc_utf8)
	vim_memset(ScreenLinesUC + off, 0, sizeof(u8char_T) * (size_t)len);
# endif
    vim_memset(ScreenAttrs + off, attr, (size_t)len);
}

/*
 * Fill the foldcolumn at "p" for window "wp".
 */
    static void
fill_foldcolumn(p, wp, closed, lnum)
    char_u	*p;
    win_T	*wp;
    int		closed;		/* TRUE of FALSE */
    linenr_T	lnum;		/* current line number */
{
    int		i = 0;
    int		level;
    int		first_level;

    /* Init to all spaces. */
    copy_spaces(p, (size_t)wp->w_p_fdc);

    level = win_foldinfo.fi_level;
    if (level > 0)
    {
	/* If the column is too narrow, we start at the lowest level that
	 * fits and use numbers to indicated the depth. */
	first_level = level - wp->w_p_fdc - closed + 2;
	if (first_level < 1)
	    first_level = 1;

	for (i = 0; i + 1 < wp->w_p_fdc; ++i)
	{
	    if (win_foldinfo.fi_lnum == lnum
			      && first_level + i >= win_foldinfo.fi_low_level)
		p[i] = '-';
	    else if (first_level == 1)
		p[i] = '|';
	    else if (first_level + i <= 9)
		p[i] = '0' + first_level + i;
	    else
		p[i] = '>';
	    if (first_level + i == level)
		break;
	}
    }
    if (closed)
	p[i] = '+';
}
#endif /* FEAT_FOLDING */

/*
 * Display line "lnum" of window 'wp' on the screen.
 * Start at row "startrow", stop when "endrow" is reached.
 * wp->w_virtcol needs to be valid.
 *
 * Return the number of last row the line occupies.
 */
    static int
win_line(wp, lnum, startrow, endrow)
    win_T	*wp;
    linenr_T	lnum;
    int		startrow;
    int		endrow;
{
    int		col;			/* visual column on screen */
    unsigned	off;			/* offset in ScreenLines/ScreenAttrs */
    int		c = 0;			/* init for GCC */
    long	vcol = 0;		/* virtual column (for tabs) */
    long	vcol_prev = -1;		/* "vcol" of previous character */
    char_u	*line;			/* current line */
    char_u	*ptr;			/* current position in "line" */
    int		row;			/* row in the window, excl w_winrow */
    int		screen_row;		/* row on the screen, incl w_winrow */

    char_u	extra[18];		/* "%ld" and 'fdc' must fit in here */
    int		n_extra = 0;		/* number of extra chars */
    char_u	*p_extra = NULL;	/* string of extra chars */
    int		c_extra = NUL;		/* extra chars, all the same */
    int		extra_attr = 0;		/* attributes when n_extra != 0 */
    static char_u *at_end_str = (char_u *)""; /* used for p_extra when
					   displaying lcs_eol at end-of-line */
    int		lcs_eol_one = lcs_eol;	/* lcs_eol until it's been used */

    /* saved "extra" items for when draw_state becomes WL_LINE (again) */
    int		saved_n_extra = 0;
    char_u	*saved_p_extra = NULL;
    int		saved_c_extra = 0;
    int		saved_char_attr = 0;

    int		n_attr = 0;		/* chars with special attr */
    int		saved_attr2 = 0;	/* char_attr saved for n_attr */

    int		n_skip = 0;		/* nr of chars to skip for 'nowrap' */

    int		fromcol, tocol;		/* start/end of inverting */
    int		noinvcur = FALSE;	/* don't invert the cursor */
#ifdef FEAT_VISUAL
    pos_T	*top, *bot;
#endif
    pos_T	pos;
    long	v;

    int		char_attr = 0;		/* attributes for next character */
    int		area_highlighting = FALSE; /* Visual or incsearch highlighting
					      in this line */
    int		attr = 0;		/* attributes for area highlighting */
    int		area_attr = 0;		/* attributes desired by highlighting */
    int		search_attr = 0;	/* attributes desired by 'hlsearch' */
#ifdef FEAT_SYN_HL
    int		syntax_attr = 0;	/* attributes desired by syntax */
    int		has_syntax = FALSE;	/* this buffer has syntax highl. */
#endif
    int		extra_check;		/* has syntax or linebreak */
#ifdef FEAT_MBYTE
    int		multi_attr = 0;		/* attributes desired by multibyte */
    int		mb_l = 1;		/* multi-byte byte length */
    int		mb_c = 0;		/* decoded multi-byte character */
    int		mb_utf8 = FALSE;	/* screen char is UTF-8 char */
    int		u8c_c1 = 0;		/* first composing UTF-8 char */
    int		u8c_c2 = 0;		/* second composing UTF-8 char */
#endif
#ifdef FEAT_DIFF
    int		filler_lines;		/* nr of filler lines to be drawn */
    int		filler_todo;		/* nr of filler lines still to do + 1 */
    int		diff_attr = 0;		/* attributes for changed/added line */
    int		change_start = MAXCOL;	/* first col of changed area */
    int		change_end = -1;	/* last col of changed area */
#endif
    colnr_T	trailcol = MAXCOL;	/* start of trailing spaces */
#ifdef FEAT_LINEBREAK
    int		need_showbreak = FALSE;
#endif
#ifdef FEAT_SIGNS
    int_u	type;			/* sign type (if signs are used) */
#endif
#if defined(FEAT_SIGNS) || (defined(FEAT_QUICKFIX) && defined(FEAT_WINDOWS))
# define LINE_ATTR
    int		line_attr = 0;		/* atrribute for the whole line */
#endif
#ifdef FEAT_SEARCH_EXTRA
    match_T	*shl;			/* points to search_hl or match_hl */
#endif

    /* draw_state: items that are drawn in sequence: */
#define WL_START	0		/* nothing done yet */
#ifdef FEAT_CMDWIN
# define WL_CMDLINE	WL_START + 1	/* cmdline window column */
#else
# define WL_CMDLINE	WL_START
#endif
#ifdef FEAT_FOLDING
# define WL_FOLD	WL_CMDLINE + 1	/* 'foldcolumn' */
#else
# define WL_FOLD	WL_CMDLINE
#endif
#ifdef FEAT_SIGNS
# define WL_SIGN	WL_FOLD + 1	/* column for signs */
#else
# define WL_SIGN	WL_FOLD		/* column for signs */
#endif
#define WL_NR		WL_SIGN + 1	/* line number */
#if defined(FEAT_LINEBREAK) || defined(FEAT_DIFF)
# define WL_SBR		WL_NR + 1	/* 'showbreak' or 'diff' */
#else
# define WL_SBR		WL_NR
#endif
#define WL_LINE		WL_SBR + 1	/* text in the line */
    int		draw_state = WL_START;	/* what to draw next */


    if (startrow > endrow)		/* past the end already! */
	return startrow;

    row = startrow;
    screen_row = row + W_WINROW(wp);

    /*
     * To speed up the loop below, set extra_check when there is linebreak,
     * trailing white space and/or syntax processing to be done.
     */
#ifdef FEAT_LINEBREAK
    extra_check = wp->w_p_lbr;
#else
    extra_check = 0;
#endif
#ifdef FEAT_SYN_HL
    if (syntax_present(wp->w_buffer))
    {
	syntax_start(wp, lnum);
	has_syntax = TRUE;
	extra_check = TRUE;
    }
#endif

    /*
     * handle visual active in this window
     */
    fromcol = -10;
    tocol = MAXCOL;
#ifdef FEAT_VISUAL
    if (VIsual_active && wp->w_buffer == curwin->w_buffer)
    {
					/* Visual is after curwin->w_cursor */
	if (ltoreq(curwin->w_cursor, VIsual))
	{
	    top = &curwin->w_cursor;
	    bot = &VIsual;
	}
	else				/* Visual is before curwin->w_cursor */
	{
	    top = &VIsual;
	    bot = &curwin->w_cursor;
	}
	if (VIsual_mode == Ctrl_V)	/* block mode */
	{
	    if (lnum >= top->lnum && lnum <= bot->lnum)
	    {
		fromcol = wp->w_old_cursor_fcol;
		tocol = wp->w_old_cursor_lcol;
	    }
	}
	else				/* non-block mode */
	{
	    if (lnum > top->lnum && lnum <= bot->lnum)
		fromcol = 0;
	    else if (lnum == top->lnum)
	    {
		if (VIsual_mode == 'V')	/* linewise */
		    fromcol = 0;
		else
		    getvvcol(wp, top, (colnr_T *)&fromcol, NULL, NULL);
	    }
	    if (VIsual_mode != 'V' && lnum == bot->lnum)
	    {
		if (*p_sel == 'e' && bot->col == 0)
		{
		    fromcol = -10;
		    tocol = MAXCOL;
		}
		else
		{
		    pos = *bot;
		    if (*p_sel == 'e')
		    {
			--pos.col;
#ifdef FEAT_MBYTE
			if (has_mbyte)
			{
			    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
			    pos.col -= (*mb_head_off)(line, line + pos.col);
			}
#endif
		    }
		    getvvcol(wp, &pos, NULL, NULL, (colnr_T *)&tocol);
		    ++tocol;
		}
	    }
	}

#ifndef MSDOS
	/* Check if the character under the cursor should not be inverted */
	if (!highlight_match && lnum == curwin->w_cursor.lnum && wp == curwin
# ifdef FEAT_GUI
		&& !gui.in_use
# endif
		)
	    noinvcur = TRUE;
#endif

	/* if inverting in this line set area_highlighting */
	if (fromcol >= 0)
	{
	    area_highlighting = TRUE;
	    attr = hl_attr(HLF_V);
#if defined(FEAT_CLIPBOARD) && defined(FEAT_X11)
	    if (clip_star.available && !clip_star.owned && clip_isautosel())
		attr = hl_attr(HLF_VNC);
#endif
	}
    }

    /*
     * handle 'insearch' and ":s///c" highlighting
     */
    else
#endif /* FEAT_VISUAL */
	if (highlight_match
	    && wp == curwin
	    && lnum >= curwin->w_cursor.lnum
	    && lnum <= curwin->w_cursor.lnum + search_match_lines)
    {
	if (lnum == curwin->w_cursor.lnum)
	    getvcol(curwin, &(curwin->w_cursor),
					     (colnr_T *)&fromcol, NULL, NULL);
	else
	    fromcol = 0;
	if (lnum == curwin->w_cursor.lnum + search_match_lines)
	{
	    pos.lnum = lnum;
	    pos.col = search_match_endcol;
	    getvcol(curwin, &pos, (colnr_T *)&tocol, NULL, NULL);
	}
	else
	    tocol = MAXCOL;
	if (fromcol == tocol)		/* do at least one character */
	    tocol = fromcol + 1;	/* happens when past end of line */
	area_highlighting = TRUE;
	attr = hl_attr(HLF_I);
    }

#ifdef FEAT_DIFF
    filler_lines = diff_check(wp, lnum);
    if (filler_lines < 0)
    {
	if (filler_lines == -1)
	{
	    diff_find_change(wp, lnum, &change_start, &change_end);
	    if (change_start == 0)
		diff_attr = hl_attr(HLF_TXD);	/* changed text */
	    else
		diff_attr = hl_attr(HLF_CHD);	/* changed line */
	}
	else
	    diff_attr = hl_attr(HLF_ADD);	/* added line */
	filler_lines = 0;
	area_highlighting = TRUE;
    }
    if (lnum == wp->w_topline)
	filler_lines = wp->w_topfill;
    filler_todo = filler_lines;
#endif

#ifdef LINE_ATTR
# ifdef FEAT_SIGNS
    /* Draw a sign at the start of the line and/or highlight the line. */
    type = buf_getsigntype(wp->w_buffer, lnum);
    if (type != 0)
	line_attr = get_debug_highlight(type);
# endif
# if defined(FEAT_QUICKFIX) && defined(FEAT_WINDOWS)
    /* Highlight the current line in the quickfix window. */
    if (bt_quickfix(wp->w_buffer) && qf_current_entry() == lnum)
	line_attr = hl_attr(HLF_L);
# endif
    if (line_attr != 0)
	area_highlighting = TRUE;
#endif

    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
    ptr = line;

    /* find start of trailing whitespace */
    if (wp->w_p_list && lcs_trail)
    {
	trailcol = STRLEN(ptr);
	while (trailcol > (colnr_T)0 && vim_iswhite(ptr[trailcol - 1]))
	    --trailcol;
	trailcol += ptr - line;
	extra_check = TRUE;
    }

    /*
     * 'nowrap' or 'wrap' and a single line that doesn't fit: Advance to the
     * first character to be displayed.
     */
    if (wp->w_p_wrap)
	v = wp->w_skipcol;
    else
	v = wp->w_leftcol;
    if (v > 0)
    {
#ifdef FEAT_MBYTE
	char_u	*prev_ptr = ptr;
#endif
	while (vcol < v && *ptr != NUL)
	{
	    c = win_lbr_chartabsize(wp, ptr, (colnr_T)vcol, NULL);
	    vcol += c;
#ifdef FEAT_MBYTE
	    prev_ptr = ptr;
	    if (has_mbyte)
		ptr += (*mb_ptr2len_check)(ptr);
	    else
#endif
		++ptr;
	}
	/* Handle a character that's not completely on the screen: Put ptr at
	 * that character but skip the first few screen characters. */
	if (vcol > v)
	{
	    vcol -= c;
#ifdef FEAT_MBYTE
	    ptr = prev_ptr;
#else
	    --ptr;
#endif
	    n_skip = v - vcol;
	}

	/*
	 * Adjust for when the inverted text is before the screen,
	 * and when the start of the inverted text is before the screen.
	 */
	if (tocol <= vcol)
	    fromcol = 0;
	else if (fromcol >= 0 && fromcol < vcol)
	    fromcol = vcol;

#ifdef FEAT_LINEBREAK
	/* When w_skipcol is non-zero, first line needs 'showbreak' */
	if (wp->w_p_wrap)
	    need_showbreak = TRUE;
#endif
    }

#ifdef FEAT_SEARCH_EXTRA
    /*
     * Handle highlighting the last used search pattern.
     * Do this for both search_hl and match_hl.
     */
    shl = &search_hl;
    for (;;)
    {
	shl->startp = NULL;
	shl->endp = NULL;
	shl->attr_cur = 0;
	if (shl->rm.regprog != NULL)
	{
	    v = ptr - line;
	    next_search_hl(wp, shl, lnum, (colnr_T)v);

	    /* Need to get the line again, a multi-line regexp may have made it
	     * invalid. */
	    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
	    ptr = line + v;

	    if (shl->lnum != 0 && shl->lnum <= lnum)
	    {
		if (shl->lnum == lnum)
		    shl->startp = line + shl->rm.startpos[0].col;
		else
		    shl->startp = line;
		if (lnum == shl->lnum + shl->rm.endpos[0].lnum
						  - shl->rm.startpos[0].lnum)
		    shl->endp = line + shl->rm.endpos[0].col;
		else
		    shl->endp = line + MAXCOL;
		/* Highlight one character for an empty match. */
#if 0
		if (*shl->startp == NUL && shl->startp > line)
		{
#ifdef FEAT_MBYTE
		    if (has_mbyte)
			shl->startp = mb_prevptr(line, shl->startp);
		    else
#endif
			--shl->startp;
		}
		else
#endif
		    if (shl->startp == shl->endp)
		{
#ifdef FEAT_MBYTE
		    if (has_mbyte)
			shl->endp += (*mb_ptr2len_check)(shl->endp);
		    else
#endif
			++shl->endp;
		}
		if (shl->startp < ptr)  /* match at leftcol */
		{
		    shl->attr_cur = shl->attr;
		    search_attr = shl->attr;
		}
		area_highlighting = TRUE;
	    }
	}
	if (shl == &match_hl)
	    break;
	shl = &match_hl;
    }
#endif

    off = current_ScreenLine - ScreenLines;
    col = 0;
#ifdef FEAT_RIGHTLEFT
    if (wp->w_p_rl)
    {
	/* Rightleft window: process the text in the normal direction, but put
	 * it in current_ScreenLine[] from right to left.  Start at the
	 * rightmost column of the window. */
	col = W_WIDTH(wp) - 1;
	off += col;
    }
#endif

    /*
     * Repeat for the whole displayed line.
     */
    for (;;)
    {
	/* Skip this quickly when working on the text. */
	if (draw_state != WL_LINE)
	{
#ifdef FEAT_CMDWIN
	    if (draw_state == WL_CMDLINE - 1 && n_extra == 0)
	    {
		draw_state = WL_CMDLINE;
		if (cmdwin_type != 0 && wp == curwin)
		{
		    /* Draw the cmdline character. */
		    *extra = cmdwin_type;
		    n_extra = 1;
		    p_extra = extra;
		    c_extra = NUL;
		    char_attr = hl_attr(HLF_AT);
		}
	    }
#endif

#ifdef FEAT_FOLDING
	    if (draw_state == WL_FOLD - 1 && n_extra == 0)
	    {
		draw_state = WL_FOLD;
		if (wp->w_p_fdc > 0)
		{
		    /* Draw the 'foldcolumn'. */
		    fill_foldcolumn(extra, wp, FALSE, lnum);
		    n_extra = wp->w_p_fdc;
		    p_extra = extra;
		    c_extra = NUL;
		    char_attr = hl_attr(HLF_FC);
		}
	    }
#endif

#ifdef FEAT_SIGNS
	    if (draw_state == WL_SIGN - 1 && n_extra == 0)
	    {
		draw_state = WL_SIGN;
		if (wp->w_buffer->b_signlist != NULL
# ifdef FEAT_DIFF
			&& filler_todo <= 0
# endif
			)
		{
		    /* Draw two spaces to put the sign in. */
		    c_extra = ' ';
		    n_extra = 2;
		    char_attr = 0;
		}
	    }
#endif

	    if (draw_state == WL_NR - 1 && n_extra == 0)
	    {
		draw_state = WL_NR;
		/* Display the line number.  After the first fill with blanks
		 * when the 'n' flag isn't in 'cpo' */
		if (wp->w_p_nu
			&& (row == startrow
#ifdef FEAT_DIFF
			    + filler_lines
#endif
			    || vim_strchr(p_cpo, CPO_NUMCOL) == NULL))
		{
		    /* Draw the line number (empty space after wrapping). */
		    if (row == startrow
#ifdef FEAT_DIFF
			    + filler_lines
#endif
			    )
		    {
			sprintf((char *)extra, "%7ld ", (long)lnum);
			if (wp->w_skipcol > 0)
			    for (p_extra = extra; *p_extra == ' '; ++p_extra)
				*p_extra = '-';
#ifdef FEAT_RIGHTLEFT
			if (wp->w_p_rl)		    /* reverse line numbers */
			{
			    char_u *c1, *c2, t;

			    for (c1 = extra, c2 = extra + STRLEN(extra) - 1;
				    c1 < c2; c1++, c2--)
			    {
				t = *c1;
				*c1 = *c2;
				*c2 = t;
			    }
			}
#endif
			p_extra = extra;
			c_extra = NUL;
		    }
		    else
			c_extra = ' ';
		    n_extra = 8;
		    char_attr = hl_attr(HLF_N);
		}
	    }

#if defined(FEAT_LINEBREAK) || defined(FEAT_DIFF)
	    if (draw_state == WL_SBR - 1 && n_extra == 0)
	    {
		draw_state = WL_SBR;
# ifdef FEAT_DIFF
		if (filler_todo > 0)
		{
		    /* Draw "deleted" diff line(s). */
		    if (vim_iswhite(fill_diff) || char2cells(fill_diff) > 1)
			c_extra = 'x';
		    else
			c_extra = fill_diff;
		    n_extra = W_WIDTH(wp) - col;
		    char_attr = hl_attr(HLF_DED);
		}
# endif
# ifdef FEAT_LINEBREAK
		if (*p_sbr != NUL && need_showbreak)
		{
		    /* Draw 'showbreak' at the start of each broken line. */
		    p_extra = p_sbr;
		    c_extra = NUL;
		    n_extra = STRLEN(p_sbr);
		    char_attr = hl_attr(HLF_AT);
		    need_showbreak = FALSE;
		}
# endif
	    }
#endif

	    if (draw_state == WL_LINE - 1 && n_extra == 0)
	    {
		draw_state = WL_LINE;
		if (saved_n_extra)
		{
		    /* Continue item from end of wrapped line. */
		    n_extra = saved_n_extra;
		    c_extra = saved_c_extra;
		    p_extra = saved_p_extra;
		    char_attr = saved_char_attr;
		}
		else
		    char_attr = 0;
	    }
	}

	/* When still displaying '$' of change command, stop at cursor */
	if (dollar_vcol != 0 && wp == curwin && vcol >= (long)wp->w_virtcol
#ifdef FEAT_VIRTUALEDIT
						   + (long)wp->w_cursor.coladd
#endif
#ifdef FEAT_DIFF
				   && filler_todo <= 0
#endif
		)
	{
	    SCREEN_LINE(screen_row, W_WINCOL(wp), col, -(int)W_WIDTH(wp),
								  wp->w_p_rl);
	    /* Pretend we have finished updating the window. */
	    row = wp->w_height;
	    break;
	}

	if (draw_state == WL_LINE && area_highlighting)
	{
	    /* handle Visual or match highlighting in this line */
	    if (((vcol == fromcol
			    && !(noinvcur
				&& (colnr_T)vcol == wp->w_virtcol
#ifdef FEAT_VIRTUALEDIT
							 + wp->w_cursor.coladd
#endif
				))
			|| (noinvcur
			    && (colnr_T)vcol_prev == wp->w_virtcol
#ifdef FEAT_VIRTUALEDIT
							 + wp->w_cursor.coladd
#endif
			    && vcol >= fromcol))
		    && vcol < tocol)
		area_attr = attr;		/* start highlighting */
	    else if (area_attr
		    && (vcol == tocol
			|| (noinvcur
			    && (colnr_T)vcol == wp->w_virtcol
#ifdef FEAT_VIRTUALEDIT
							 + wp->w_cursor.coladd
#endif
			    )))
#ifdef LINE_ATTR
		area_attr = line_attr;		/* stop highlighting */
	    else if (line_attr && ((fromcol == -10 && tocol == MAXCOL)
					 || (vcol < fromcol || vcol > tocol)))
		area_attr = line_attr;
#else
		area_attr = 0;			/* stop highlighting */
#endif

#ifdef FEAT_SEARCH_EXTRA
	    if (!n_extra)
	    {
		/*
		 * Check for start/end of search pattern match.
		 * After end, check for start/end of next match.
		 * When another match, have to check for start again.
		 * Watch out for matching an empty string!
		 * Do this first for search_hl, then for match_hl, so that
		 * ":match" overrules 'hlsearch'.
		 */
		shl = &search_hl;
		for (;;)
		{
		    for (;;)
		    {
			if (shl->startp != NULL
				&& ptr >= shl->startp
				&& ptr < shl->endp)
			{
			    shl->attr_cur = shl->attr;
			}
			else if (ptr == shl->endp)
			{
			    shl->attr_cur = 0;

			    v = ptr - line;
			    next_search_hl(wp, shl, lnum, (colnr_T)v);

			    /* Need to get the line again, a multi-line regexp
			     * may have made it invalid. */
			    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
			    ptr = line + v;

			    if (shl->lnum == lnum)
			    {
				shl->startp = line + shl->rm.startpos[0].col;
				if (shl->rm.endpos[0].lnum == 0)
				    shl->endp = line + shl->rm.endpos[0].col;
				else
				    shl->endp = line + MAXCOL;
#if 0
				/* Matching end-of-line: highlight last
				 * character in the line. */
				if (*shl->startp == NUL && shl->startp > line)
				{
#ifdef FEAT_MBYTE
				    if (has_mbyte)
					shl->startp =
						mb_prevptr(line, shl->startp);
				    else
#endif
					--shl->startp;
				}
#endif

				/* for a non-null match, loop to check if the
				 * match starts at the current position */
				if (shl->startp != shl->endp)
				    continue;

				/* highlight empty match, try again after it */
#ifdef FEAT_MBYTE
				if (has_mbyte)
				    shl->endp += (*mb_ptr2len_check)(shl->endp);
				else
#endif
				    ++shl->endp;
			    }
			}
			break;
		    }
		    if (shl == &match_hl)
			break;
		    shl = &match_hl;
		}
		/* ":match" highlighting overrules 'hlsearch' */
		if (match_hl.attr_cur != 0)
		    search_attr = match_hl.attr_cur;
		else
		    search_attr = search_hl.attr_cur;
	    }
#endif

	    if (area_attr != 0)
		char_attr = area_attr;
#ifdef FEAT_SYN_HL
	    else if (search_attr == 0 && has_syntax)
		char_attr = syntax_attr;
#endif
	    else
		char_attr = search_attr;

#ifdef FEAT_DIFF
	    if (diff_attr != 0 && n_extra == 0)
	    {
		if (diff_attr == hl_attr(HLF_CHD) && ptr - line >= change_start)
		    diff_attr = hl_attr(HLF_TXD);	/* changed text */
		if (diff_attr == hl_attr(HLF_TXD) && ptr - line > change_end)
		    diff_attr = hl_attr(HLF_CHD);	/* changed line */
		if (attr == 0 || area_attr != attr)
		    area_attr = diff_attr;
		if (attr == 0 || char_attr != attr)
		    char_attr = diff_attr;
	    }
#endif
	}

	/*
	 * Get the next character to put on the screen.
	 */
	/*
	 * The 'extra' array contains the extra stuff that is inserted to
	 * represent special characters (non-printable stuff).  When all
	 * characters are the same, c_extra is used.
	 * For the '$' of the 'list' option, n_extra == 1, p_extra == "".
	 */
	if (n_extra > 0)
	{
	    if (c_extra != NUL)
	    {
		c = c_extra;
#ifdef FEAT_MBYTE
		mb_c = c;	/* doesn't handle multi-byte! */
#endif
	    }
	    else
	    {
		c = *p_extra;
#ifdef FEAT_MBYTE
		if (has_mbyte)
		{
		    mb_c = c;
		    if (enc_utf8)
		    {
			/* If the UTF-8 character is more than one byte:
			 * Decode it into "mb_c". */
			mb_l = (*mb_ptr2len_check)(p_extra);
			mb_utf8 = FALSE;
			if (mb_l > n_extra)
			    mb_l = 1;
			else if (mb_l > 1)
			{
			    mb_c = utfc_ptr2char(p_extra, &u8c_c1, &u8c_c2);
			    mb_utf8 = TRUE;
			}
		    }
		    else
		    {
			/* if this is a DBCS character, put it in "mb_c" */
			mb_l = MB_BYTE2LEN(c);
			if (mb_l >= n_extra)
			    mb_l = 1;
			else if (mb_l > 1)
			    mb_c = (c << 8) + p_extra[1];
		    }
		    /* If a double-width char doesn't fit display a '>' in the
		     * last column. */
		    if (
# ifdef FEAT_RIGHTLEFT
			    wp->w_p_rl ? (col <= 0) :
# endif
				    (col >= W_WIDTH(wp) - 1)
			    && (*mb_char2cells)(mb_c) == 2)
		    {
			c = '>';
			mb_c = c;
			mb_l = 1;
			mb_utf8 = FALSE;
			multi_attr = hl_attr(HLF_AT);
			/* put the pointer back to output the double-width
			 * character at the start of the next line. */
			++n_extra;
			--p_extra;
		    }
		    else
		    {
			n_extra -= mb_l - 1;
			p_extra += mb_l - 1;
		    }
		}
#endif
		++p_extra;
	    }
	    --n_extra;
	}
	else
	{
	    /*
	     * Get a character from the line itself.
	     */
	    c = *ptr;
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		mb_c = c;
		if (enc_utf8)
		{
		    /* If the UTF-8 character is more than one byte: Decode it
		     * into "mb_c". */
		    mb_l = (*mb_ptr2len_check)(ptr);
		    mb_utf8 = FALSE;
		    if (mb_l > 1)
		    {
			mb_c = utfc_ptr2char(ptr, &u8c_c1, &u8c_c2);
			/* Overlong encoded ASCII or ASCII with composing char
			 * is displayed normally, except a NUL. */
			if (mb_c < 0x80)
			    c = mb_c;
			mb_utf8 = TRUE;
		    }
		    if ((mb_l == 1 && c >= 0x80)
			    || (mb_l >= 1 && mb_c == 0)
			    || (mb_l > 1 && !vim_isprintc(mb_c)))
		    {
			/*
			 * Illegal UTF-8 byte: display as <xx>.
			 */
			transchar_hex(extra, mb_c);
			p_extra = extra;
			c = *p_extra++;
			mb_c = c;
			mb_utf8 = FALSE;
			n_extra = STRLEN(p_extra);
			c_extra = NUL;
			if (area_attr == 0 && search_attr == 0)
			{
			    n_attr = n_extra + 1;
			    extra_attr = hl_attr(HLF_8);
			    saved_attr2 = char_attr; /* save current attr */
			}
		    }
		    else if (mb_l == 0)  /* at the NUL at end-of-line */
			mb_l = 1;
		}
		else	/* enc_dbcs */
		{
		    mb_l = MB_BYTE2LEN(c);
		    if (mb_l == 0)  /* at the NUL at end-of-line */
			mb_l = 1;
		    else if (mb_l > 1)
		    {
			if (ptr[1] != NUL)
			    mb_c = (c << 8) + ptr[1];
			else
			{
			    /* head byte at end of line */
			    mb_l = 1;
			    transchar_nonprint(extra, c);
			    p_extra = extra;
			    n_extra = STRLEN(extra) - 1;
			    c_extra = NUL;
			    c = *p_extra++;
			    if (area_attr == 0 && search_attr == 0)
			    {
				n_attr = n_extra + 1;
				extra_attr = hl_attr(HLF_8);
				saved_attr2 = char_attr; /* save current attr */
			    }
			    mb_c = c;
			}
		    }
		}
		/* If a double-width char doesn't fit display a '>' in the
		 * last column; the character is displayed at the start of the
		 * next line. */
		if ((
# ifdef FEAT_RIGHTLEFT
			    wp->w_p_rl ? (col <= 0) :
# endif
				(col >= W_WIDTH(wp) - 1))
			&& (*mb_char2cells)(mb_c) == 2)
		{
		    c = '>';
		    mb_c = c;
		    mb_utf8 = FALSE;
		    mb_l = 1;
		    multi_attr = hl_attr(HLF_AT);
		    /* Put pointer back so that the character will be
		     * displayed at the start of the next line. */
		    --ptr;
		}
		else
		    ptr += mb_l - 1;

	    }
#endif
	    ++ptr;

	    if (extra_check)
	    {
#ifdef FEAT_SYN_HL
		if (has_syntax)
		{
		    v = ptr - line;
		    syntax_attr = get_syntax_attr((colnr_T)v - 1);

		    /* Need to get the line again, a multi-line regexp may
		     * have made it invalid. */
		    line = ml_get_buf(wp->w_buffer, lnum, FALSE);
		    ptr = line + v;

		    if (area_attr == 0 && search_attr == 0)
			char_attr = syntax_attr;
		}
#endif
#ifdef FEAT_LINEBREAK
		/*
		 * Found last space before word: check for line break
		 */
		if (wp->w_p_lbr && vim_isbreak(c) && !vim_isbreak(*ptr)
						      && !wp->w_p_list)
		{
		    n_extra = win_lbr_chartabsize(wp, ptr - (
# ifdef FEAT_MBYTE
				has_mbyte ? mb_l :
# endif
				1), (colnr_T)vcol, NULL) - 1;
		    c_extra = ' ';
		    if (vim_iswhite(c))
			c = ' ';
		}
#endif

		if (trailcol != MAXCOL && ptr > line + trailcol && c == ' ')
		{
		    c = lcs_trail;
		    if (area_attr == 0 && search_attr == 0)
		    {
			n_attr = 1;
			extra_attr = hl_attr(HLF_8);
			saved_attr2 = char_attr; /* save current attr */
		    }
		}
	    }

	    /*
	     * Handling of non-printable characters.
	     */
	    if (!(chartab[c] & CT_PRINT_CHAR))
	    {
		/*
		 * when getting a character from the file, we may have to
		 * turn it into something else on the way to putting it
		 * into "ScreenLines".
		 */
		if (c == TAB && (!wp->w_p_list || lcs_tab1))
		{
		    /* tab amount depends on current column */
		    n_extra = (int)wp->w_buffer->b_p_ts
				   - vcol % (int)wp->w_buffer->b_p_ts - 1;
		    if (wp->w_p_list)
		    {
			c = lcs_tab1;
			c_extra = lcs_tab2;
			if (area_attr == 0 && search_attr == 0)
			{
			    n_attr = n_extra + 1;
			    extra_attr = hl_attr(HLF_8);
			    saved_attr2 = char_attr; /* save current attr */
			}
		    }
		    else
		    {
			c_extra = ' ';
			c = ' ';
		    }
#ifdef FEAT_MBYTE
		    mb_utf8 = FALSE;	/* don't draw as UTF-8 */
#endif
		}
		else if (c == NUL && wp->w_p_list && lcs_eol_one != NUL)
		{
#ifdef FEAT_DIFF
		    /* For a diff line the highlighting continues after the
		     * "$". */
		    if (diff_attr == 0)
#endif
		    {
			p_extra = at_end_str;
			n_extra = 1;
			c_extra = NUL;
		    }
		    c = lcs_eol;
		    lcs_eol_one = NUL;
		    --ptr;	    /* put it back at the NUL */
		    if (area_attr == 0 && search_attr == 0)
		    {
			extra_attr = hl_attr(HLF_AT);
			n_attr = 1;
		    }
#ifdef FEAT_MBYTE
		    mb_utf8 = FALSE;	/* don't draw as UTF-8 */
#endif
		}
		else if (c != NUL)
		{
		    p_extra = transchar(c);
		    n_extra = byte2cells(c) - 1;
		    c_extra = NUL;
		    c = *p_extra++;
		    if (area_attr == 0 && search_attr == 0)
		    {
			n_attr = n_extra + 1;
			extra_attr = hl_attr(HLF_8);
			saved_attr2 = char_attr; /* save current attr */
		    }
#ifdef FEAT_MBYTE
		    mb_utf8 = FALSE;	/* don't draw as UTF-8 */
#endif
		}
#ifdef FEAT_VIRTUALEDIT
		else if (VIsual_active
			 && VIsual_mode == Ctrl_V
			 && (ve_flags & VE_BLOCK)
			 && tocol != MAXCOL
			 && vcol < tocol
			 && (
# ifdef FEAT_RIGHTLEFT
			    wp->w_p_rl ? (col >= 0) :
# endif
			    (col < W_WIDTH(wp))))
		{
		    c = ' ';
		    --ptr;	    /* put it back at the NUL */
		}
#endif
#ifdef FEAT_DIFF
		else if (diff_attr != 0 && col < W_WIDTH(wp))
		{
		    /* Highlight until the right side of the window */
		    c = ' ';
		    --ptr;	    /* put it back at the NUL */
		    if (diff_attr == hl_attr(HLF_TXD))
		    {
			diff_attr = hl_attr(HLF_CHD);
			if (attr == 0 || char_attr != attr)
			    char_attr = diff_attr;
		    }
		}
#endif
	    }
	}


	/*
	 * Handle the case where we are in column 0 but not on the first
	 * character of the line and the user wants us to show us a
	 * special character (via 'listchars' option "precedes:<char>".
	 */
	if (lcs_prec != NUL
		&& (wp->w_p_wrap ? wp->w_skipcol > 0 : wp->w_leftcol > 0)
#ifdef FEAT_DIFF
		&& filler_todo <= 0
#endif
		&& (
#ifdef FEAT_RIGHTLEFT
		    wp->w_p_rl ? col == W_WIDTH(wp) - 1 :
#endif
		    col == 0)
		&& c != NUL)
	{
	    c = lcs_prec;
	    extra_attr = hl_attr(HLF_AT); /* later copied to char_attr */
	    n_attr = 1;
	    saved_attr2 = char_attr; /* save current attr */
#ifdef FEAT_MBYTE
	    mb_utf8 = FALSE;	/* don't draw as UTF-8 */
#endif
	}

	if (n_attr > 0)
	    char_attr = extra_attr;

	/*
	 * At end of the text line.
	 */
	if (c == NUL)
	{
	    /* invert at least one char, used for Visual and empty line or
	     * highlight match at end of line. If it's beyond the last
	     * char on the screen, just overwrite that one (tricky!) */
	    if ((area_attr != 0 && vcol == fromcol)
#ifdef FEAT_SEARCH_EXTRA
		    /* highlight 'hlsearch' match at end of line */
		    || ptr - 1 == search_hl.startp
		    || ptr - 1 == match_hl.startp
#endif
		    )
	    {
#ifdef FEAT_RIGHTLEFT
		if (wp->w_p_rl)
		{
		    if (col < 0)
		    {
			++off;
			++col;
		    }
		}
		else
#endif
		{
		    if (col >= W_WIDTH(wp))
		    {
			--off;
			--col;
		    }
		}
		ScreenLines[off] = ' ';
#ifdef FEAT_SEARCH_EXTRA
		if (area_attr == 0)
		{
		    if (ptr - 1 == match_hl.startp)
			char_attr = match_hl.attr;
		    else
			char_attr = search_hl.attr;
		}
#endif
		ScreenAttrs[off] = char_attr;
#ifdef FEAT_RIGHTLEFT
		if (wp->w_p_rl)
		    --col;
		else
#endif
		    ++col;
	    }

	    SCREEN_LINE(screen_row, W_WINCOL(wp), col, (int)W_WIDTH(wp),
								  wp->w_p_rl);
	    row++;

	    /*
	     * Update w_cline_height and w_cline_folded if the cursor line was
	     * updated (saves a call to plines() later).
	     */
	    if (wp == curwin && lnum == curwin->w_cursor.lnum)
	    {
		curwin->w_cline_row = startrow;
		curwin->w_cline_height = row - startrow;
#ifdef FEAT_FOLDING
		curwin->w_cline_folded = FALSE;
#endif
		curwin->w_valid |= (VALID_CHEIGHT|VALID_CROW);
	    }

	    break;
	}

	/* line continues beyond line end */
	if (lcs_ext
		&& !wp->w_p_wrap
#ifdef FEAT_DIFF
		&& filler_todo <= 0
#endif
		&& (
#ifdef FEAT_RIGHTLEFT
		    wp->w_p_rl ? col == 0 :
#endif
		    col == W_WIDTH(wp) - 1)
		&& (*ptr != NUL
		    || (wp->w_p_list && lcs_eol != NUL && p_extra != at_end_str)
		    || (n_extra && (c_extra != NUL || *p_extra != NUL))))
	{
	    c = lcs_ext;
	    char_attr = hl_attr(HLF_AT);
	}

	/*
	 * Store character to be displayed.
	 * Skip characters that are left of the screen for 'nowrap'.
	 */
	vcol_prev = vcol;
	if (draw_state < WL_LINE || n_skip <= 0)
	{
	    /*
	     * Store the character.
	     */
	    ScreenLines[off] = c;
#ifdef FEAT_MBYTE
	    if (enc_dbcs == DBCS_JPNU)
		ScreenLines2[off] = mb_c & 0xff;
	    else if (enc_utf8)
	    {
		if (mb_utf8)
		{
		    ScreenLinesUC[off] = mb_c;
		    ScreenLinesC1[off] = u8c_c1;
		    ScreenLinesC2[off] = u8c_c2;
		}
		else
		    ScreenLinesUC[off] = 0;
	    }
	    if (multi_attr)
	    {
		ScreenAttrs[off] = multi_attr;
		multi_attr = 0;
	    }
	    else
#endif
		ScreenAttrs[off] = char_attr;

#ifdef FEAT_MBYTE
	    if (has_mbyte && (*mb_char2cells)(mb_c) > 1)
	    {
		/* Need to advance two screen columns. */
#ifdef FEAT_RIGHTLEFT
		if (wp->w_p_rl)
		{
		    --off;
		    --col;
		}
		else
#endif
		{
		    ++off;
		    ++col;
		}
		if (enc_utf8)
		    /* UTF-8: Put a 0 in the second screen char. */
		    ScreenLines[off] = 0;
		else
		    /* DBCS: Put second byte in the second screen char. */
		    ScreenLines[off] = mb_c & 0xff;
		++vcol;
		/* When "tocol" is halfway a character, set it to the end of
		 * the character, otherwise highlighting won't stop. */
		if (tocol == vcol)
		    ++tocol;
	    }
#endif
#ifdef FEAT_RIGHTLEFT
	    if (wp->w_p_rl)
	    {
		--off;
		--col;
	    }
	    else
#endif
	    {
		++off;
		++col;
	    }
	}
	else
	    --n_skip;

	/* Only advance the "vcol" when after the 'number' column. */
	if (draw_state >= WL_SBR
#ifdef FEAT_DIFF
		&& filler_todo <= 0
#endif
		)
	    ++vcol;

	/* restore attributes after last 'listchars' or 'number' char */
	if (n_attr && --n_attr == 0)
	    char_attr = saved_attr2;

	/*
	 * At end of screen line and there is more to come: Display the line
	 * so far.  If there is no more to display it is catched above.
	 */
	if (
#ifdef FEAT_RIGHTLEFT
	    wp->w_p_rl ? (col < 0) :
#endif
				    (col >= W_WIDTH(wp))
		&& (*ptr != NUL
#ifdef FEAT_DIFF
		    || filler_todo > 0
#endif
		    || (wp->w_p_list && lcs_eol != NUL && p_extra != at_end_str)
		    || (n_extra != 0 && (c_extra != NUL || *p_extra != NUL)))
		)
	{
	    SCREEN_LINE(screen_row, W_WINCOL(wp), col, (int)W_WIDTH(wp),
								  wp->w_p_rl);
	    ++row;
	    ++screen_row;

	    /* When not wrapping and finished diff lines, break here. */
	    if (!wp->w_p_wrap
#ifdef FEAT_DIFF
		    && filler_todo <= 0
#endif
		    )
		break;

	    /* When the window is too narrow draw all "@" lines. */
	    if (draw_state != WL_LINE
#ifdef FEAT_DIFF
		    && filler_todo <= 0
#endif
		    )
	    {
		win_draw_end(wp, '@', row);
#ifdef FEAT_VERTSPLIT
		draw_vsep_win(wp, row);
#endif
		row = endrow;
	    }

	    /* When line got too long for screen break here. */
	    if (row == endrow)
	    {
		++row;
		break;
	    }

	    /*
	     * Special trick to make copy/paste of wrapped lines work with
	     * xterm/screen: write an extra character beyond the end of the
	     * line. This will work with all terminal types (regardless of the
	     * xn,am settings).
	     * Only do this on a fast tty.
	     * Only do this if the cursor is on the current line (something
	     * has been written in it).
	     * Don't do this for the GUI.
	     * Don't do this for double-width characters.
	     * Don't do this for a window not at the right screen border.
	     */
	    if (p_tf && screen_cur_row == screen_row - 1
#ifdef FEAT_GUI
		     && !gui.in_use
#endif
#ifdef FEAT_MBYTE
		     && !(has_mbyte
			 && ((*mb_off2cells)(LineOffset[screen_row]) == 2
			     || (*mb_off2cells)(LineOffset[screen_row - 1]
						    + (int)Columns - 2) == 2))
#endif
		     && W_WIDTH(wp) == Columns)
	    {
		if (screen_cur_col != W_WIDTH(wp))
		    screen_char(LineOffset[screen_row - 1]
						      + (unsigned)Columns - 1,
					  screen_row - 1, (int)(Columns - 1));
		screen_char(LineOffset[screen_row],
						screen_row - 1, (int)Columns);
		screen_start();		/* don't know where cursor is now */
	    }

	    col = 0;
	    off = current_ScreenLine - ScreenLines;
#ifdef FEAT_RIGHTLEFT
	    if (wp->w_p_rl)
	    {
		col = W_WIDTH(wp) - 1;	/* col is not used if breaking! */
		off += col;
	    }
#endif

	    /* reset the drawing state for the start of a wrapped line */
	    draw_state = WL_START;
	    saved_n_extra = n_extra;
	    saved_p_extra = p_extra;
	    saved_c_extra = c_extra;
	    saved_char_attr = char_attr;
	    n_extra = 0;
#ifdef FEAT_LINEBREAK
# ifdef FEAT_DIFF
	    if (filler_todo <= 0)
# endif
		need_showbreak = TRUE;
#endif
#ifdef FEAT_DIFF
	    --filler_todo;
#endif
	}

    }	/* for every character in the line */

    return row;
}

/*
 * Check whether the given character needs redrawing:
 * - the (first byte of the) character is different
 * - the attributes are different
 * - the character is multi-byte and the next byte is different
 */
    static int
char_needs_redraw(off_from, off_to, len)
    int		off_from;
    int		off_to;
    int		len;
{
    if (len > 0
	    && ((ScreenLines[off_from] != ScreenLines[off_to]
		    || ScreenAttrs[off_from] != ScreenAttrs[off_to])

#ifdef FEAT_MBYTE
		|| (enc_dbcs != 0
		    && MB_BYTE2LEN(ScreenLines[off_from]) > 1
		    && len > 1
		    && (enc_dbcs == DBCS_JPNU && ScreenLines[off_from] == 0x8e
			? ScreenLines2[off_from] != ScreenLines2[off_to]
			: ScreenLines[off_from + 1] != ScreenLines[off_to + 1]))
		|| (enc_utf8
		    && (ScreenLinesUC[off_from] != ScreenLinesUC[off_to]
			|| (ScreenLinesUC[off_from] != 0
			    && (ScreenLinesC1[off_from]
						      != ScreenLinesC1[off_to]
				|| ScreenLinesC2[off_from]
						  != ScreenLinesC2[off_to]))))
#endif
	       ))
	return TRUE;
    return FALSE;
}

/*
 * Move one "cooked" screen line to the screen, but only the characters that
 * have actually changed.  Handle insert/delete character.
 * "coloff" gives the first column on the screen for this line.
 * "endcol" gives the columns where valid characters are.
 * "clear_width" is the width of the window.  It's > 0 if the rest of the line
 * needs to be cleared, negative otherwise.
 * "rlflag" is TRUE in a rightleft window:
 *    When TRUE and "clear_width" > 0, clear columns 0 to "endcol"
 *    When FALSE and "clear_width" > 0, clear columns "endcol" to "clear_width"
 */
    static void
screen_line(row, coloff, endcol, clear_width
#ifdef FEAT_RIGHTLEFT
				    , rlflag
#endif
						)
    int	    row;
    int	    coloff;
    int	    endcol;
    int	    clear_width;
#ifdef FEAT_RIGHTLEFT
    int	    rlflag;
#endif
{
    unsigned	    off_from;
    unsigned	    off_to;
    int		    col = 0;
#if defined(FEAT_GUI) || defined(UNIX) || defined(FEAT_VERTSPLIT)
    int		    hl;
#endif
    int		    force = FALSE;	/* force update rest of the line */
    int		    redraw_this		/* bool: does character need redraw? */
#ifdef FEAT_GUI
				= TRUE	/* For GUI when while-loop empty */
#endif
				;
    int		    redraw_next;	/* redraw_this for next character */
#ifdef FEAT_MBYTE
    int		    char_cells;		/* 1: normal char */
					/* 2: occupies two display cells */
# define CHAR_CELLS char_cells
#else
# define CHAR_CELLS 1
#endif

# ifdef FEAT_CLIPBOARD
    clip_may_clear_selection(row, row);
# endif

    off_from = current_ScreenLine - ScreenLines;
    off_to = LineOffset[row] + coloff;

#ifdef FEAT_RIGHTLEFT
    if (rlflag)
    {
	/* Clear rest first, because it's left of the text. */
	if (clear_width > 0)
	{
	    while (col <= endcol && ScreenLines[off_to] == ' '
						  && ScreenAttrs[off_to] == 0)
	    {
		++off_to;
		++col;
	    }
	    if (col <= endcol)
		screen_fill(row, row + 1, col + coloff,
					    endcol + coloff + 1, ' ', ' ', 0);
	}
	col = endcol + 1;
	off_to = LineOffset[row] + col + coloff;
	off_from += col;
	endcol = coloff + (clear_width > 0 ? clear_width : -clear_width);
    }
#endif /* FEAT_RIGHTLEFT */

    redraw_next = char_needs_redraw(off_from, off_to, endcol - col);

    while (col < endcol)
    {
#ifdef FEAT_MBYTE
	if (has_mbyte && (col + 1 < endcol))
	    char_cells = (*mb_off2cells)(off_from);
	else
	    char_cells = 1;
#endif

	redraw_this = redraw_next;
	redraw_next = force || char_needs_redraw(off_from + CHAR_CELLS,
			      off_to + CHAR_CELLS, endcol - col - CHAR_CELLS);

#ifdef FEAT_GUI
	/* If the next character was bold, then redraw the current character to
	 * remove any pixels that might have spilt over into us.  This only
	 * happens in the GUI.
	 */
	if (redraw_next && gui.in_use)
	{
	    hl = ScreenAttrs[off_to + CHAR_CELLS];
	    if (hl > HL_ALL || (hl & HL_BOLD))
		redraw_this = TRUE;
	}
#endif

	if (redraw_this)
	{
	    /*
	     * Special handling when 'xs' termcap flag set (hpterm):
	     * Attributes for characters are stored at the position where the
	     * cursor is when writing the highlighting code.  The
	     * start-highlighting code must be written with the cursor on the
	     * first highlighted character.  The stop-highlighting code must
	     * be written with the cursor just after the last highlighted
	     * character.
	     * Overwriting a character doesn't remove it's highlighting.  Need
	     * to clear the rest of the line, and force redrawing it
	     * completely.
	     */
	    if (       p_wiv
		    && !force
#ifdef FEAT_GUI
		    && !gui.in_use
#endif
		    && ScreenAttrs[off_to] != 0
		    && ScreenAttrs[off_from] != ScreenAttrs[off_to])
	    {
		/*
		 * Need to remove highlighting attributes here.
		 */
		windgoto(row, col + coloff);
		out_str(T_CE);		/* clear rest of this screen line */
		screen_start();		/* don't know where cursor is now */
		force = TRUE;		/* force redraw of rest of the line */
		redraw_next = TRUE;	/* or else next char would miss out */

		/*
		 * If the previous character was highlighted, need to stop
		 * highlighting at this character.
		 */
		if (col + coloff > 0 && ScreenAttrs[off_to - 1] != 0)
		{
		    screen_attr = ScreenAttrs[off_to - 1];
		    term_windgoto(row, col + coloff);
		    screen_stop_highlight();
		}
		else
		    screen_attr = 0;	    /* highlighting has stopped */
	    }
#ifdef FEAT_MBYTE
	    if (enc_dbcs != 0)
	    {
		/* Check if overwriting a double-byte with a single-byte or
		 * the other way around requires another character to be
		 * redrawn.  For UTF-8 this isn't needed, because comparing
		 * ScreenLinesUC[] is sufficient. */
		if (char_cells == 1
			&& col + 1 < endcol
			&& (*mb_off2cells)(off_to) > 1)
		{
		    /* Writing a single-cell character over a double-cell
		     * character: need to redraw the next cell. */
		    ScreenLines[off_to + 1] = 0;
		    redraw_next = TRUE;
		}
		else if (char_cells == 2
			&& col + 2 < endcol
			&& (*mb_off2cells)(off_to) == 1
			&& (*mb_off2cells)(off_to + 1) > 1)
		{
		    /* Writing the second half of a double-cell character over
		     * a double-cell character: need to redraw the second
		     * cell. */
		    ScreenLines[off_to + 2] = 0;
		    redraw_next = TRUE;
		}

		if (enc_dbcs == DBCS_JPNU)
		    ScreenLines2[off_to] = ScreenLines2[off_from];
	    }
#endif

	    ScreenLines[off_to] = ScreenLines[off_from];
#ifdef FEAT_MBYTE
	    if (enc_utf8)
	    {
		ScreenLinesUC[off_to] = ScreenLinesUC[off_from];
		ScreenLinesC1[off_to] = ScreenLinesC1[off_from];
		ScreenLinesC2[off_to] = ScreenLinesC2[off_from];
	    }
	    if (char_cells == 2)
		ScreenLines[off_to + 1] = ScreenLines[off_from + 1];
#endif

#if defined(FEAT_GUI) || defined(UNIX)
	    /* The bold trick makes a single row of pixels appear in the next
	     * character.  When a bold character is removed, the next
	     * character should be redrawn too.  This happens for our own GUI
	     * and for some xterms. */
	    if (
# ifdef FEAT_GUI
		    gui.in_use
# endif
# if defined(FEAT_GUI) && defined(UNIX)
		    ||
# endif
# ifdef UNIX
		    term_is_xterm
# endif
		    )
	    {
		hl = ScreenAttrs[off_to];
		if (hl > HL_ALL || (hl & HL_BOLD))
		    redraw_next = TRUE;
	    }
#endif
	    ScreenAttrs[off_to] = ScreenAttrs[off_from];
#ifdef FEAT_MBYTE
	    if (enc_dbcs != 0 && char_cells == 2)
	    {
		/* just a hack: It makes two bytes of DBCS have same attr */
		ScreenAttrs[off_to + 1] = ScreenAttrs[off_from];
		screen_char_2(off_to, row, col + coloff);
	    }
	    else
#endif
		screen_char(off_to, row, col + coloff);
	}
	else if (  p_wiv
#ifdef FEAT_GUI
		&& !gui.in_use
#endif
		&& col + coloff > 0)
	{
	    if (ScreenAttrs[off_to] == ScreenAttrs[off_to - 1])
	    {
		/*
		 * Don't output stop-highlight when moving the cursor, it will
		 * stop the highlighting when it should continue.
		 */
		screen_attr = 0;
	    }
	    else if (screen_attr != 0)
		screen_stop_highlight();
	}

	off_to += CHAR_CELLS;
	off_from += CHAR_CELLS;
	col += CHAR_CELLS;
    }

    if (clear_width > 0
#ifdef FEAT_RIGHTLEFT
		    && !rlflag
#endif
				   )
    {
#ifdef FEAT_GUI
	int startCol = col;
#endif

	/* blank out the rest of the line */
	while (col < clear_width && ScreenLines[off_to] == ' '
						  && ScreenAttrs[off_to] == 0)
	{
	    ++off_to;
	    ++col;
	}
	if (col < clear_width)
	{
#ifdef FEAT_GUI
	    /*
	     * In the GUI, clearing the rest of the line may leave pixels
	     * behind if the first character cleared was bold.  Some bold
	     * fonts spill over the left.  In this case we redraw the previous
	     * character too.  If we didn't skip any blanks above, then we
	     * only redraw if the character wasn't already redrawn anyway.
	     */
	    if (gui.in_use && (col > startCol || !redraw_this)
# ifdef FEAT_MBYTE
		    && enc_dbcs == 0
# endif
	       )
	    {
		hl = ScreenAttrs[off_to];
		if (hl > HL_ALL || (hl & HL_BOLD))
		    screen_char(off_to - 1, row, col + coloff - 1);
	    }
#endif
	    screen_fill(row, row + 1, col + coloff, clear_width + coloff,
								 ' ', ' ', 0);
#ifdef FEAT_VERTSPLIT
	    off_to += clear_width - col;
	    col = clear_width;
#endif
	}
    }

#ifdef FEAT_VERTSPLIT
    if (clear_width > 0)
    {
	/* For a window that's left of another, draw the separator char. */
	if (col + coloff < Columns)
	{
	    int c;

	    c = fillchar_vsep(&hl);
	    if (ScreenLines[off_to] != c || ScreenAttrs[off_to] != hl)
	    {
		ScreenLines[off_to] = c;
		ScreenAttrs[off_to] = hl;
		screen_char(off_to, row, col + coloff);
	    }
	}
    }
#endif
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
/*
 * mark all status lines for redraw; used after first :cd
 */
    void
status_redraw_all()
{
    win_T	*wp;

    for (wp = firstwin; wp; wp = wp->w_next)
	if (wp->w_status_height)
	{
	    wp->w_redr_status = TRUE;
	    redraw_later(VALID);
	}
}

/*
 * mark all status lines of the current buffer for redraw
 */
    void
status_redraw_curbuf()
{
    win_T	*wp;

    for (wp = firstwin; wp; wp = wp->w_next)
	if (wp->w_status_height != 0 && wp->w_buffer == curbuf)
	{
	    wp->w_redr_status = TRUE;
	    redraw_later(VALID);
	}
}

/*
 * Redraw all status lines that need to be redrawn.
 */
    void
redraw_statuslines()
{
    win_T	*wp;

    for (wp = firstwin; wp; wp = wp->w_next)
	if (wp->w_redr_status)
	    win_redr_status(wp);
}
#endif

#if defined(FEAT_WILDMENU) && defined(FEAT_VERTSPLIT)
/*
 * Redraw all status lines at the bottom of frame "frp".
 */
    static void
win_redraw_last_status(frp)
    frame_T	*frp;
{
    if (frp->fr_layout == FR_LEAF)
	frp->fr_win->w_redr_status = TRUE;
    else if (frp->fr_layout == FR_ROW)
    {
	for (frp = frp->fr_child; frp != NULL; frp = frp->fr_next)
	    win_redraw_last_status(frp);
    }
    else /* frp->fr_layout == FR_COL */
    {
	frp = frp->fr_child;
	while (frp->fr_next != NULL)
	    frp = frp->fr_next;
	win_redraw_last_status(frp);
    }
}
#endif

#ifdef FEAT_VERTSPLIT
/*
 * Draw the verticap separator right of window "wp" starting with line "row".
 */
    static void
draw_vsep_win(wp, row)
    win_T	*wp;
    int		row;
{
    int		hl;
    int		c;

    if (wp->w_vsep_width)
    {
	/* draw the vertical separator right of this window */
	c = fillchar_vsep(&hl);
	screen_fill(W_WINROW(wp) + row, W_WINROW(wp) + wp->w_height,
		W_ENDCOL(wp), W_ENDCOL(wp) + 1,
		c, ' ', hl);
    }
}
#endif

#ifdef FEAT_WILDMENU
/*
 * Private gettail for win_redr_status_matches(): Find tail of file name path
 * but ignore trailing "/".
 */
static char_u *wm_gettail __ARGS((char_u *s));
static int status_match_len __ARGS((expand_T *xp, char_u *s));

    static char_u *
wm_gettail(s)
    char_u	*s;
{
    char_u	*p;
    char_u	*t = s;
    int		had_sep = FALSE;

    for (p = s; *p != NUL; )
    {
	if (vim_ispathsep(*p))
	    had_sep = TRUE;
	else if (had_sep)
	{
	    t = p;
	    had_sep = FALSE;
	}
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    p += (*mb_ptr2len_check)(p);
	else
#endif
	    ++p;
    }
    return t;
}

/*
 * Get the lenght of an item as it will be shown in that status line.
 */
    static int
status_match_len(xp, s)
    expand_T	*xp;
    char_u	*s;
{
    int	len = 0;

#ifdef FEAT_MENU
    int emenu = (xp->xp_context == EXPAND_MENUS
	    || xp->xp_context == EXPAND_MENUNAMES);

    /* Check for menu separators - replace with '|'. */
    if (emenu && menu_is_separator(s))
	return 1;
#endif

    while (*s != NUL)
    {
	/* Don't display backslashes used for escaping, they look ugly. */
	if (rem_backslash(s)
#ifdef FEAT_MENU
		|| (emenu && (s[0] == '\\' && s[1] != NUL))
#endif
		)
	    ++s;
#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    len += ptr2cells(s);
	    s += (*mb_ptr2len_check)(s);
	}
	else
#endif
	    len += ptr2cells(s++);

    }

    return len;
}

/*
 * Show wildchar matches in the status line.
 * Show at least the "match" item.
 * We start at item 'first_match' in the list and show all matches that fit.
 *
 * If inversion is possible we use it. Else '=' characters are used.
 */
    void
win_redr_status_matches(xp, num_matches, matches, match)
    expand_T	*xp;
    int		num_matches;
    char_u	**matches;	/* list of matches */
    int		match;
{
#define L_MATCH(m) (fmatch ? wm_gettail(matches[m]) : matches[m])
    int		fmatch = (xp->xp_context == EXPAND_FILES);

    int		row;
    char_u	*buf;
    int		len;
    int		clen;		/* lenght in screen cells */
    int		fillchar;
    int		attr;
    int		i;
    int		highlight = TRUE;
    char_u	*selstart = NULL;
    int		selstart_col = 0;
    char_u	*selend = NULL;
    static int	first_match = 0;
    int		add_left = FALSE;
    char_u	*s;
#ifdef FEAT_MENU
    int		emenu;
#endif
#if defined(FEAT_MBYTE) || defined(FEAT_MENU)
    int		l;
#endif

    if (matches == NULL)	/* interrupted completion? */
	return;

    buf = alloc((unsigned)Columns + 1);
    if (buf == NULL)
	return;

    if (match == -1)	/* don't show match but original text */
    {
	match = 0;
	highlight = FALSE;
    }
    /* count 1 for the ending ">" */
    clen = status_match_len(xp, L_MATCH(match)) + 3;
    if (match == 0)
	first_match = 0;
    else if (match < first_match)
    {
	/* jumping left, as far as we can go */
	first_match = match;
	add_left = TRUE;
    }
    else
    {
	/* check if match fits on the screen */
	for (i = first_match; i < match; ++i)
	    clen += status_match_len(xp, L_MATCH(i)) + 2;
	if (first_match > 0)
	    clen += 2;
	/* jumping right, put match at the left */
	if ((long)clen > Columns)
	{
	    first_match = match;
	    /* if showing the last match, we can add some on the left */
	    clen = 2;
	    for (i = match; i < num_matches; ++i)
	    {
		clen += status_match_len(xp, L_MATCH(i)) + 2;
		if ((long)clen >= Columns)
		    break;
	    }
	    if (i == num_matches)
		add_left = TRUE;
	}
    }
    if (add_left)
	while (first_match > 0)
	{
	    clen += status_match_len(xp, L_MATCH(first_match - 1)) + 2;
	    if ((long)clen >= Columns)
		break;
	    --first_match;
	}

    fillchar = fillchar_status(&attr, TRUE);

    if (first_match == 0)
    {
	*buf = NUL;
	len = 0;
    }
    else
    {
	STRCPY(buf, "< ");
	len = 2;
    }
    clen = len;

    i = first_match;
    while ((long)(clen + status_match_len(xp, L_MATCH(i)) + 2) < Columns)
    {
	if (i == match)
	{
	    selstart = buf + len;
	    selstart_col = clen;
	}

	s = L_MATCH(i);
	/* Check for menu separators - replace with '|' */
#ifdef FEAT_MENU
	emenu = (xp->xp_context == EXPAND_MENUS
		|| xp->xp_context == EXPAND_MENUNAMES);
	if (emenu && menu_is_separator(s))
	{
	    STRCPY(buf + len, transchar('|'));
	    l = STRLEN(buf + len);
	    len += l;
	    clen += l;
	}
	else
#endif
	    for ( ; *s != NUL; ++s)
	{
	    /* Don't display backslashes used for escaping, they look ugly. */
	    if (rem_backslash(s)
#ifdef FEAT_MENU
		    || (emenu
			  && (s[0] == '\t' || (s[0] == '\\' && s[1] != NUL)))
#endif
		    )
		++s;
	    clen += ptr2cells(s);
#ifdef FEAT_MBYTE
	    if (has_mbyte && (l = (*mb_ptr2len_check)(s)) > 1)
	    {
		STRNCPY(buf + len, s, l);
		s += l - 1;
		len += l;
	    }
	    else
#endif
	    {
		STRCPY(buf + len, transchar(*s));
		len += STRLEN(buf + len);
	    }
	}
	if (i == match)
	    selend = buf + len;

	*(buf + len++) = ' ';
	*(buf + len++) = ' ';
	clen += 2;
	if (++i == num_matches)
		break;
    }

    if (i != num_matches)
    {
	*(buf + len++) = '>';
	++clen;
    }

    buf[len] = NUL;

    row = cmdline_row - 1;
    if (row >= 0)
    {
	if (!wild_menu_showing)
	{
	    if (msg_scrolled && !wild_menu_showing)
	    {
		/* Put the wildmenu just above the command line.  If there is
		 * no room, scroll the screen one line up. */
		if (cmdline_row == Rows - 1)
		{
		    screen_del_lines(0, 0, 1, (int)Rows, TRUE, NULL);
		    ++msg_scrolled;
		}
		else
		{
		    ++cmdline_row;
		    ++row;
		}
		wild_menu_showing = WM_SCROLLED;
	    }
	    else
	    {
		/* create status line if needed */
		if (lastwin->w_status_height == 0)
		{
		    save_p_ls = p_ls;
		    p_ls = 2;
		    last_status(FALSE);
		}
		wild_menu_showing = WM_SHOWN;
	    }
	}

	screen_puts(buf, row, 0, attr);
	if (selstart != NULL && highlight)
	{
	    *selend = NUL;
	    screen_puts(selstart, row, selstart_col, hl_attr(HLF_WM));
	}

	screen_fill(row, row + 1, clen, (int)Columns, fillchar, fillchar, attr);
    }

#ifdef FEAT_VERTSPLIT
    win_redraw_last_status(topframe);
#else
    lastwin->w_redr_status = TRUE;
#endif
    vim_free(buf);
}
#endif

#if defined(FEAT_WINDOWS) || defined(PROTO)
/*
 * Redraw the status line of window wp.
 *
 * If inversion is possible we use it. Else '=' characters are used.
 */
    void
win_redr_status(wp)
    win_T	*wp;
{
    int		row;
    char_u	*p;
    int		len;
    int		fillchar;
    int		attr;
    int		this_ru_col;

    wp->w_redr_status = FALSE;
    if (wp->w_status_height == 0)
    {
	/* no status line, can only be last window */
	redraw_cmdline = TRUE;
    }
    else if (!redrawing())
    {
	/* Don't redraw right now, do it later. */
	wp->w_redr_status = TRUE;
    }
#ifdef FEAT_STL_OPT
    else if (*p_stl)
    {
	/* redraw custom status line */
	win_redr_custom(wp, FALSE);
    }
#endif
    else
    {
	fillchar = fillchar_status(&attr, wp == curwin);

	if (buf_spname(wp->w_buffer) != NULL)
	    STRCPY(NameBuff, buf_spname(wp->w_buffer));
	else
	{
	    home_replace(wp->w_buffer, wp->w_buffer->b_fname, NameBuff,
							      MAXPATHL, TRUE);
	    trans_characters(NameBuff, MAXPATHL);
	}
	p = NameBuff;
	len = STRLEN(p);

	if (wp->w_buffer->b_help
#ifdef FEAT_QUICKFIX
		|| wp->w_p_pvw
#endif
		|| bufIsChanged(wp->w_buffer)
		|| wp->w_buffer->b_p_ro)
	    *(p + len++) = ' ';
	if (wp->w_buffer->b_help)
	{
	    STRCPY(p + len, _("[help]"));
	    len += STRLEN(p + len);
	}
#ifdef FEAT_QUICKFIX
	if (wp->w_p_pvw)
	{
	    STRCPY(p + len, _("[Preview]"));
	    len += STRLEN(p + len);
	}
#endif
	if (bufIsChanged(wp->w_buffer))
	{
	    STRCPY(p + len, "[+]");
	    len += 3;
	}
	if (wp->w_buffer->b_p_ro)
	{
	    STRCPY(p + len, "[RO]");
	    len += 4;
	}

#ifndef FEAT_VERTSPLIT
	this_ru_col = ru_col;
	if (this_ru_col < (Columns + 1) / 2)
	    this_ru_col = (Columns + 1) / 2;
#else
	this_ru_col = ru_col - (Columns - W_WIDTH(wp));
	if (this_ru_col < (W_WIDTH(wp) + 1) / 2)
	    this_ru_col = (W_WIDTH(wp) + 1) / 2;
	if (this_ru_col <= 1)
	{
	    p = (char_u *)"<";		/* No room for file name! */
	    len = 1;
	}
	else
#endif
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		int	clen = 0, i;

		p[len] = NUL;
		/* Count total number of display cells. */
		for (i = 0; p[i] != NUL; i += (*mb_ptr2len_check)(p + i))
		    clen += (*mb_ptr2cells)(p + i);
		/* Find first character that will fit.
		 * Going from start to end is much faster for DBCS. */
		for (i = 0; p[i] != NUL && clen > this_ru_col - 1;
					      i += (*mb_ptr2len_check)(p + i))
		    clen -= (*mb_ptr2cells)(p + i);
		if (i > 0)
		{
		    p = p + i - 1;
		    *p = '<';
		    len = len - i + 1;
		}

	    }
	    else
#endif
	    if (len > this_ru_col - 1)
	    {
		p += len - (this_ru_col - 1);
		*p = '<';
		len = this_ru_col - 1;
	    }

	row = W_WINROW(wp) + wp->w_height;
	screen_puts(p, row, W_WINCOL(wp), attr);
	screen_fill(row, row + 1, len + W_WINCOL(wp),
			this_ru_col + W_WINCOL(wp), fillchar, fillchar, attr);

	if (get_keymap_str(wp, NameBuff, MAXPATHL)
		&& (int)(this_ru_col - len) > (int)(STRLEN(NameBuff) + 1))
	    screen_puts(NameBuff, row, (int)(this_ru_col - STRLEN(NameBuff)
						   - 1 + W_WINCOL(wp)), attr);

#ifdef FEAT_CMDL_INFO
	win_redr_ruler(wp, TRUE);
#endif
    }

#ifdef FEAT_VERTSPLIT
    /*
     * May need to draw the character below the vertical separator.
     */
    if (wp->w_vsep_width != 0)
    {
	if (stl_connected(wp))
	    fillchar = fillchar_status(&attr, wp == curwin);
	else
	    fillchar = fillchar_vsep(&attr);
	screen_putchar(fillchar, W_WINROW(wp) + wp->w_height, W_ENDCOL(wp),
									attr);
    }
#endif
}

# ifdef FEAT_VERTSPLIT
/*
 * Return TRUE if the status line of window "wp" is connected to the status
 * line of the window right of it.  If not, then it's a vertical separator.
 * Only call if (wp->w_vsep_width != 0).
 */
    int
stl_connected(wp)
    win_T	*wp;
{
    frame_T	*fr;

    fr = wp->w_frame;
    while (fr->fr_parent != NULL)
    {
	if (fr->fr_parent->fr_layout == FR_COL)
	{
	    if (fr->fr_next != NULL)
		break;
	}
	else
	{
	    if (fr->fr_next != NULL)
		return TRUE;
	}
	fr = fr->fr_parent;
    }
    return FALSE;
}
# endif

#endif /* FEAT_WINDOWS */

#if defined(FEAT_WINDOWS) || defined(FEAT_STL_OPT) || defined(PROTO)
/*
 * Get the value to show for the language mappings, active 'keymap'.
 */
    int
get_keymap_str(wp, buf, len)
    win_T	*wp;
    char_u	*buf;	    /* buffer for the result */
    int		len;	    /* length of buffer */
{
    char_u	*p;

    if (!(wp->w_buffer->b_lmap & B_LMAP_INSERT))
	return FALSE;

    {
#ifdef FEAT_EVAL
	buf_T	*old_curbuf = curbuf;
	win_T	*old_curwin = curwin;
	char_u	*s;

	curbuf = wp->w_buffer;
	curwin = wp;
	STRCPY(buf, "b:keymap_name");	/* must be writable */
	++emsg_skip;
	s = p = eval_to_string(buf, NULL);
	--emsg_skip;
	curbuf = old_curbuf;
	curwin = old_curwin;
	if (p == NULL || *p == NUL)
#endif
	{
#ifdef FEAT_KEYMAP
	    if (wp->w_buffer->b_kmap_state & KEYMAP_LOADED)
		p = wp->w_buffer->b_p_keymap;
	    else
#endif
		p = (char_u *)"lang";
	}
	if ((int)(STRLEN(p) + 3) < len)
	    sprintf((char *)buf, "<%s>", p);
	else
	    buf[0] = NUL;
#ifdef FEAT_EVAL
	vim_free(s);
#endif
    }
    return buf[0] != NUL;
}
#endif

#if defined(FEAT_STL_OPT) || defined(PROTO)
/*
 * Redraw the status line or ruler of window wp.
 */
    static void
win_redr_custom(wp, Ruler)
    win_T	*wp;
    int		Ruler;
{
    int		attr;
    int		curattr;
    int		row;
    int		col = 0;
    int		maxlen;
    int		n;
    int		len;
    int		fillchar;
    char_u	buf[MAXPATHL];
    char_u	*p;
    char_u	c;
    struct	stl_hlrec hl[STL_MAX_ITEM];

    /* setup environment for the task at hand */
    row = W_WINROW(wp) + wp->w_height;
    fillchar = fillchar_status(&attr, wp == curwin);
    maxlen = W_WIDTH(wp);
    p = p_stl;
    if (Ruler)
    {
	p = p_ruf;
	/* advance past any leading group spec - implicit in ru_col */
	if (*p == '%')
	{
	    if (*++p == '-')
		p++;
	    if (atoi((char *) p))
		while (isdigit(*p))
		    p++;
	    if (*p++ != '(')
		p = p_ruf;
	}
#ifdef FEAT_VERTSPLIT
	col = ru_col - (Columns - W_WIDTH(wp));
	if (col < (W_WIDTH(wp) + 1) / 2)
	    col = (W_WIDTH(wp) + 1) / 2;
#else
	col = ru_col;
	if (col > (Columns + 1) / 2)
	    col = (Columns + 1) / 2;
#endif
	maxlen = W_WIDTH(wp) - col;
#ifdef FEAT_WINDOWS
	if (!wp->w_status_height)
#endif
	{
	    row = Rows - 1;
	    --maxlen;	/* writing in last column may cause scrolling */
	    fillchar = ' ';
	    attr = 0;
	}
    }
    if (maxlen >= sizeof(buf))
	maxlen = sizeof(buf) - 1;
    if (maxlen <= 0)
	return;
#ifdef FEAT_VERTSPLIT
    col += W_WINCOL(wp);
#endif

    len = build_stl_str_hl(wp, buf, p, fillchar, maxlen, hl);

    for (p = buf + len; p < buf + maxlen; p++)
	*p = fillchar;
    buf[maxlen] = 0;

    curattr = attr;
    p = buf;
    for (n = 0; hl[n].start != NULL; n++)
    {
	c = hl[n].start[0];
	hl[n].start[0] = 0;
	screen_puts(p, row, col, curattr);

	hl[n].start[0] = c;
	col += hl[n].start - p;
	p = hl[n].start;

	if (hl[n].userhl == 0)
	    curattr = attr;
#ifdef FEAT_WINDOWS
	else if (wp != curwin && wp->w_status_height != 0)
	    curattr = highlight_stlnc[hl[n].userhl - 1];
#endif
	else
	    curattr = highlight_user[hl[n].userhl - 1];
    }
    screen_puts(p, row, col, curattr);
}

#endif /* FEAT_STL_OPT */

/*
 * Output a single character directly to the screen and update ScreenLines.
 * Not for multi-byte chars!
 */
    void
screen_putchar(c, row, col, attr)
    int	    c;
    int	    row, col;
    int	    attr;
{
    char_u	buf[2];

    {
	buf[0] = c;
	buf[1] = NUL;
	screen_puts(buf, row, col, attr);
    }
}

/*
 * Get a single character directly from ScreenLines.
 * Also return its attribute in *attrp;
 * For multi-byte chars only the first byte is obtained!
 */
    int
screen_getchar(row, col, attrp)
    int	    row, col;
    int	    *attrp;
{
    unsigned off;

    /* safety check */
    if (ScreenLines != NULL && row < screen_Rows && col < screen_Columns)
    {
	off = LineOffset[row] + col;
	*attrp = ScreenAttrs[off];
	return ScreenLines[off];
    }
    return 0;
}

/*
 * Put string '*text' on the screen at position 'row' and 'col', with
 * attributes 'attr', and update ScreenLines[] and ScreenAttrs[].
 * Note: only outputs within one row, message is truncated at screen boundary!
 * Note: if ScreenLines[], row and/or col is invalid, nothing is done.
 */
    void
screen_puts(text, row, col, attr)
    char_u	*text;
    int		row;
    int		col;
    int		attr;
{
    unsigned	off;
#ifdef FEAT_MBYTE
    int		mbyte_blen = 0;
    int		mbyte_cells = 1;
    int		u8c = 0;
    int		u8c_c1 = 0;
    int		u8c_c2 = 0;
#endif

    if (ScreenLines != NULL && row < screen_Rows)	/* safety check */
    {
	off = LineOffset[row] + col;
	while (*text && col < screen_Columns)
	{
#ifdef FEAT_MBYTE
	    /* check if this is the first byte of a multibyte */
	    if (has_mbyte)
	    {
		mbyte_blen = (*mb_ptr2len_check)(text);
		if (enc_dbcs == DBCS_JPNU && *text == 0x8e)
		    mbyte_cells = 1;
		else if (enc_dbcs != 0)
		    mbyte_cells = mbyte_blen;
		else	/* enc_utf8 */
		{
		    u8c = utfc_ptr2char(text, &u8c_c1, &u8c_c2);
		    mbyte_cells = utf_char2cells(u8c);
		}
	    }
#endif

	    if (ScreenLines[off] != *text
#ifdef FEAT_MBYTE
		    || (mbyte_cells == 2
			&& ScreenLines[off + 1] != (enc_dbcs ? text[1] : 0))
		    || (enc_dbcs == DBCS_JPNU
			&& *text == 0x8e
			&& ScreenLines2[off] != text[1])
		    || (enc_utf8
			&& mbyte_blen > 1
			&& (ScreenLinesUC[off] != u8c
			    || ScreenLinesC1[off] != u8c_c1
			    || ScreenLinesC2[off] != u8c_c2))
#endif
		    || ScreenAttrs[off] != attr
		    || exmode_active
		    )
	    {
#if defined(FEAT_GUI) || defined(UNIX)
		/* The bold trick makes a single row of pixels appear in the
		 * next character.  When a bold character is removed, the next
		 * character should be redrawn too.  This happens for our own
		 * GUI and for some xterms.
		 * Don't do this for the last drawn character, because the
		 * next character may not be redrawn. */
		if (
# ifdef FEAT_GUI
			gui.in_use
# endif
# if defined(FEAT_GUI) && defined(UNIX)
			||
# endif
# ifdef UNIX
			term_is_xterm
# endif
		   )
		{
		    int		n;

		    n = ScreenAttrs[off];
		    if (col + 1
# ifdef FEAT_MBYTE
				+ mbyte_cells - 1
# endif
				< screen_Columns
			    && (n > HL_ALL || (n & HL_BOLD))
			    && text[1] != NUL)
			ScreenLines[off + 1
# ifdef FEAT_MBYTE
			    + mbyte_cells - 1
# endif
			    ] = 0;
		}
#endif
		ScreenLines[off] = *text;
		ScreenAttrs[off] = attr;
#ifdef FEAT_MBYTE
		if (enc_utf8)
		{
		    if (*text < 0x80 && u8c_c1 == 0 && u8c_c2 == 0)
			ScreenLinesUC[off] = 0;
		    else
		    {
			ScreenLinesUC[off] = u8c;
			ScreenLinesC1[off] = u8c_c1;
			ScreenLinesC2[off] = u8c_c2;
		    }
		    if (mbyte_cells == 2)
		    {
			ScreenLines[off + 1] = 0;
			ScreenAttrs[off + 1] = attr;
		    }
		    screen_char(off, row, col);
		}
		else if (mbyte_cells == 2)
		{
		    ScreenLines[off + 1] = text[1];
		    ScreenAttrs[off + 1] = attr;
		    screen_char_2(off, row, col);
		}
		else if (enc_dbcs == DBCS_JPNU && *text == 0x8e)
		{
		    ScreenLines2[off] = text[1];
		    screen_char(off, row, col);
		}
		else
#endif
		    screen_char(off, row, col);
	    }
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		off += mbyte_cells;
		col += mbyte_cells;
		text += mbyte_blen;
	    }
	    else
#endif
	    {
		++off;
		++col;
		++text;
	    }
	}
    }
}

#ifdef FEAT_SEARCH_EXTRA
/*
 * Prepare for 'searchhl' highlighting.
 */
    static void
start_search_hl()
{
    if (p_hls && !no_hlsearch)
    {
	last_pat_prog(&search_hl.rm);
	search_hl.attr = hl_attr(HLF_L);
    }
}

/*
 * Clean up for 'searchhl' highlighting.
 */
    static void
end_search_hl()
{
    if (search_hl.rm.regprog != NULL)
    {
	vim_free(search_hl.rm.regprog);
	search_hl.rm.regprog = NULL;
    }
}

/*
 * Advance to the match in window "wp" line "lnum" or past it.
 */
    static void
prepare_search_hl(wp, lnum)
    win_T	*wp;
    linenr_T	lnum;
{
    match_T	*shl;		/* points to search_hl or match_hl */
    int		n;

    /*
     * When using a multi-line pattern, start searching at the top
     * of the window or just after a closed fold.
     * Do this both for search_hl and match_hl.
     */
    shl = &search_hl;
    for (;;)
    {
	if (shl->rm.regprog != NULL
		&& shl->lnum == 0
		&& re_multiline(shl->rm.regprog))
	{
	    if (shl->first_lnum == 0)
	    {
# ifdef FEAT_FOLDING
		for (shl->first_lnum = lnum;
			   shl->first_lnum > wp->w_topline; --shl->first_lnum)
		    if (hasFoldingWin(wp, shl->first_lnum - 1,
						      NULL, NULL, TRUE, NULL))
			break;
# else
		shl->first_lnum = wp->w_topline;
# endif
	    }
	    n = 0;
	    while (shl->first_lnum < lnum)
	    {
		next_search_hl(wp, shl, shl->first_lnum, (colnr_T)n);
		if (shl->lnum != 0)
		{
		    shl->first_lnum = shl->lnum
				    + shl->rm.endpos[0].lnum
				    - shl->rm.startpos[0].lnum;
		    n = shl->rm.endpos[0].col;
		}
		else
		{
		    ++shl->first_lnum;
		    n = 0;
		}
	    }
	}
	if (shl == &match_hl)
	    break;
	shl = &match_hl;
    }
}

/*
 * Search for a next 'searchl' or ":match" match.
 * Uses shl->buf.
 * Sets shl->lnum and shl->rm contents.
 * Note: Assumes a previous match is always before "lnum", unless
 * shl->lnum is zero.
 * Careful: Any pointers for buffer lines will become invalid.
 */
    static void
next_search_hl(win, shl, lnum, mincol)
    win_T	*win;
    match_T	*shl;		/* points to search_hl or match_hl */
    linenr_T	lnum;
    colnr_T	mincol;		/* minimal column for a match */
{
    linenr_T	l;
    colnr_T	matchcol;
    long	nmatched;

    if (shl->lnum != 0)
    {
	/* Check for three situations:
	 * 1. If the "lnum" is below a previous match, start a new search.
	 * 2. If the previous match includes "mincol", use it.
	 * 3. Continue after the previous match.
	 */
	l = shl->lnum + shl->rm.endpos[0].lnum - shl->rm.startpos[0].lnum;
	if (lnum > l)
	    shl->lnum = 0;
	else if (lnum < l || shl->rm.endpos[0].col > mincol)
	    return;
    }

    /*
     * Repeat searching for a match until one is found that includes "mincol"
     * or none is found in this line.
     */
    for (;;)
    {
	/* Three situations:
	 * 1. No useful previous match: search from start of line.
	 * 2. Not Vi compatible or empty match: continue at next character.
	 *    Break the loop if this is beyond the end of the line.
	 * 3. Vi compatible searching: continue at end of previous match.
	 */
	if (shl->lnum == 0)
	    matchcol = 0;
	else if (vim_strchr(p_cpo, CPO_SEARCH) == NULL
		|| (shl->rm.endpos[0].lnum == 0
		    && shl->rm.endpos[0].col == shl->rm.startpos[0].col))
	{
	    matchcol = shl->rm.startpos[0].col + 1;
	    if (ml_get_buf(shl->buf, lnum, FALSE)[matchcol - 1] == NUL)
	    {
		shl->lnum = 0;
		break;
	    }
	}
	else
	    matchcol = shl->rm.endpos[0].col;

	shl->lnum = lnum;
	nmatched = vim_regexec_multi(&shl->rm, win, shl->buf, lnum, matchcol);
	if (nmatched == 0)
	{
	    shl->lnum = 0;		/* no match found */
	    break;
	}
	if (shl->rm.startpos[0].lnum > 0
		|| shl->rm.startpos[0].col >= mincol
		|| nmatched > 1
		|| shl->rm.endpos[0].col > mincol)
	{
	    shl->lnum += shl->rm.startpos[0].lnum;
	    break;			/* useful match found */
	}
    }
}
#endif

      static void
screen_start_highlight(attr)
      int	attr;
{
    attrentry_T *aep = NULL;

    screen_attr = attr;
    if (full_screen
#ifdef WIN32
		    && termcap_active
#endif
				       )
    {
#ifdef FEAT_GUI
	if (gui.in_use)
	{
	    char	buf[20];

	    sprintf(buf, IF_EB("\033|%dh", ESC_STR "|%dh"), attr);		/* internal GUI code */
	    OUT_STR(buf);
	}
	else
#endif
	{
	    if (attr > HL_ALL)				/* special HL attr. */
	    {
		if (t_colors > 1)
		    aep = syn_cterm_attr2entry(attr);
		else
		    aep = syn_term_attr2entry(attr);
		if (aep == NULL)	    /* did ":syntax clear" */
		    attr = 0;
		else
		    attr = aep->ae_attr;
	    }
	    if ((attr & HL_BOLD) && T_MD != NULL)	/* bold */
		out_str(T_MD);
	    if ((attr & HL_STANDOUT) && T_SO != NULL)	/* standout */
		out_str(T_SO);
	    if ((attr & HL_UNDERLINE) && T_US != NULL)	/* underline */
		out_str(T_US);
	    if ((attr & HL_ITALIC) && T_CZH != NULL)	/* italic */
		out_str(T_CZH);
	    if ((attr & HL_INVERSE) && T_MR != NULL)	/* inverse (reverse) */
		out_str(T_MR);

	    /*
	     * Output the color or start string after bold etc., in case the
	     * bold etc. override the color setting.
	     */
	    if (aep != NULL)
	    {
		if (t_colors > 1)
		{
		    if (aep->ae_u.cterm.fg_color)
			term_fg_color(aep->ae_u.cterm.fg_color - 1);
		    if (aep->ae_u.cterm.bg_color)
			term_bg_color(aep->ae_u.cterm.bg_color - 1);
		}
		else
		{
		    if (aep->ae_u.term.start != NULL)
			out_str(aep->ae_u.term.start);
		}
	    }
	}
    }
}

      void
screen_stop_highlight()
{
    int	    do_ME = FALSE;	    /* output T_ME code */

    if (screen_attr != 0
#ifdef WIN32
			&& termcap_active
#endif
					   )
    {
#ifdef FEAT_GUI
	if (gui.in_use)
	{
	    char	buf[20];

	    /* use internal GUI code */
	    sprintf(buf, IF_EB("\033|%dH", ESC_STR "|%dH"), screen_attr);
	    OUT_STR(buf);
	}
	else
#endif
	{
	    if (screen_attr > HL_ALL)			/* special HL attr. */
	    {
		attrentry_T *aep;

		if (t_colors > 1)
		{
		    /*
		     * Assume that t_me restores the original colors!
		     */
		    aep = syn_cterm_attr2entry(screen_attr);
		    if (aep != NULL && (aep->ae_u.cterm.fg_color
						 || aep->ae_u.cterm.bg_color))
			do_ME = TRUE;
		}
		else
		{
		    aep = syn_term_attr2entry(screen_attr);
		    if (aep != NULL && aep->ae_u.term.stop != NULL)
		    {
			if (STRCMP(aep->ae_u.term.stop, T_ME) == 0)
			    do_ME = TRUE;
			else
			    out_str(aep->ae_u.term.stop);
		    }
		}
		if (aep == NULL)	    /* did ":syntax clear" */
		    screen_attr = 0;
		else
		    screen_attr = aep->ae_attr;
	    }

	    /*
	     * Often all ending-codes are equal to T_ME.  Avoid outputting the
	     * same sequence several times.
	     */
	    if (screen_attr & HL_STANDOUT)
	    {
		if (STRCMP(T_SE, T_ME) == 0)
		    do_ME = TRUE;
		else
		    out_str(T_SE);
	    }
	    if (screen_attr & HL_UNDERLINE)
	    {
		if (STRCMP(T_UE, T_ME) == 0)
		    do_ME = TRUE;
		else
		    out_str(T_UE);
	    }
	    if (screen_attr & HL_ITALIC)
	    {
		if (STRCMP(T_CZR, T_ME) == 0)
		    do_ME = TRUE;
		else
		    out_str(T_CZR);
	    }
	    if (do_ME || (screen_attr & (HL_BOLD | HL_INVERSE)))
		out_str(T_ME);

	    if (t_colors > 1)
	    {
		/* set Normal cterm colors */
		if (cterm_normal_fg_color != 0)
		    term_fg_color(cterm_normal_fg_color - 1);
		if (cterm_normal_bg_color != 0)
		    term_bg_color(cterm_normal_bg_color - 1);
		if (cterm_normal_fg_bold)
		    out_str(T_MD);
	    }
	}
    }
    screen_attr = 0;
}

/*
 * Reset the colors for a cterm.  Used when leaving Vim.
 * The machine specific code may override this again.
 */
    void
reset_cterm_colors()
{
    if (t_colors > 1)
    {
	/* set Normal cterm colors */
	if (cterm_normal_fg_color || cterm_normal_bg_color)
	    out_str(T_OP);
	if (cterm_normal_fg_bold)
	    out_str(T_ME);
    }
}

/*
 * Put character ScreenLines["off"] on the screen at position "row" and "col",
 * using the attributes from ScreenAttrs["off"].
 */
    static void
screen_char(off, row, col)
    unsigned	off;
    int		row;
    int		col;
{
    int		attr;

    /* Check for illegal values, just in case (could happen just after
     * resizing). */
    if (row >= screen_Rows || col >= screen_Columns)
	return;

    /* Outputting the last character on the screen may scrollup the screen.
     * Don't to it!  Mark the character invalid (update it when scrolled up) */
    if (row == screen_Rows - 1 && col == screen_Columns - 1)
    {
	ScreenAttrs[off] = (sattr_T)-1;
	return;
    }

    /*
     * Stop highlighting first, so it's easier to move the cursor.
     */
#if defined(FEAT_CLIPBOARD) || defined(FEAT_VERTSPLIT)
    if (screen_char_attr != 0)
	attr = screen_char_attr;
    else
#endif
	attr = ScreenAttrs[off];
    if (screen_attr != attr)
	screen_stop_highlight();

    windgoto(row, col);

    if (screen_attr != attr)
	screen_start_highlight(attr);

#ifdef FEAT_MBYTE
    if (enc_utf8 && ScreenLinesUC[off] != 0)
    {
	char_u	    buf[MB_MAXBYTES + 1];

	/* Convert UTF-8 character to bytes and write it. */
	buf[utfc_char2bytes(off, buf)] = NUL;
	out_str(buf);
	if (utf_char2cells(ScreenLinesUC[off]) > 1)
	    ++screen_cur_col;
    }
    else
#endif
    {
	out_char(ScreenLines[off]);
#ifdef FEAT_MBYTE
	/* double-byte character in single-width cell */
	if (enc_dbcs == DBCS_JPNU && ScreenLines[off] == 0x8e)
	    out_char(ScreenLines2[off]);
#endif
    }

    screen_cur_col++;
}

#ifdef FEAT_MBYTE

/*
 * Used for enc_dbcs only: Put one double-wide character at ScreenLines["off"]
 * on the screen at position 'row' and 'col'.
 * The attributes of the first byte is used for all.  This is required to
 * output the two bytes of a double-byte character with nothing in between.
 */
    static void
screen_char_2(off, row, col)
    unsigned	off;
    int		row;
    int		col;
{
    /*
     * Outputting the last character on the screen may scrollup the screen.
     * Don't to it!  At the same time check for illegal values, just in case.
     */
    if (off + 2 >= (unsigned)(screen_Rows * screen_Columns))
	return;

    /* Output the first byte normally (positions the cursor), then write the
     * second byte directly. */
    screen_char(off, row, col);
    out_char(ScreenLines[off + 1]);
    ++screen_cur_col;
}
#endif

#if defined(FEAT_CLIPBOARD) || defined(FEAT_VERTSPLIT) || defined(PROTO)
/*
 * Draw a rectangle of the screen, inverted when "invert" is TRUE.
 * This uses the contents of ScreenLines[] and doesn't change it.
 */
    void
screen_draw_rectangle(row, col, height, width, invert)
    int		row;
    int		col;
    int		height;
    int		width;
    int		invert;
{
    int		r, c;
    int		off;

    if (invert)
	screen_char_attr = HL_INVERSE;
    for (r = row; r < row + height; ++r)
    {
	off = LineOffset[r];
	for (c = col; c < col + width; ++c)
	{
#ifdef FEAT_MBYTE
	    if (enc_dbcs != 0 && dbcs_off2cells(off + c) > 1)
	    {
		screen_char_2(off + c, r, c);
		++c;
	    }
	    else
#endif
	    {
		screen_char(off + c, r, c);
#ifdef FEAT_MBYTE
		if (utf_off2cells(off + c) > 1)
		    ++c;
#endif
	    }
	}
    }
    screen_char_attr = 0;
}
#endif

#ifdef FEAT_VERTSPLIT
/*
 * Redraw the characters for a vertically split window.
 */
    static void
redraw_block(row, end, wp)
    int		row;
    int		end;
    win_T	*wp;
{
    int		col;
    int		width;

# ifdef FEAT_CLIPBOARD
    clip_may_clear_selection(row, end - 1);
# endif

    if (wp == NULL)
    {
	col = 0;
	width = Columns;
    }
    else
    {
	col = wp->w_wincol;
	width = wp->w_width;
    }
    screen_draw_rectangle(row, col, end - row, width, FALSE);
}
#endif

/*
 * Fill the screen from 'start_row' to 'end_row', from 'start_col' to 'end_col'
 * with character 'c1' in first column followed by 'c2' in the other columns.
 * Use attributes 'attr'.
 * Cannot handle multi-byte characters, c1 and c2 must be < 0x80!
 */
    void
screen_fill(start_row, end_row, start_col, end_col, c1, c2, attr)
    int	    start_row, end_row;
    int	    start_col, end_col;
    int	    c1, c2;
    int	    attr;
{
    int		    row;
    int		    col;
    int		    off;
    int		    did_delete;
    int		    c;
    int		    norm_term;
#if defined(FEAT_GUI) || defined(UNIX)
    int		    force_next = FALSE;
#endif

    if (end_row > screen_Rows)		/* safety check */
	end_row = screen_Rows;
    if (end_col > screen_Columns)	/* safety check */
	end_col = screen_Columns;
    if (ScreenLines == NULL
	    || start_row >= end_row
	    || start_col >= end_col)	/* nothing to do */
	return;

    /* it's a "normal" terminal when not in a GUI or cterm */
    norm_term = (
#ifdef FEAT_GUI
	    !gui.in_use &&
#endif
			    t_colors <= 1);
    for (row = start_row; row < end_row; ++row)
    {
	/*
	 * Try to use delete-line termcap code, when no attributes or in a
	 * "normal" terminal, where a bold/italic space is just a
	 * space.
	 */
	did_delete = FALSE;
	if (c2 == ' '
		&& end_col == Columns
		&& can_clear(T_CE)
		&& (attr == 0
		    || (norm_term
			&& attr <= HL_ALL
			&& ((attr & ~(HL_BOLD | HL_ITALIC)) == 0))))
	{
	    /*
	     * check if we really need to clear something
	     */
	    col = start_col;
	    if (c1 != ' ')			/* don't clear first char */
		++col;

	    off = LineOffset[row] + col;

	    /* skip blanks (used often, keep it fast!) */
	    while (col < end_col && ScreenLines[off] == ' '
						     && ScreenAttrs[off] == 0)
	    {
		++col;
		++off;
	    }
	    if (col < end_col)		/* something to be cleared */
	    {
		screen_stop_highlight();
		term_windgoto(row, col);/* clear rest of this screen line */
		out_str(T_CE);
		screen_start();		/* don't know where cursor is now */
		col = end_col - col;
		while (col--)		/* clear chars in ScreenLines */
		{
		    ScreenLines[off] = ' ';
#ifdef FEAT_MBYTE
		    if (enc_utf8)
			ScreenLinesUC[off] = 0;
#endif
		    ScreenAttrs[off] = 0;
		    ++off;
		}
	    }
	    did_delete = TRUE;		/* the chars are cleared now */
	}

	off = LineOffset[row] + start_col;
	c = c1;
	for (col = start_col; col < end_col; ++col)
	{
	    if (ScreenLines[off] != c || ScreenAttrs[off] != attr
#if defined(FEAT_GUI) || defined(UNIX)
		    || force_next
#endif
		    )
	    {
#if defined(FEAT_GUI) || defined(UNIX)
		/* The bold trick may make a single row of pixels appear in
		 * the next character.  When a bold character is removed, the
		 * next character should be redrawn too.  This happens for our
		 * own GUI and for some xterms.  */
		if (
# ifdef FEAT_GUI
			gui.in_use
# endif
# if defined(FEAT_GUI) && defined(UNIX)
			||
# endif
# ifdef UNIX
			term_is_xterm
# endif
		   )
		{
		    if (ScreenLines[off] != ' '
			    && (ScreenAttrs[off] > HL_ALL
				|| ScreenAttrs[off] & HL_BOLD))
			force_next = TRUE;
		    else
			force_next = FALSE;
		}
#endif
		ScreenLines[off] = c;
#ifdef FEAT_MBYTE
		if (enc_utf8)
		    ScreenLinesUC[off] = 0;
#endif
		ScreenAttrs[off] = attr;
		if (!did_delete || c != ' ')
		    screen_char(off, row, col);
	    }
	    ++off;
	    if (col == start_col)
	    {
		if (did_delete)
		    break;
		c = c2;
	    }
	}
	if (row == Rows - 1)		/* overwritten the command line */
	{
	    redraw_cmdline = TRUE;
	    if (c1 == ' ' && c2 == ' ')
		clear_cmdline = FALSE;	/* command line has been cleared */
	}
    }
}

/*
 * Check if there should be a delay.  Used before clearing or redrawing the
 * screen or the command line.
 */
    void
check_for_delay(check_msg_scroll)
    int	    check_msg_scroll;
{
    if ((emsg_on_display || (check_msg_scroll && msg_scroll))
	    && emsg_silent == 0)
    {
	out_flush();
	ui_delay(1000L, TRUE);
	emsg_on_display = FALSE;
	if (check_msg_scroll)
	    msg_scroll = FALSE;
    }
}

/*
 * screen_valid -  allocate screen buffers if size changed
 *   If "clear" is TRUE: clear screen if it has been resized.
 *	Returns TRUE if there is a valid screen to write to.
 *	Returns FALSE when starting up and screen not initialized yet.
 */
    int
screen_valid(clear)
    int	    clear;
{
    screenalloc(clear);	    /* allocate screen buffers if size changed */
    return (ScreenLines != NULL);
}

/*
 * Resize the shell to Rows and Columns.
 * Allocate ScreenLines[] and associated items.
 *
 * There may be some time between setting Rows and Columns and (re)allocating
 * ScreenLines[].  This happens when starting up and when (manually) changing
 * the shell size.  Always use screen_Rows and screen_Columns to access items
 * in ScreenLines[].  Use Rows and Columns for positioning text etc. where the
 * final size of the shell is needed.
 */
    void
screenalloc(clear)
    int	    clear;
{
    int		    new_row, old_row;
#ifdef FEAT_GUI
    int		    old_Rows;
#endif
    win_T	    *wp;
    int		    outofmem = FALSE;
    int		    len;
    schar_T	    *new_ScreenLines;
#ifdef FEAT_MBYTE
    u8char_T	    *new_ScreenLinesUC = NULL;
    u8char_T	    *new_ScreenLinesC1 = NULL;
    u8char_T	    *new_ScreenLinesC2 = NULL;
    schar_T	    *new_ScreenLines2 = NULL;
#endif
    sattr_T	    *new_ScreenAttrs;
    unsigned	    *new_LineOffset;
    static int	    entered = FALSE;		/* avoid recursiveness */

    /*
     * Allocation of the screen buffers is done only when the size changes and
     * when Rows and Columns have been set and we have started doing full
     * screen stuff.
     */
    if ((ScreenLines != NULL
		&& Rows == screen_Rows
		&& Columns == screen_Columns
#ifdef FEAT_MBYTE
		&& enc_utf8 == (ScreenLinesUC != NULL)
		&& (enc_dbcs == DBCS_JPNU) == (ScreenLines2 != NULL)
#endif
		)
	    || Rows == 0
	    || Columns == 0
	    || (!full_screen && ScreenLines == NULL))
	return;

    /*
     * It's possible that we produce an out-of-memory message below, which
     * will cause this function to be called again.  To break the loop, just
     * return here.
     */
    if (entered)
	return;
    entered = TRUE;

#ifdef FEAT_GUI_BEOS
    vim_lock_screen();  /* be safe, put it here */
#endif

    comp_col();		/* recompute columns for shown command and ruler */

    /*
     * We're changing the size of the screen.
     * - Allocate new arrays for ScreenLines and ScreenAttrs.
     * - Move lines from the old arrays into the new arrays, clear extra
     *	 lines (unless the screen is going to be cleared).
     * - Free the old arrays.
     *
     * If anything fails, make ScreenLines NULL, so we don't do anything!
     * Continuing with the old ScreenLines may result in a crash, because the
     * size is wrong.
     */
#ifdef FEAT_WINDOWS
    for (wp = firstwin; wp; wp = wp->w_next)
	win_free_lsize(wp);
#else
	win_free_lsize(curwin);
#endif

    new_ScreenLines = (schar_T *)lalloc((long_u)(
			      (Rows + 1) * Columns * sizeof(schar_T)), FALSE);
#ifdef FEAT_MBYTE
    if (enc_utf8)
    {
	new_ScreenLinesUC = (u8char_T *)lalloc((long_u)(
			     (Rows + 1) * Columns * sizeof(u8char_T)), FALSE);
	new_ScreenLinesC1 = (u8char_T *)lalloc((long_u)(
			     (Rows + 1) * Columns * sizeof(u8char_T)), FALSE);
	new_ScreenLinesC2 = (u8char_T *)lalloc((long_u)(
			     (Rows + 1) * Columns * sizeof(u8char_T)), FALSE);
    }
    if (enc_dbcs == DBCS_JPNU)
	new_ScreenLines2 = (schar_T *)lalloc((long_u)(
			     (Rows + 1) * Columns * sizeof(schar_T)), FALSE);
#endif
    new_ScreenAttrs = (sattr_T *)lalloc((long_u)(
			      (Rows + 1) * Columns * sizeof(sattr_T)), FALSE);
    new_LineOffset = (unsigned *)lalloc((long_u)(
					 Rows * sizeof(unsigned)), FALSE);

    FOR_ALL_WINDOWS(wp)
    {
	if (win_alloc_lines(wp) == FAIL)
	{
	    outofmem = TRUE;
#ifdef FEAT_WINDOWS
	    break;
#endif
	}
    }

    if (new_ScreenLines == NULL
#ifdef FEAT_MBYTE
	    || (enc_utf8 && (new_ScreenLinesUC == NULL
		   || new_ScreenLinesC1 == NULL || new_ScreenLinesC2 == NULL))
	    || (enc_dbcs == DBCS_JPNU && new_ScreenLines2 == NULL)
#endif
	    || new_ScreenAttrs == NULL
	    || new_LineOffset == NULL
	    || outofmem)
    {
	do_outofmem_msg((long_u)((Rows + 1) * Columns));    /* guess the size */
	vim_free(new_ScreenLines);
	new_ScreenLines = NULL;
#ifdef FEAT_MBYTE
	vim_free(new_ScreenLinesUC);
	new_ScreenLinesUC = NULL;
	vim_free(new_ScreenLinesC1);
	new_ScreenLinesC1 = NULL;
	vim_free(new_ScreenLinesC2);
	new_ScreenLinesC2 = NULL;
	vim_free(new_ScreenLines2);
	new_ScreenLines2 = NULL;
#endif
	vim_free(new_ScreenAttrs);
	new_ScreenAttrs = NULL;
	vim_free(new_LineOffset);
	new_LineOffset = NULL;
    }
    else
    {
	for (new_row = 0; new_row < Rows; ++new_row)
	{
	    new_LineOffset[new_row] = new_row * Columns;

	    /*
	     * If the screen is not going to be cleared, copy as much as
	     * possible from the old screen to the new one and clear the rest
	     * (used when resizing the window at the "--more--" prompt or when
	     * executing an external command, for the GUI).
	     */
	    if (!clear)
	    {
		(void)vim_memset(new_ScreenLines + new_row * Columns,
				      ' ', (size_t)Columns * sizeof(schar_T));
#ifdef FEAT_MBYTE
		if (enc_utf8)
		{
		    (void)vim_memset(new_ScreenLinesUC + new_row * Columns,
				       0, (size_t)Columns * sizeof(u8char_T));
		    (void)vim_memset(new_ScreenLinesC1 + new_row * Columns,
				       0, (size_t)Columns * sizeof(u8char_T));
		    (void)vim_memset(new_ScreenLinesC2 + new_row * Columns,
				       0, (size_t)Columns * sizeof(u8char_T));
		}
		if (enc_dbcs == DBCS_JPNU)
		    (void)vim_memset(new_ScreenLines2 + new_row * Columns,
				       0, (size_t)Columns * sizeof(schar_T));
#endif
		(void)vim_memset(new_ScreenAttrs + new_row * Columns,
					0, (size_t)Columns * sizeof(sattr_T));
		old_row = new_row + (screen_Rows - Rows);
		if (old_row >= 0)
		{
		    if (screen_Columns < Columns)
			len = screen_Columns;
		    else
			len = Columns;
		    mch_memmove(new_ScreenLines + new_LineOffset[new_row],
			    ScreenLines + LineOffset[old_row],
			    (size_t)len * sizeof(schar_T));
#ifdef FEAT_MBYTE
		    if (enc_utf8 && ScreenLinesUC != NULL)
		    {
			mch_memmove(new_ScreenLinesUC + new_LineOffset[new_row],
				ScreenLinesUC + LineOffset[old_row],
				(size_t)len * sizeof(u8char_T));
			mch_memmove(new_ScreenLinesC1 + new_LineOffset[new_row],
				ScreenLinesC1 + LineOffset[old_row],
				(size_t)len * sizeof(u8char_T));
			mch_memmove(new_ScreenLinesC2 + new_LineOffset[new_row],
				ScreenLinesC2 + LineOffset[old_row],
				(size_t)len * sizeof(u8char_T));
		    }
		    if (enc_dbcs == DBCS_JPNU && ScreenLines2 != NULL)
			mch_memmove(new_ScreenLines2 + new_LineOffset[new_row],
				ScreenLines2 + LineOffset[old_row],
				(size_t)len * sizeof(schar_T));
#endif
		    mch_memmove(new_ScreenAttrs + new_LineOffset[new_row],
			    ScreenAttrs + LineOffset[old_row],
			    (size_t)len * sizeof(sattr_T));
		}
	    }
	}
	/* Use the last line of the screen for the current line. */
	current_ScreenLine = new_ScreenLines + Rows * Columns;
    }

    vim_free(ScreenLines);
#ifdef FEAT_MBYTE
    vim_free(ScreenLinesUC);
    vim_free(ScreenLinesC1);
    vim_free(ScreenLinesC2);
    vim_free(ScreenLines2);
#endif
    vim_free(ScreenAttrs);
    vim_free(LineOffset);
    ScreenLines = new_ScreenLines;
#ifdef FEAT_MBYTE
    ScreenLinesUC = new_ScreenLinesUC;
    ScreenLinesC1 = new_ScreenLinesC1;
    ScreenLinesC2 = new_ScreenLinesC2;
    ScreenLines2 = new_ScreenLines2;
#endif
    ScreenAttrs = new_ScreenAttrs;
    LineOffset = new_LineOffset;

    /* It's important that screen_Rows and screen_Columns reflect the actual
     * size of ScreenLines[].  Set them before calling anything. */
#ifdef FEAT_GUI
    old_Rows = screen_Rows;
#endif
    screen_Rows = Rows;
    screen_Columns = Columns;

    must_redraw = CLEAR;	/* need to clear the screen later */
    if (clear)
	screenclear2();

#ifdef FEAT_GUI
    else if (gui.in_use
	    && !gui.starting
	    && ScreenLines != NULL
	    && old_Rows != Rows)
    {
	(void)gui_redraw_block(0, 0, (int)Rows - 1, (int)Columns - 1, 0);
	/*
	 * Adjust the position of the cursor, for when executing an external
	 * command.
	 */
	if (msg_row >= Rows)		/* Rows got smaller */
	    msg_row = Rows - 1;		/* put cursor at last row */
	else if (Rows > old_Rows)	/* Rows got bigger */
	    msg_row += Rows - old_Rows; /* put cursor in same place */
	if (msg_col >= Columns)		/* Columns got smaller */
	    msg_col = Columns - 1;	/* put cursor at last column */
    }
#endif

#ifdef FEAT_GUI_BEOS
    vim_unlock_screen();
#endif
    entered = FALSE;
}

    void
screenclear()
{
    check_for_delay(FALSE);
    screenalloc(FALSE);	    /* allocate screen buffers if size changed */
    screenclear2();	    /* clear the screen */
}

    static void
screenclear2()
{
    int	    i;

    if (starting == NO_SCREEN || ScreenLines == NULL)
	return;

    screen_stop_highlight();	/* don't want highlighting here */

#ifdef FEAT_CLIPBOARD
    /* disable selection without redrawing it */
    clip_scroll_selection(9999);
#endif

    /* blank out ScreenLines */
    for (i = 0; i < Rows; ++i)
	lineclear(LineOffset[i], (int)Columns);

    if (can_clear(T_CL))
    {
	out_str(T_CL);		/* clear the display */
	clear_cmdline = FALSE;
    }
    else
    {
	/* can't clear the screen, mark all chars with invalid attributes */
	for (i = 0; i < Rows; ++i)
	    lineinvalid(LineOffset[i], (int)Columns);
	clear_cmdline = TRUE;
    }

    screen_cleared = TRUE;	/* can use contents of ScreenLines now */

    win_rest_invalid(firstwin);
    redraw_cmdline = TRUE;
    if (must_redraw == CLEAR)	/* no need to clear again */
	must_redraw = NOT_VALID;
    compute_cmdrow();
    msg_row = cmdline_row;	/* put cursor on last line for messages */
    msg_col = 0;
    screen_start();		/* don't know where cursor is now */
    msg_scrolled = 0;		/* can't scroll back */
    msg_didany = FALSE;
    msg_didout = FALSE;
}

/*
 * Clear one line in ScreenLines.
 */
    static void
lineclear(off, width)
    unsigned	off;
    int		width;
{
    (void)vim_memset(ScreenLines + off, ' ', (size_t)width * sizeof(schar_T));
#ifdef FEAT_MBYTE
    if (enc_utf8)
	(void)vim_memset(ScreenLinesUC + off, 0,
					  (size_t)width * sizeof(u8char_T));
#endif
    (void)vim_memset(ScreenAttrs + off, 0, (size_t)width * sizeof(sattr_T));
}

/*
 * Mark one line in ScreenLines invalid by setting the attributes to an
 * invalid value.
 */
    static void
lineinvalid(off, width)
    unsigned	off;
    int		width;
{
    (void)vim_memset(ScreenAttrs + off, -1, (size_t)width * sizeof(sattr_T));
}

#ifdef FEAT_VERTSPLIT
/*
 * Copy part of a Screenline for vertically split window "wp".
 */
    static void
linecopy(to, from, wp)
    int		to;
    int		from;
    win_T	*wp;
{
    unsigned	off_to = LineOffset[to] + wp->w_wincol;
    unsigned	off_from = LineOffset[from] + wp->w_wincol;

    mch_memmove(ScreenLines + off_to, ScreenLines + off_from,
	    wp->w_width * sizeof(schar_T));
# ifdef FEAT_MBYTE
    if (enc_utf8)
    {
	mch_memmove(ScreenLinesUC + off_to, ScreenLinesUC + off_from,
		wp->w_width * sizeof(u8char_T));
	mch_memmove(ScreenLinesC1 + off_to, ScreenLinesC1 + off_from,
		wp->w_width * sizeof(u8char_T));
	mch_memmove(ScreenLinesC2 + off_to, ScreenLinesC2 + off_from,
		wp->w_width * sizeof(u8char_T));
    }
    if (enc_dbcs == DBCS_JPNU)
	mch_memmove(ScreenLines2 + off_to, ScreenLines2 + off_from,
		wp->w_width * sizeof(schar_T));
# endif
    mch_memmove(ScreenAttrs + off_to, ScreenAttrs + off_from,
	    wp->w_width * sizeof(sattr_T));
}
#endif

/*
 * Return TRUE if clearing with term string "p" would work.
 * It can't work when the string is empty or it won't set the right background.
 */
    int
can_clear(p)
    char_u	*p;
{
    return (*p != NUL && (t_colors <= 1
#ifdef FEAT_GUI
		|| gui.in_use
#endif
		|| cterm_normal_bg_color == 0 || *T_UT != NUL));
}

/*
 * Reset cursor position. Use whenever cursor was moved because of outputting
 * something directly to the screen (shell commands) or a terminal control
 * code.
 */
    void
screen_start()
{
    screen_cur_row = screen_cur_col = 9999;
}

/*
 * Note that the cursor has gone down to the next line, column 0.
 * Used for Ex mode.
 */
    void
screen_down()
{
    screen_cur_col = 0;
    if (screen_cur_row < Rows - 1)
	++screen_cur_row;
}

/*
 * Move the cursor to position "row","col" in the screen.
 * This tries to find the most efficient way to move, minimizing the number of
 * characters sent to the terminal.
 */
    void
windgoto(row, col)
    int	    row;
    int	    col;
{
    char_u	    *p;
    int		    i;
    int		    plan;
    int		    cost;
    int		    wouldbe_col;
    int		    noinvcurs;
    char_u	    *bs;
    int		    goto_cost;
    int		    attr;

#define GOTO_COST   7	/* asssume a term_windgoto() takes about 7 chars */
#define HIGHL_COST  5	/* assume unhighlight takes 5 chars */

#define PLAN_LE	    1
#define PLAN_CR	    2
#define PLAN_NL	    3
#define PLAN_WRITE  4
    /* Can't use ScreenLines unless initialized */
    if (ScreenLines == NULL)
	return;

    if (col != screen_cur_col || row != screen_cur_row)
    {
	/* Check for valid position. */
	if (row < 0)	/* window without text lines? */
	    row = 0;
	if (row >= screen_Rows)
	    row = screen_Rows - 1;
	if (col >= screen_Columns)
	    col = screen_Columns - 1;

	/* check if no cursor movement is allowed in highlight mode */
	if (screen_attr && *T_MS == NUL)
	    noinvcurs = HIGHL_COST;
	else
	    noinvcurs = 0;
	goto_cost = GOTO_COST + noinvcurs;

	/*
	 * Plan how to do the positioning:
	 * 1. Use CR to move it to column 0, same row.
	 * 2. Use T_LE to move it a few columns to the left.
	 * 3. Use NL to move a few lines down, column 0.
	 * 4. Move a few columns to the right with T_ND or by writing chars.
	 *
	 * Don't do this if the cursor went beyond the last column, the cursor
	 * position is unknown then (some terminals wrap, some don't )
	 *
	 * First check if the highlighting attibutes allow us to write
	 * characters to move the cursor to the right.
	 */
	if (row >= screen_cur_row && screen_cur_col < Columns)
	{
	    /*
	     * If the cursor is in the same row, bigger col, we can use CR
	     * or T_LE.
	     */
	    bs = NULL;			    /* init for GCC */
	    attr = screen_attr;
	    if (row == screen_cur_row && col < screen_cur_col)
	    {
		/* "le" is preferred over "bc", because "bc" is obsolete */
		if (*T_LE)
		    bs = T_LE;		    /* "cursor left" */
		else
		    bs = T_BC;		    /* "backspace character (old) */
		if (*bs)
		    cost = (screen_cur_col - col) * STRLEN(bs);
		else
		    cost = 999;
		if (col + 1 < cost)	    /* using CR is less characters */
		{
		    plan = PLAN_CR;
		    wouldbe_col = 0;
		    cost = 1;		    /* CR is just one character */
		}
		else
		{
		    plan = PLAN_LE;
		    wouldbe_col = col;
		}
		if (noinvcurs)		    /* will stop highlighting */
		{
		    cost += noinvcurs;
		    attr = 0;
		}
	    }

	    /*
	     * If the cursor is above where we want to be, we can use CR LF.
	     */
	    else if (row > screen_cur_row)
	    {
		plan = PLAN_NL;
		wouldbe_col = 0;
		cost = (row - screen_cur_row) * 2;  /* CR LF */
		if (noinvcurs)		    /* will stop highlighting */
		{
		    cost += noinvcurs;
		    attr = 0;
		}
	    }

	    /*
	     * If the cursor is in the same row, smaller col, just use write.
	     */
	    else
	    {
		plan = PLAN_WRITE;
		wouldbe_col = screen_cur_col;
		cost = 0;
	    }

	    /*
	     * Check if any characters that need to be written have the
	     * correct attributes.  Also avoid UTF-8 characters.
	     */
	    i = col - wouldbe_col;
	    if (i > 0)
		cost += i;
	    if (cost < goto_cost && i > 0)
	    {
		/*
		 * Check if the attributes are correct without additionally
		 * stopping highlighting.
		 */
		p = ScreenAttrs + LineOffset[row] + wouldbe_col;
		while (i && *p++ == attr)
		    --i;
		if (i != 0)
		{
		    /*
		     * Try if it works when highlighting is stopped here.
		     */
		    if (*--p == 0)
		    {
			cost += noinvcurs;
			while (i && *p++ == 0)
			    --i;
		    }
		    if (i != 0)
			cost = 999;	/* different attributes, don't do it */
		}
#ifdef FEAT_MBYTE
		if (enc_utf8)
		{
		    /* Don't use an UTF-8 char for positioning, it's slow. */
		    i = col - wouldbe_col;
		    p = ScreenLines + LineOffset[row] + wouldbe_col;
		    while (i && MB_BYTE2LEN(*p++) == 1)
			--i;
		    if (i != 0)
			cost = 999;
		}
#endif
	    }

	    /*
	     * We can do it without term_windgoto()!
	     */
	    if (cost < goto_cost)
	    {
		if (plan == PLAN_LE)
		{
		    if (noinvcurs)
			screen_stop_highlight();
		    while (screen_cur_col > col)
		    {
			out_str(bs);
			--screen_cur_col;
		    }
		}
		else if (plan == PLAN_CR)
		{
		    if (noinvcurs)
			screen_stop_highlight();
		    out_char('\r');
		    screen_cur_col = 0;
		}
		else if (plan == PLAN_NL)
		{
		    if (noinvcurs)
			screen_stop_highlight();
		    while (screen_cur_row < row)
		    {
			out_char('\n');
			++screen_cur_row;
		    }
		    screen_cur_col = 0;
		}

		i = col - screen_cur_col;
		if (i > 0)
		{
		    /*
		     * Use cursor-right if it's one character only.  Avoids
		     * removing a line of pixels from the last bold char, when
		     * using the bold trick in the GUI.
		     */
		    if (T_ND[0] != NUL && T_ND[1] == NUL)
		    {
			while (i-- > 0)
			    out_char(*T_ND);
		    }
		    else
		    {
			int	off;

			off = LineOffset[row] + screen_cur_col;
			while (i-- > 0)
			{
			    if (ScreenAttrs[off] != screen_attr)
				screen_stop_highlight();
			    out_char(ScreenLines[off]);
#ifdef FEAT_MBYTE
			    if (enc_dbcs == DBCS_JPNU
						  && ScreenLines[off] == 0x8e)
				out_char(ScreenLines2[off]);
#endif
			    ++off;
			}
		    }
		}
	    }
	}
	else
	    cost = 999;

	if (cost >= goto_cost)
	{
	    if (noinvcurs)
		screen_stop_highlight();
	    if (row == screen_cur_row && (col > screen_cur_col) &&
								*T_CRI != NUL)
		term_cursor_right(col - screen_cur_col);
	    else
		term_windgoto(row, col);
	}
	screen_cur_row = row;
	screen_cur_col = col;
    }
}

/*
 * Set cursor to its position in the current window.
 */
    void
setcursor()
{
    if (redrawing())
    {
	validate_cursor();
	windgoto(W_WINROW(curwin) + curwin->w_wrow,
		W_WINCOL(curwin) + (
#ifdef FEAT_RIGHTLEFT
		curwin->w_p_rl ? ((int)W_WIDTH(curwin) - 1 - curwin->w_wcol) :
#endif
							    curwin->w_wcol));
    }
}


/*
 * insert 'line_count' lines at 'row' in window 'wp'
 * if 'invalid' is TRUE the wp->w_lines[].wl_lnum is invalidated.
 * if 'mayclear' is TRUE the screen will be cleared if it is faster than
 * scrolling.
 * Returns FAIL if the lines are not inserted, OK for success.
 */
    int
win_ins_lines(wp, row, line_count, invalid, mayclear)
    win_T	*wp;
    int		row;
    int		line_count;
    int		invalid;
    int		mayclear;
{
    int		did_delete;
    int		nextrow;
    int		lastrow;
    int		retval;

    if (invalid)
	wp->w_lines_valid = 0;

    if (wp->w_height < 5)
	return FAIL;

    if (line_count > wp->w_height - row)
	line_count = wp->w_height - row;

    retval = win_do_lines(wp, row, line_count, mayclear, FALSE);
    if (retval != MAYBE)
	return retval;

    /*
     * If there is a next window or a status line, we first try to delete the
     * lines at the bottom to avoid messing what is after the window.
     * If this fails and there are following windows, don't do anything to avoid
     * messing up those windows, better just redraw.
     */
    did_delete = FALSE;
#ifdef FEAT_WINDOWS
    if (wp->w_next != NULL || wp->w_status_height)
    {
	if (screen_del_lines(0, W_WINROW(wp) + wp->w_height - line_count,
				    line_count, (int)Rows, FALSE, NULL) == OK)
	    did_delete = TRUE;
	else if (wp->w_next)
	    return FAIL;
    }
#endif
    /*
     * if no lines deleted, blank the lines that will end up below the window
     */
    if (!did_delete)
    {
#ifdef FEAT_WINDOWS
	wp->w_redr_status = TRUE;
#endif
	redraw_cmdline = TRUE;
	nextrow = W_WINROW(wp) + wp->w_height + W_STATUS_HEIGHT(wp);
	lastrow = nextrow + line_count;
	if (lastrow > Rows)
	    lastrow = Rows;
	screen_fill(nextrow - line_count, lastrow - line_count,
		  W_WINCOL(wp), (int)W_ENDCOL(wp),
		  ' ', ' ', 0);
    }

    if (screen_ins_lines(0, W_WINROW(wp) + row, line_count, (int)Rows, NULL)
								      == FAIL)
    {
	    /* deletion will have messed up other windows */
	if (did_delete)
	{
#ifdef FEAT_WINDOWS
	    wp->w_redr_status = TRUE;
#endif
	    win_rest_invalid(W_NEXT(wp));
	}
	return FAIL;
    }

    return OK;
}

/*
 * delete "line_count" window lines at "row" in window "wp"
 * If "invalid" is TRUE curwin->w_lines[] is invalidated.
 * If "mayclear" is TRUE the screen will be cleared if it is faster than
 * scrolling
 * Return OK for success, FAIL if the lines are not deleted.
 */
    int
win_del_lines(wp, row, line_count, invalid, mayclear)
    win_T	*wp;
    int		row;
    int		line_count;
    int		invalid;
    int		mayclear;
{
    int		retval;

    if (invalid)
	wp->w_lines_valid = 0;

    if (line_count > wp->w_height - row)
	line_count = wp->w_height - row;

    retval = win_do_lines(wp, row, line_count, mayclear, TRUE);
    if (retval != MAYBE)
	return retval;

    if (screen_del_lines(0, W_WINROW(wp) + row, line_count,
					      (int)Rows, FALSE, NULL) == FAIL)
	return FAIL;

#ifdef FEAT_WINDOWS
    /*
     * If there are windows or status lines below, try to put them at the
     * correct place. If we can't do that, they have to be redrawn.
     */
    if (wp->w_next || wp->w_status_height || cmdline_row < Rows - 1)
    {
	if (screen_ins_lines(0, W_WINROW(wp) + wp->w_height - line_count,
					 line_count, (int)Rows, NULL) == FAIL)
	{
	    wp->w_redr_status = TRUE;
	    win_rest_invalid(wp->w_next);
	}
    }
    /*
     * If this is the last window and there is no status line, redraw the
     * command line later.
     */
    else
#endif
	redraw_cmdline = TRUE;
    return OK;
}

/*
 * Common code for win_ins_lines() and win_del_lines().
 * Returns OK or FAIL when the work has been done.
 * Returns MAYBE when not finished yet.
 */
    static int
win_do_lines(wp, row, line_count, mayclear, del)
    win_T	*wp;
    int		row;
    int		line_count;
    int		mayclear;
    int		del;
{
    int		retval;

    if (!redrawing() || line_count <= 0)
	return FAIL;

    /* only a few lines left: redraw is faster */
    if (mayclear && Rows - line_count < 5
#ifdef FEAT_VERTSPLIT
	    && wp->w_width == Columns
#endif
	    )
    {
	screenclear();	    /* will set wp->w_lines_valid to 0 */
	return FAIL;
    }

    /*
     * Delete all remaining lines
     */
    if (row + line_count >= wp->w_height)
    {
	screen_fill(W_WINROW(wp) + row, W_WINROW(wp) + wp->w_height,
		W_WINCOL(wp), (int)W_ENDCOL(wp),
		' ', ' ', 0);
	return OK;
    }

    /*
     * when scrolling, the message on the command line should be cleared,
     * otherwise it will stay there forever.
     */
    clear_cmdline = TRUE;

    /*
     * If the terminal can set a scroll region, use that.
     * Always do this in a vertically split window.  This will redraw from
     * ScreenLines[] when t_CV isn't defined.  That's faster than using
     * win_line().
     */
    if (scroll_region
#ifdef FEAT_VERTSPLIT
	    || W_WIDTH(wp) != Columns
#endif
	    )
    {
#ifdef FEAT_VERTSPLIT
	if (scroll_region)
#endif
	    scroll_region_set(wp, row);
	if (del)
	    retval = screen_del_lines(W_WINROW(wp) + row, 0, line_count,
					       wp->w_height - row, FALSE, wp);
	else
	    retval = screen_ins_lines(W_WINROW(wp) + row, 0, line_count,
						      wp->w_height - row, wp);
#ifdef FEAT_VERTSPLIT
	if (scroll_region)
#endif
	    scroll_region_reset();
	return retval;
    }

#ifdef FEAT_WINDOWS
    if (wp->w_next != NULL && p_tf) /* don't delete/insert on fast terminal */
	return FAIL;
#endif

    return MAYBE;
}

/*
 * window 'wp' and everything after it is messed up, mark it for redraw
 */
    static void
win_rest_invalid(wp)
    win_T	*wp;
{
#ifdef FEAT_WINDOWS
    while (wp != NULL)
#else
    if (wp != NULL)
#endif
    {
	redraw_win_later(wp, NOT_VALID);
#ifdef FEAT_WINDOWS
	wp->w_redr_status = TRUE;
	wp = wp->w_next;
#endif
    }
    redraw_cmdline = TRUE;
}

/*
 * The rest of the routines in this file perform screen manipulations. The
 * given operation is performed physically on the screen. The corresponding
 * change is also made to the internal screen image. In this way, the editor
 * anticipates the effect of editing changes on the appearance of the screen.
 * That way, when we call screenupdate a complete redraw isn't usually
 * necessary. Another advantage is that we can keep adding code to anticipate
 * screen changes, and in the meantime, everything still works.
 */

/*
 * types for inserting or deleting lines
 */
#define USE_T_CAL   1
#define USE_T_CDL   2
#define USE_T_AL    3
#define USE_T_CE    4
#define USE_T_DL    5
#define USE_T_SR    6
#define USE_NL	    7
#define USE_T_CD    8
#define USE_REDRAW  9

/*
 * insert lines on the screen and update ScreenLines[]
 * 'end' is the line after the scrolled part. Normally it is Rows.
 * When scrolling region used 'off' is the offset from the top for the region.
 * 'row' and 'end' are relative to the start of the region.
 *
 * return FAIL for failure, OK for success.
 */
    static int
screen_ins_lines(off, row, line_count, end, wp)
    int		off;
    int		row;
    int		line_count;
    int		end;
    win_T	*wp;	    /* NULL or window to use width from */
{
    int		i;
    int		j;
    unsigned	temp;
    int		cursor_row;
    int		type;
    int		result_empty;
    int		can_ce = can_clear(T_CE);

    /*
     * FAIL if
     * - there is no valid screen
     * - the screen has to be redrawn completely
     * - the line count is less than one
     * - the line count is more than 'ttyscroll'
     */
    if (!screen_valid(TRUE) || line_count <= 0 || line_count > p_ttyscroll)
	return FAIL;

    /*
     * There are seven ways to insert lines:
     * 0. When in a vertically split window and t_CV isn't set, redraw the
     *    characters from ScreenLines[].
     * 1. Use T_CD (clear to end of display) if it exists and the result of
     *	  the insert is just empty lines
     * 2. Use T_CAL (insert multiple lines) if it exists and T_AL is not
     *	  present or line_count > 1. It looks better if we do all the inserts
     *	  at once.
     * 3. Use T_CDL (delete multiple lines) if it exists and the result of the
     *	  insert is just empty lines and T_CE is not present or line_count >
     *	  1.
     * 4. Use T_AL (insert line) if it exists.
     * 5. Use T_CE (erase line) if it exists and the result of the insert is
     *	  just empty lines.
     * 6. Use T_DL (delete line) if it exists and the result of the insert is
     *	  just empty lines.
     * 7. Use T_SR (scroll reverse) if it exists and inserting at row 0 and
     *	  the 'da' flag is not set or we have clear line capability.
     * 8. redraw the characters from ScreenLines[].
     *
     * Careful: In a hpterm scroll reverse doesn't work as expected, it moves
     * the scrollbar for the window. It does have insert line, use that if it
     * exists.
     */
    result_empty = (row + line_count >= end);
#ifdef FEAT_VERTSPLIT
    if (wp != NULL && wp->w_width != Columns && *T_CSV == NUL)
	type = USE_REDRAW;
    else
#endif
    if (can_clear(T_CD) && result_empty)
	type = USE_T_CD;
    else if (*T_CAL != NUL && (line_count > 1 || *T_AL == NUL))
	type = USE_T_CAL;
    else if (*T_CDL != NUL && result_empty && (line_count > 1 || !can_ce))
	type = USE_T_CDL;
    else if (*T_AL != NUL)
	type = USE_T_AL;
    else if (can_ce && result_empty)
	type = USE_T_CE;
    else if (*T_DL != NUL && result_empty)
	type = USE_T_DL;
    else if (*T_SR != NUL && row == 0 && (*T_DA == NUL || can_ce))
	type = USE_T_SR;
    else
	return FAIL;

    /*
     * For clearing the lines screen_del_lines() is used. This will also take
     * care of t_db if necessary.
     */
    if (type == USE_T_CD || type == USE_T_CDL ||
					 type == USE_T_CE || type == USE_T_DL)
	return screen_del_lines(off, row, line_count, end, FALSE, wp);

    /*
     * If text is retained below the screen, first clear or delete as many
     * lines at the bottom of the window as are about to be inserted so that
     * the deleted lines won't later surface during a screen_del_lines.
     */
    if (*T_DB)
	screen_del_lines(off, end - line_count, line_count, end, FALSE, wp);

#ifdef FEAT_CLIPBOARD
    /* Remove a modeless selection when inserting lines halfway the screen
     * or not the full width of the screen. */
    if (off + row > 0
# ifdef FEAT_VERTSPLIT
	    || (wp != NULL && wp->w_width != Columns)
# endif
       )
	clip_clear_selection();
    else
	clip_scroll_selection(-line_count);
#endif

#ifdef FEAT_GUI_BEOS
    vim_lock_screen();
#endif
#ifdef FEAT_GUI
    /* Don't update the GUI cursor here, ScreenLines[] is invalid until the
     * scrolling is actually carried out. */
    gui_dont_update_cursor();
#endif

    if (*T_CCS != NUL)	   /* cursor relative to region */
	cursor_row = row;
    else
	cursor_row = row + off;

    /*
     * Shift LineOffset[] line_count down to reflect the inserted lines.
     * Clear the inserted lines in ScreenLines[].
     */
    row += off;
    end += off;
    for (i = 0; i < line_count; ++i)
    {
#ifdef FEAT_VERTSPLIT
	if (wp != NULL && wp->w_width != Columns)
	{
	    /* need to copy part of a line */
	    j = end - 1 - i;
	    while ((j -= line_count) >= row)
		linecopy(j + line_count, j, wp);
	    j += line_count;
	    if (can_clear((char_u *)" "))
		lineclear(LineOffset[j] + wp->w_wincol, wp->w_width);
	    else
		lineinvalid(LineOffset[j] + wp->w_wincol, wp->w_width);
	}
	else
#endif
	{
	    j = end - 1 - i;
	    temp = LineOffset[j];
	    while ((j -= line_count) >= row)
		LineOffset[j + line_count] = LineOffset[j];
	    LineOffset[j + line_count] = temp;
	    if (can_clear((char_u *)" "))
		lineclear(temp, (int)Columns);
	    else
		lineinvalid(temp, (int)Columns);
	}
    }
#ifdef FEAT_GUI_BEOS
    vim_unlock_screen();
#endif

    screen_stop_highlight();
    windgoto(cursor_row, 0);

#ifdef FEAT_VERTSPLIT
    /* redraw the characters */
    if (type == USE_REDRAW)
	redraw_block(row, end, wp);
    else
#endif
	if (type == USE_T_CAL)
    {
	term_append_lines(line_count);
	screen_start();		/* don't know where cursor is now */
    }
    else
    {
	for (i = 0; i < line_count; i++)
	{
	    if (type == USE_T_AL)
	    {
		if (i && cursor_row != 0)
		    windgoto(cursor_row, 0);
		out_str(T_AL);
	    }
	    else  /* type == USE_T_SR */
		out_str(T_SR);
	    screen_start();	    /* don't know where cursor is now */
	}
    }

    /*
     * With scroll-reverse and 'da' flag set we need to clear the lines that
     * have been scrolled down into the region.
     */
    if (type == USE_T_SR && *T_DA)
    {
	for (i = 0; i < line_count; ++i)
	{
	    windgoto(off + i, 0);
	    out_str(T_CE);
	    screen_start();	    /* don't know where cursor is now */
	}
    }

#ifdef FEAT_GUI
    gui_can_update_cursor();
    if (gui.in_use)
	out_flush();	/* always flush after a scroll */
#endif
    return OK;
}

/*
 * delete lines on the screen and update ScreenLines[]
 * 'end' is the line after the scrolled part. Normally it is Rows.
 * When scrolling region used 'off' is the offset from the top for the region.
 * 'row' and 'end' are relative to the start of the region.
 *
 * Return OK for success, FAIL if the lines are not deleted.
 */
/*ARGSUSED*/
    int
screen_del_lines(off, row, line_count, end, force, wp)
    int		off;
    int		row;
    int		line_count;
    int		end;
    int		force;		/* even when line_count > p_ttyscroll */
    win_T	*wp;		/* NULL or window to use width from */
{
    int		j;
    int		i;
    unsigned	temp;
    int		cursor_row;
    int		cursor_end;
    int		result_empty;	/* result is empty until end of region */
    int		can_delete;	/* deleting line codes can be used */
    int		type;

    /*
     * FAIL if
     * - there is no valid screen
     * - the screen has to be redrawn completely
     * - the line count is less than one
     * - the line count is more than 'ttyscroll'
     */
    if (!screen_valid(TRUE) || line_count <= 0 ||
					 (!force && line_count > p_ttyscroll))
	return FAIL;

    /*
     * Check if the rest of the current region will become empty.
     */
    result_empty = row + line_count >= end;

    /*
     * We can delete lines only when 'db' flag not set or when 'ce' option
     * available.
     */
    can_delete = (*T_DB == NUL || can_clear(T_CE));

    /*
     * There are six ways to delete lines:
     * 0. When in a vertically split window and t_CV isn't set, redraw the
     *    characters from ScreenLines[].
     * 1. Use T_CD if it exists and the result is empty.
     * 2. Use newlines if row == 0 and count == 1 or T_CDL does not exist.
     * 3. Use T_CDL (delete multiple lines) if it exists and line_count > 1 or
     *	  none of the other ways work.
     * 4. Use T_CE (erase line) if the result is empty.
     * 5. Use T_DL (delete line) if it exists.
     * 6. redraw the characters from ScreenLines[].
     */
#ifdef FEAT_VERTSPLIT
    if (wp != NULL && wp->w_width != Columns && *T_CSV == NUL)
	type = USE_REDRAW;
    else
#endif
    if (can_clear(T_CD) && result_empty)
	type = USE_T_CD;
#if defined(__BEOS__) && defined(BEOS_DR8)
    /*
     * USE_NL does not seem to work in Terminal of DR8 so we set T_DB="" in
     * its internal termcap... this works okay for tests which test *T_DB !=
     * NUL.  It has the disadvantage that the user cannot use any :set t_*
     * command to get T_DB (back) to empty_option, only :set term=... will do
     * the trick...
     * Anyway, this hack will hopefully go away with the next OS release.
     * (Olaf Seibert)
     */
    else if (row == 0 && T_DB == empty_option
					&& (line_count == 1 || *T_CDL == NUL))
#else
    else if (row == 0 && (
#ifndef AMIGA
	/* On the Amiga, somehow '\n' on the last line doesn't always scroll
	 * up, so use delete-line command */
			    line_count == 1 ||
#endif
						*T_CDL == NUL))
#endif
	type = USE_NL;
    else if (*T_CDL != NUL && line_count > 1 && can_delete)
	type = USE_T_CDL;
    else if (can_clear(T_CE) && result_empty
#ifdef FEAT_VERTSPLIT
	    && (wp == NULL || wp->w_width == Columns)
#endif
	    )
	type = USE_T_CE;
    else if (*T_DL != NUL && can_delete)
	type = USE_T_DL;
    else if (*T_CDL != NUL && can_delete)
	type = USE_T_CDL;
    else
	return FAIL;

#ifdef FEAT_CLIPBOARD
    /* Remove a modeless selection when deleting lines halfway the screen or
     * not the full width of the screen. */
    if (off + row > 0
# ifdef FEAT_VERTSPLIT
	    || (wp != NULL && wp->w_width != Columns)
# endif
       )
	clip_clear_selection();
    else
	clip_scroll_selection(line_count);
#endif

#ifdef FEAT_GUI_BEOS
    vim_lock_screen();
#endif
#ifdef FEAT_GUI
    /* Don't update the GUI cursor here, ScreenLines[] is invalid until the
     * scrolling is actually carried out. */
    gui_dont_update_cursor();
#endif

    if (*T_CCS != NUL)	    /* cursor relative to region */
    {
	cursor_row = row;
	cursor_end = end;
    }
    else
    {
	cursor_row = row + off;
	cursor_end = end + off;
    }

    /*
     * Now shift LineOffset[] line_count up to reflect the deleted lines.
     * Clear the inserted lines in ScreenLines[].
     */
    row += off;
    end += off;
    for (i = 0; i < line_count; ++i)
    {
#ifdef FEAT_VERTSPLIT
	if (wp != NULL && wp->w_width != Columns)
	{
	    /* need to copy part of a line */
	    j = row + i;
	    while ((j += line_count) <= end - 1)
		linecopy(j - line_count, j, wp);
	    j -= line_count;
	    if (can_clear((char_u *)" "))
		lineclear(LineOffset[j] + wp->w_wincol, wp->w_width);
	    else
		lineinvalid(LineOffset[j] + wp->w_wincol, wp->w_width);
	}
	else
#endif
	{
	    /* whole width, moving the line pointers is faster */
	    j = row + i;
	    temp = LineOffset[j];
	    while ((j += line_count) <= end - 1)
		LineOffset[j - line_count] = LineOffset[j];
	    LineOffset[j - line_count] = temp;
	    if (can_clear((char_u *)" "))
		lineclear(temp, (int)Columns);
	    else
		lineinvalid(temp, (int)Columns);
	}
    }
#ifdef FEAT_GUI_BEOS
    vim_unlock_screen();
#endif

    screen_stop_highlight();

#ifdef FEAT_VERTSPLIT
    /* redraw the characters */
    if (type == USE_REDRAW)
	redraw_block(row, end, wp);
    else
#endif
	if (type == USE_T_CD)	/* delete the lines */
    {
	windgoto(cursor_row, 0);
	out_str(T_CD);
	screen_start();			/* don't know where cursor is now */
    }
    else if (type == USE_T_CDL)
    {
	windgoto(cursor_row, 0);
	term_delete_lines(line_count);
	screen_start();			/* don't know where cursor is now */
    }
    /*
     * Deleting lines at top of the screen or scroll region: Just scroll
     * the whole screen (scroll region) up by outputting newlines on the
     * last line.
     */
    else if (type == USE_NL)
    {
	windgoto(cursor_end - 1, 0);
	for (i = line_count; --i >= 0; )
	    out_char('\n');		/* cursor will remain on same line */
    }
    else
    {
	for (i = line_count; --i >= 0; )
	{
	    if (type == USE_T_DL)
	    {
		windgoto(cursor_row, 0);
		out_str(T_DL);		/* delete a line */
	    }
	    else /* type == USE_T_CE */
	    {
		windgoto(cursor_row + i, 0);
		out_str(T_CE);		/* erase a line */
	    }
	    screen_start();		/* don't know where cursor is now */
	}
    }

    /*
     * If the 'db' flag is set, we need to clear the lines that have been
     * scrolled up at the bottom of the region.
     */
    if (*T_DB && (type == USE_T_DL || type == USE_T_CDL))
    {
	for (i = line_count; i > 0; --i)
	{
	    windgoto(cursor_end - i, 0);
	    out_str(T_CE);		/* erase a line */
	    screen_start();		/* don't know where cursor is now */
	}
    }

#ifdef FEAT_GUI
    gui_can_update_cursor();
    if (gui.in_use)
	out_flush();	/* always flush after a scroll */
#endif

    return OK;
}

/*
 * show the current mode and ruler
 *
 * If clear_cmdline is TRUE, clear the rest of the cmdline.
 * If clear_cmdline is FALSE there may be a message there that needs to be
 * cleared only if a mode is shown.
 * Return the length of the message (0 if no message).
 */
    int
showmode()
{
    int		need_clear;
    int		length = 0;
    int		do_mode;
    int		attr;
    int		nwr_save;
#ifdef FEAT_INS_EXPAND
    int		sub_attr;
#endif

    do_mode = (p_smd && ((State & INSERT) || restart_edit
#ifdef FEAT_VISUAL
		|| VIsual_active
#endif
		));
    if (do_mode || Recording)
    {
	/*
	 * Don't show mode right now, when not redrawing or inside a mapping.
	 * Call char_avail() only when we are going to show something, because
	 * it takes a bit of time.
	 */
	if (!redrawing() || (char_avail() && !KeyTyped))
	{
	    redraw_cmdline = TRUE;		/* show mode later */
	    return 0;
	}

	nwr_save = need_wait_return;

	/* wait a bit before overwriting an important message */
	check_for_delay(FALSE);

	/* if the cmdline is more than one line high, erase top lines */
	need_clear = clear_cmdline;
	if (clear_cmdline && cmdline_row < Rows - 1)
	    msg_clr_cmdline();			/* will reset clear_cmdline */

	/* Position on the last line in the window, column 0 */
	msg_pos_mode();
	cursor_off();
	attr = hl_attr(HLF_CM);			/* Highlight mode */
	if (do_mode)
	{
	    MSG_PUTS_ATTR("--", attr);
#if defined(FEAT_XIM)
	    if (xim_preediting)
		MSG_PUTS_ATTR(" XIM", attr);
#endif
#if defined(FEAT_HANGULIN) && defined(FEAT_GUI)
	    if (gui.in_use)
	    {
		if (hangul_input_state_get())
		    MSG_PUTS_ATTR(" �ѱ�", attr);   /* HANGUL */
	    }
#endif
#ifdef FEAT_INS_EXPAND
	    if (edit_submode != NULL)		/* CTRL-X in Insert mode */
	    {
		if (edit_submode_pre != NULL)
		    msg_puts_attr(edit_submode_pre, attr);
		msg_puts_attr(edit_submode, attr);
		if (edit_submode_extra != NULL)
		{
		    MSG_PUTS_ATTR(" ", attr);	/* add a space in between */
		    if ((int)edit_submode_highl < (int)HLF_COUNT)
			sub_attr = hl_attr(edit_submode_highl);
		    else
			sub_attr = attr;
		    msg_puts_attr(edit_submode_extra, sub_attr);
		}
	    }
	    else
#endif
	    {
		if (State & VREPLACE_FLAG)
		    MSG_PUTS_ATTR(_(" VREPLACE"), attr);
		else if (State & REPLACE_FLAG)
		    MSG_PUTS_ATTR(_(" REPLACE"), attr);
		else if (State & INSERT)
		{
#ifdef FEAT_RIGHTLEFT
		    if (p_ri)
			MSG_PUTS_ATTR(_(" REVERSE"), attr);
#endif
		    MSG_PUTS_ATTR(_(" INSERT"), attr);
		}
		else if (restart_edit == 'I')
		    MSG_PUTS_ATTR(_(" (insert)"), attr);
		else if (restart_edit == 'R')
		    MSG_PUTS_ATTR(_(" (replace)"), attr);
		else if (restart_edit == 'V')
		    MSG_PUTS_ATTR(_(" (vreplace)"), attr);
#ifdef FEAT_RIGHTLEFT
		if (p_hkmap)
		    MSG_PUTS_ATTR(_(" Hebrew"), attr);
# ifdef FEAT_FKMAP
		if (p_fkmap)
		    MSG_PUTS_ATTR(farsi_text_5, attr);
# endif
#endif
#ifdef FEAT_KEYMAP
		if (State & LANGMAP)
		    MSG_PUTS_ATTR(_(" (lang)"), attr);
#endif
		if ((State & INSERT) && p_paste)
		    MSG_PUTS_ATTR(_(" (paste)"), attr);

#ifdef FEAT_VISUAL
		if (VIsual_active)
		{
		    if (VIsual_select)
			MSG_PUTS_ATTR(_(" SELECT"), attr);
		    else
			MSG_PUTS_ATTR(_(" VISUAL"), attr);
		    if (VIsual_mode == Ctrl_V)
			MSG_PUTS_ATTR(_(" BLOCK"), attr);
		    else if (VIsual_mode == 'V')
			MSG_PUTS_ATTR(_(" LINE"), attr);
		}
#endif
	    }
	    MSG_PUTS_ATTR(" --", attr);
	    need_clear = TRUE;
	}
	if (Recording)
	{
	    MSG_PUTS_ATTR(_("recording"), attr);
	    need_clear = TRUE;
	}
	if (need_clear || clear_cmdline)
	    msg_clr_eos();
	msg_didout = FALSE;		/* overwrite this message */
	length = msg_col;
	msg_col = 0;
	need_wait_return = nwr_save;	/* never ask for hit-return for this */
    }
    else if (clear_cmdline)		/* just clear anything */
	msg_clr_cmdline();		/* will reset clear_cmdline */

#ifdef FEAT_CMDL_INFO
    /* If the last window has no status line, the ruler is after the mode
     * message and must be redrawn */
# ifdef FEAT_WINDOWS
    if (lastwin->w_status_height == 0)
# endif
	win_redr_ruler(lastwin, TRUE);
#endif
    redraw_cmdline = FALSE;
    clear_cmdline = FALSE;

    return length;
}

/*
 * Position for a mode message.
 */
    static void
msg_pos_mode()
{
    msg_col = 0;
    msg_row = Rows - 1;
}

/*
 * Delete mode message.  Used when ESC is typed which is expected to end
 * Insert mode (but Insert mode didn't end yet!).
 */
    void
unshowmode(force)
    int	    force;
{
    /*
     * Don't delete it right now, when not redrawing or insided a mapping.
     */
    if (!redrawing() || (!force && char_avail() && !KeyTyped))
	redraw_cmdline = TRUE;		/* delete mode later */
    else
    {
	msg_pos_mode();
	if (Recording)
	    MSG_PUTS_ATTR(_("recording"), hl_attr(HLF_CM));
	msg_clr_eos();
    }
}

#if defined(FEAT_WINDOWS) || defined(FEAT_WILDMENU) || defined(FEAT_STL_OPT)
/*
 * Get the character to use in a status line.  Get its attributes in "*attr".
 */
    static int
fillchar_status(attr, is_curwin)
    int		*attr;
    int		is_curwin;
{
    int fill;
    if (is_curwin)
    {
	*attr = hl_attr(HLF_S);
	fill = fill_stl;
    }
    else
    {
	*attr = hl_attr(HLF_SNC);
	fill = fill_stlnc;
    }
    /* Use fill when there is highlighting, and highlighting of current
     * window differs, or the fillchars differ, or this is not the
     * current window */
    if (*attr != 0 && ((hl_attr(HLF_S) != hl_attr(HLF_SNC)
			|| !is_curwin || firstwin == lastwin)
		    || (fill_stl != fill_stlnc)))
	return fill;
    if (is_curwin)
	return '^';
    return '=';
}
#endif

#ifdef FEAT_VERTSPLIT
/*
 * Get the character to use in a separator between vertically split windows.
 * Get its attributes in "*attr".
 */
    static int
fillchar_vsep(attr)
    int	    *attr;
{
    *attr = hl_attr(HLF_C);
    if (*attr == 0 && fill_vert == ' ')
	return '|';
    else
	return fill_vert;
}
#endif

/*
 * Return TRUE if redrawing should currently be done.
 */
    int
redrawing()
{
    return (!RedrawingDisabled && !(p_lz && char_avail() && !KeyTyped));
}

/*
 * Return TRUE if printing messages should currently be done.
 */
    int
messaging()
{
    return (!(p_lz && char_avail() && !KeyTyped));
}

/*
 * Show current status info in ruler and various other places
 * If always is FALSE, only show ruler if position has changed.
 */
    void
showruler(always)
    int	    always;
{
    if (!always && !redrawing())
	return;
#if defined(FEAT_STL_OPT) && defined(FEAT_WINDOWS)
    if (*p_stl && curwin->w_status_height)
	win_redr_custom(curwin, FALSE);
    else
#endif
#ifdef FEAT_CMDL_INFO
	win_redr_ruler(curwin, always);
#endif

#ifdef FEAT_TITLE
    if (need_maketitle
# ifdef FEAT_STL_OPT
	    || (p_icon && (stl_syntax & STL_IN_ICON))
	    || (p_title && (stl_syntax & STL_IN_TITLE))
# endif
       )
	maketitle();
#endif
}

#ifdef FEAT_CMDL_INFO
    static void
win_redr_ruler(wp, always)
    win_T	*wp;
    int		always;
{
    char_u	buffer[70];
    int		row;
    int		fillchar;
    int		attr;
    int		empty_line = FALSE;
    colnr_T	virtcol;
    int		i;
    int		o;
#ifdef FEAT_VERTSPLIT
    int		this_ru_col;
    int		off = 0;
    int		width = Columns;
# define WITH_OFF(x) x
# define WITH_WIDTH(x) x
#else
# define WITH_OFF(x) 0
# define WITH_WIDTH(x) Columns
# define this_ru_col ru_col
#endif

    /* If 'ruler' off or redrawing disabled, don't do anything */
    if (!p_ru)
	return;

    /*
     * Check if cursor.lnum is valid, since win_redr_ruler() may be called
     * after deleting lines, before cursor.lnum is corrected.
     */
    if (wp->w_cursor.lnum > wp->w_buffer->b_ml.ml_line_count)
	return;

#ifdef FEAT_STL_OPT
    if (*p_ruf)
    {
	win_redr_custom(wp, TRUE);
	return;
    }
#endif

    /*
     * Check if the line is empty (will show "0-1").
     */
    if (*ml_get_buf(wp->w_buffer, wp->w_cursor.lnum, FALSE) == NUL)
	empty_line = TRUE;

    /*
     * Only draw the ruler when something changed.
     */
    validate_virtcol_win(wp);
    if (       redraw_cmdline
	    || always
	    || wp->w_cursor.lnum != wp->w_ru_cursor.lnum
	    || wp->w_cursor.col != wp->w_ru_cursor.col
	    || wp->w_virtcol != wp->w_ru_virtcol
#ifdef FEAT_VIRTUALEDIT
	    || wp->w_cursor.coladd != wp->w_ru_cursor.coladd
#endif
	    || wp->w_topline != wp->w_ru_topline
#ifdef FEAT_DIFF
	    || wp->w_topfill != wp->w_ru_topfill
#endif
	    || empty_line != wp->w_ru_empty)
    {
	cursor_off();
#ifdef FEAT_WINDOWS
	if (wp->w_status_height)
	{
	    row = W_WINROW(wp) + wp->w_height;
	    fillchar = fillchar_status(&attr, wp == curwin);
# ifdef FEAT_VERTSPLIT
	    off = W_WINCOL(wp);
	    width = W_WIDTH(wp);
# endif
	}
	else
#endif
	{
	    row = Rows - 1;
	    fillchar = ' ';
	    attr = 0;
#ifdef FEAT_VERTSPLIT
	    width = Columns;
	    off = 0;
#endif
	}

	/* In list mode virtcol needs to be recomputed */
	virtcol = wp->w_virtcol;
	if (wp->w_p_list && lcs_tab1 == NUL)
	{
	    wp->w_p_list = FALSE;
	    getvcol(wp, &wp->w_cursor, NULL, &virtcol, NULL);
	    wp->w_p_list = TRUE;
	}
#ifdef FEAT_VIRTUALEDIT
	virtcol += wp->w_cursor.coladd;
#endif

	/*
	 * Some sprintfs return the length, some return a pointer.
	 * To avoid portability problems we use strlen() here.
	 */
	sprintf((char *)buffer, "%ld,",
		(wp->w_buffer->b_ml.ml_flags & ML_EMPTY)
		    ? 0L
		    : (long)(wp->w_cursor.lnum));
	col_print(buffer + STRLEN(buffer),
		!(State & INSERT) && empty_line
		    ? 0
		    : (int)wp->w_cursor.col + 1,
		(int)virtcol + 1);

	/*
	 * Add a "50%" if there is room for it.
	 * On the last line, don't print in the last column (scrolls the
	 * screen up on some terminals).
	 */
	i = STRLEN(buffer);
	get_rel_pos(wp, buffer + i + 1);
	o = STRLEN(buffer + i + 1);
#ifdef FEAT_WINDOWS
	if (wp->w_status_height == 0)	/* can't use last char of screen */
#endif
	    ++o;
#ifdef FEAT_VERTSPLIT
	this_ru_col = ru_col - (Columns - width);
	if (this_ru_col < 0)
	    this_ru_col = 0;
#endif
	/* Never use more than half the window/screen width, leave the other
	 * half for the filename. */
	if (this_ru_col < (WITH_WIDTH(width) + 1) / 2)
	    this_ru_col = (WITH_WIDTH(width) + 1) / 2;
	if (this_ru_col + i + o < WITH_WIDTH(width))
	{
	    while (this_ru_col + i + o < WITH_WIDTH(width))
		buffer[i++] = fillchar;
	    get_rel_pos(wp, buffer + i);
	}
	/* Truncate at window boundary. */
	if (this_ru_col + (int)STRLEN(buffer) > WITH_WIDTH(width))
	    buffer[WITH_WIDTH(width) - this_ru_col] = NUL;

	screen_puts(buffer, row, this_ru_col + WITH_OFF(off), attr);
	i = redraw_cmdline;
	screen_fill(row, row + 1,
		this_ru_col + WITH_OFF(off) + (int)STRLEN(buffer),
		(int)(WITH_OFF(off) + WITH_WIDTH(width)),
		fillchar, fillchar, attr);
	/* don't redraw the cmdline because of showing the ruler */
	redraw_cmdline = i;
	wp->w_ru_cursor = wp->w_cursor;
	wp->w_ru_virtcol = wp->w_virtcol;
	wp->w_ru_empty = empty_line;
	wp->w_ru_topline = wp->w_topline;
#ifdef FEAT_DIFF
	wp->w_ru_topfill = wp->w_topfill;
#endif
    }
}
#endif
