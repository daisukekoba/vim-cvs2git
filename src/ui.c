/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * ui.c: functions that handle the user interface.
 * 1. Keyboard input stuff, and a bit of windowing stuff.  These are called
 *    before the machine specific stuff (mch_*) so that we can call the GUI
 *    stuff instead if the GUI is running.
 * 2. Clipboard stuff.
 * 3. Input buffer stuff.
 */

#include "vim.h"

    void
ui_write(s, len)
    char_u  *s;
    int	    len;
{
#ifdef FEAT_GUI
    if (gui.in_use && !gui.dying)
    {
	gui_write(s, len);
	if (p_wd)
	    gui_wait_for_chars(p_wd);
	return;
    }
#endif
#ifndef NO_CONSOLE
    /* Don't output anything in silent mode ("ex -s") */
    if (!silent_mode)
    {
#ifdef FEAT_MBYTE
	char_u	*tofree = NULL;

	if (output_conv.vc_type != CONV_NONE)
	{
	    /* Convert characters from 'encoding' to 'termencoding'. */
	    tofree = string_convert(&output_conv, s, &len);
	    if (tofree != NULL)
		s = tofree;
	}
#endif

	mch_write(s, len);

#ifdef FEAT_MBYTE
	if (output_conv.vc_type != CONV_NONE)
	    vim_free(tofree);
#endif
    }
#endif
}

#if (defined(FEAT_GUI) && defined(UNIX)) || defined(PROTO)
/*
 * When executing an external program, there may be some typed characters that
 * are not consumed by it.  Give them back to ui_inchar() and they are stored
 * here for the next call.
 */
static char_u *ta_str = NULL;
static int ta_off;	/* offset for next char to use when ta_str != NULL */
static int ta_len;	/* length of ta_str when it's not NULL*/

    void
ui_inchar_undo(s, len)
    char_u	*s;
    int		len;
{
    char_u  *new;
    int	    newlen;

    newlen = len;
    if (ta_str != NULL)
	newlen += ta_len - ta_off;
    new = alloc(newlen);
    if (new != NULL)
    {
	if (ta_str != NULL)
	{
	    mch_memmove(new, ta_str + ta_off, (size_t)(ta_len - ta_off));
	    mch_memmove(new + ta_len - ta_off, s, (size_t)len);
	    vim_free(ta_str);
	}
	else
	    mch_memmove(new, s, (size_t)len);
	ta_str = new;
	ta_len = newlen;
	ta_off = 0;
    }
}
#endif

/*
 * ui_inchar(): low level input funcion.
 * Get characters from the keyboard.
 * Return the number of characters that are available.
 * If "wtime" == 0 do not wait for characters.
 * If "wtime" == -1 wait forever for characters.
 * If "wtime" > 0 wait "wtime" milliseconds for a character.
 */
    int
ui_inchar(buf, maxlen, wtime)
    char_u	*buf;
    int		maxlen;
    long	wtime;	    /* don't use "time", MIPS cannot handle it */
{
    int		retval = 0;

#if defined(FEAT_GUI) && defined(UNIX)
    /*
     * Use the typeahead if there is any.
     */
    if (ta_str != NULL)
    {
	if (maxlen >= ta_len - ta_off)
	{
	    mch_memmove(buf, ta_str + ta_off, (size_t)ta_len);
	    vim_free(ta_str);
	    ta_str = NULL;
	    return ta_len;
	}
	mch_memmove(buf, ta_str + ta_off, (size_t)maxlen);
	ta_off += maxlen;
	return maxlen;
    }
#endif

#ifdef NO_CONSOLE_INPUT
    /* Don't wait for character input when the window hasn't been opened yet.
     * Must return something, otherwise we'll loop forever.  */
    if (no_console_input())
    {
	buf[0] = CR;
	return 1;
    }
#endif

    /* When doing a blocking wait there is no need for CTRL-C to interrupt
     * something, don't let it set got_int when it was mapped. */
    if (mapped_ctrl_c && (wtime == -1 || wtime > 100L))
	ctrl_c_interrupts = FALSE;

#ifdef FEAT_GUI
    if (gui.in_use)
    {
	if (gui_wait_for_chars(wtime))
	    retval = read_from_input_buf(buf, (long)maxlen);
    }
#endif
#ifndef NO_CONSOLE
# ifdef FEAT_GUI
    else
# endif
	retval = mch_inchar(buf, maxlen, wtime);
#endif

    ctrl_c_interrupts = TRUE;

    return retval;
}

/*
 * return non-zero if a character is available
 */
    int
ui_char_avail()
{
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	gui_mch_update();
	return !vim_is_input_buf_empty();
    }
#endif
#ifndef NO_CONSOLE
# ifdef NO_CONSOLE_INPUT
    if (no_console_input())
	return 0;
# endif
    return mch_char_avail();
#else
    return 0;
#endif
}

/*
 * Delay for the given number of milliseconds.	If ignoreinput is FALSE then we
 * cancel the delay if a key is hit.
 */
    void
ui_delay(msec, ignoreinput)
    long	msec;
    int		ignoreinput;
{
#ifdef FEAT_GUI
    if (gui.in_use && !ignoreinput)
	gui_wait_for_chars(msec);
    else
#endif
	mch_delay(msec, ignoreinput);
}

/*
 * If the machine has job control, use it to suspend the program,
 * otherwise fake it by starting a new shell.
 * When running the GUI iconify the window.
 */
    void
ui_suspend()
{
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	gui_mch_iconify();
	return;
    }
#endif
    mch_suspend();
}

#if !defined(UNIX) || !defined(SIGTSTP) || defined(PROTO) || defined(__BEOS__)
/*
 * When the OS can't really suspend, call this function to start a shell.
 * This is never called in the GUI.
 */
    void
suspend_shell()
{
    if (*p_sh == NUL)
	EMSG(_(e_shellempty));
    else
    {
	MSG_PUTS(_("new shell started\n"));
	do_shell(NULL, 0);
    }
}
#endif

/*
 * Try to get the current Vim shell size.  Put the result in Rows and Columns.
 * Use the new sizes as defaults for 'columns' and 'lines'.
 * Return OK when size could be determined, FAIL otherwise.
 */
    int
ui_get_shellsize()
{
    int	    retval;

#ifdef FEAT_GUI
    if (gui.in_use)
	retval = gui_get_shellsize();
    else
#endif
	retval = mch_get_shellsize();

    check_shellsize();

    /* adjust the default for 'lines' and 'columns' */
    if (retval == OK)
    {
	set_number_default("lines", Rows);
	set_number_default("columns", Columns);
    }
    return retval;
}

/*
 * Set the size of the Vim shell according to Rows and Columns, if possible.
 * The gui_set_shellsize() or mch_set_shellsize() function will try to set the
 * new size.  If this is not possible, it will adjust Rows and Columns.
 */
    void
ui_set_shellsize()
{
#ifdef FEAT_GUI
    if (gui.in_use)
	gui_set_shellsize(
# ifdef WIN32
		TRUE
# else
		FALSE
# endif
		);
    else
#endif
	mch_set_shellsize();
}

/*
 * Called when Rows and/or Columns changed.  Adjust scroll region and mouse
 * region.
 */
    void
ui_new_shellsize()
{
    if (full_screen && !exiting)
    {
#ifdef FEAT_GUI
	if (gui.in_use)
	    gui_new_shellsize();
	else
#endif
	    mch_new_shellsize();
    }
}

    void
ui_breakcheck()
{
#ifdef FEAT_GUI
    if (gui.in_use)
	gui_mch_update();
    else
#endif
	mch_breakcheck();
}

/*****************************************************************************
 * Functions for copying and pasting text between applications.
 * This is always included in a GUI version, but may also be included when the
 * clipboard and mouse is available to a terminal version such as xterm.
 * Note: there are some more functions in ops.c that handle selection stuff.
 *
 * Also note that the majority of functions here deal with the X 'primary'
 * (visible - for Visual mode use) selection, and only that. There are no
 * versions of these for the 'clipboard' selection, as Visual mode has no use
 * for them.
 */

#if defined(FEAT_CLIPBOARD) || defined(PROTO)

/*
 * Selection stuff using Visual mode, for cutting and pasting text to other
 * windows.
 */

/*
 * Call this to initialise the clipboard.  Pass it FALSE if the clipboard code
 * is included, but the clipboard can not be used, or TRUE if the clipboard can
 * be used.  Eg unix may call this with FALSE, then call it again with TRUE if
 * the GUI starts.
 */
    void
clip_init(can_use)
    int	    can_use;
{
    VimClipboard *cb;

    cb = &clip_star;
    for (;;)
    {
	cb->available  = can_use;
	cb->owned      = FALSE;
	cb->start.lnum = 0;
	cb->start.col  = 0;
	cb->end.lnum   = 0;
	cb->end.col    = 0;
	cb->state      = SELECT_CLEARED;

	if (cb == &clip_plus)
	    break;
	cb = &clip_plus;
    }
}

/*
 * Check whether the VIsual area has changed, and if so try to become the owner
 * of the selection, and free any old converted selection we may still have
 * lying around.  If the VIsual mode has ended, make a copy of what was
 * selected so we can still give it to others.	Will probably have to make sure
 * this is called whenever VIsual mode is ended.
 */
    void
clip_update_selection()
{
    pos_T    start, end;

    /* If visual mode is only due to a redo command ("."), then ignore it */
    if (!redo_VIsual_busy && VIsual_active)
    {
	if (lt(VIsual, curwin->w_cursor))
	{
	    start = VIsual;
	    end = curwin->w_cursor;
#ifdef FEAT_MBYTE
	    if (has_mbyte)
		end.col += (*mb_ptr2len_check)(ml_get_cursor()) - 1;
#endif
	}
	else
	{
	    start = curwin->w_cursor;
	    end = VIsual;
	}
	if (!equal(clip_star.start, start) || !equal(clip_star.end, end)
					    || clip_star.vmode != VIsual_mode)
	{
	    clip_clear_selection();
	    clip_star.start = start;
	    clip_star.end = end;
	    clip_star.vmode = VIsual_mode;
	    clip_free_selection(&clip_star);
	    clip_own_selection(&clip_star);
	    clip_gen_set_selection(&clip_star);
	}
    }
}

    void
clip_own_selection(cbd)
    VimClipboard	*cbd;
{
    /*
     * Also want to check somehow that we are reading from the keyboard rather
     * than a mapping etc.
     */
    if (!cbd->owned && cbd->available)
    {
        cbd->owned = (clip_gen_own_selection(cbd) == OK);
#ifdef FEAT_X11
        if (cbd == &clip_star)
        {
            /* May have to show a different kind of highlighting for the selected
             * area.  There is no specific redraw command for this, just redraw
             * all windows on the current buffer. */
            if (cbd->owned
                    && get_real_state() == VISUAL
                    && clip_isautosel()
                    && hl_attr(HLF_V) != hl_attr(HLF_VNC))
                redraw_curbuf_later(INVERTED_ALL);
        }
#endif
    }
}

    void
clip_lose_selection(cbd)
    VimClipboard	*cbd;
{
#ifdef FEAT_X11
    int	    was_owned = cbd->owned;
#endif
    int     visual_selection = (cbd == &clip_star);

    clip_free_selection(cbd);
    cbd->owned = FALSE;
    if (visual_selection)
        clip_clear_selection();
    clip_gen_lose_selection(cbd);
#ifdef FEAT_X11
    if (visual_selection)
    {
        /* May have to show a different kind of highlighting for the selected
         * area.  There is no specific redraw command for this, just redraw all
         * windows on the current buffer. */
        if (was_owned
                && get_real_state() == VISUAL
                && clip_isautosel()
                && hl_attr(HLF_V) != hl_attr(HLF_VNC))
        {
            update_curbuf(INVERTED_ALL);
            setcursor();
            cursor_on();
            out_flush();
        }
    }
#endif
}

    void
clip_copy_selection()
{
    if (VIsual_active && clip_star.available)
    {
	if (clip_isautosel())
	    clip_update_selection();
	clip_free_selection(&clip_star);
	clip_own_selection(&clip_star);
	if (clip_star.owned)
	    clip_get_selection(&clip_star);
	clip_gen_set_selection(&clip_star);
    }
}

/*
 * Called when Visual mode is ended: update the selection.
 */
    void
clip_auto_select()
{
    if (clip_isautosel())
	clip_copy_selection();
}

/*
 * Return TRUE if automatic selection of Visual area is desired.
 */
    int
clip_isautosel()
{
    return (
#ifdef FEAT_GUI
	    gui.in_use ? (vim_strchr(p_go, GO_ASEL) != NULL) :
#endif
	    clip_autoselect);
}


/*
 * Stuff for general mouse selection, without using Visual mode.
 */

static int clip_compare_pos __ARGS((int row1, int col1, int row2, int col2));
static void clip_invert_area __ARGS((int, int, int, int, int how));
static void clip_invert_rectangle __ARGS((int row, int col, int height, int width, int invert));
static void clip_get_word_boundaries __ARGS((VimClipboard *, int, int));
static int  clip_get_line_end __ARGS((int));
static void clip_update_modeless_selection __ARGS((VimClipboard *, int, int,
						    int, int));

/* flags for clip_invert_area() */
#define CLIP_CLEAR	1
#define CLIP_SET	2
#define CLIP_TOGGLE	3

/*
 * Start, continue or end a modeless selection.  Used when editing the
 * command-line and in the cmdline window.
 */
    void
clip_modeless(button, is_click, is_drag)
    int		button;
    int		is_click;
    int		is_drag;
{
    int		repeat;

    repeat = ((clip_star.mode == SELECT_MODE_CHAR
		|| clip_star.mode == SELECT_MODE_LINE)
					      && (mod_mask & MOD_MASK_2CLICK))
	    || (clip_star.mode == SELECT_MODE_WORD
					     && (mod_mask & MOD_MASK_3CLICK));
    if (is_click && button == MOUSE_RIGHT)
    {
	/* Right mouse button: If there was no selection, start one.
	 * Otherwise extend the existing selection. */
	if (clip_star.state == SELECT_CLEARED)
	    clip_start_selection(mouse_col, mouse_row, FALSE);
	clip_process_selection(button, mouse_col, mouse_row, repeat);
    }
    else if (is_click)
	clip_start_selection(mouse_col, mouse_row, repeat);
    else if (is_drag)
	clip_process_selection(button, mouse_col, mouse_row, repeat);
    else /* release */
	clip_process_selection(MOUSE_RELEASE, mouse_col, mouse_row, FALSE);
}

/*
 * Compare two screen positions ala strcmp()
 */
    static int
clip_compare_pos(row1, col1, row2, col2)
    int		row1;
    int		col1;
    int		row2;
    int		col2;
{
    if (row1 > row2) return(1);
    if (row1 < row2) return(-1);
    if (col1 > col2) return(1);
    if (col1 < col2) return(-1);
		     return(0);
}

/*
 * Start the selection
 */
    void
clip_start_selection(col, row, repeated_click)
    int		col;
    int		row;
    int		repeated_click;
{
    VimClipboard	*cb = &clip_star;

    if (cb->state == SELECT_DONE)
	clip_clear_selection();

    cb->start.lnum  = check_row(row);
    cb->start.col   = check_col(col);
    cb->end	    = cb->start;
    cb->origin_row  = (short_u)cb->start.lnum;
    cb->state	    = SELECT_IN_PROGRESS;

    if (repeated_click)
    {
	if (++cb->mode > SELECT_MODE_LINE)
	    cb->mode = SELECT_MODE_CHAR;
    }
    else
	cb->mode = SELECT_MODE_CHAR;

#ifdef FEAT_GUI
    /* clear the cursor until the selection is made */
    if (gui.in_use)
	gui_undraw_cursor();
#endif

    switch (cb->mode)
    {
	case SELECT_MODE_CHAR:
	    cb->origin_start_col = cb->start.col;
	    cb->word_end_col = clip_get_line_end((int)cb->start.lnum);
	    break;

	case SELECT_MODE_WORD:
	    clip_get_word_boundaries(cb, (int)cb->start.lnum, cb->start.col);
	    cb->origin_start_col = cb->word_start_col;
	    cb->origin_end_col	 = cb->word_end_col;

	    clip_invert_area((int)cb->start.lnum, cb->word_start_col,
			    (int)cb->end.lnum, cb->word_end_col, CLIP_SET);
	    cb->start.col = cb->word_start_col;
	    cb->end.col   = cb->word_end_col;
	    break;

	case SELECT_MODE_LINE:
	    clip_invert_area((int)cb->start.lnum, 0, (int)cb->start.lnum,
			    (int)Columns, CLIP_SET);
	    cb->start.col = 0;
	    cb->end.col   = Columns;
	    break;
    }

    cb->prev = cb->start;

#ifdef DEBUG_SELECTION
    printf("Selection started at (%u,%u)\n", cb->start.lnum, cb->start.col);
#endif
}

/*
 * Continue processing the selection
 */
    void
clip_process_selection(button, col, row, repeated_click)
    int		button;
    int		col;
    int		row;
    int_u	repeated_click;
{
    VimClipboard	*cb = &clip_star;
    int			diff;

    if (button == MOUSE_RELEASE)
    {
	/* Check to make sure we have something selected */
	if (cb->start.lnum == cb->end.lnum && cb->start.col == cb->end.col)
	{
#ifdef FEAT_GUI
	    if (gui.in_use)
		gui_update_cursor(FALSE, FALSE);
#endif
	    cb->state = SELECT_CLEARED;
	    return;
	}

#ifdef DEBUG_SELECTION
	printf("Selection ended: (%u,%u) to (%u,%u)\n", cb->start.lnum,
		cb->start.col, cb->end.lnum, cb->end.col);
#endif
	if (clip_isautosel())
	    clip_copy_modeless_selection(FALSE);
#ifdef FEAT_GUI
	if (gui.in_use)
	    gui_update_cursor(FALSE, FALSE);
#endif

	cb->state = SELECT_DONE;
	return;
    }

    row = check_row(row);
    col = check_col(col);

    if (col == (int)cb->prev.col && row == cb->prev.lnum && !repeated_click)
	return;

    /*
     * When extending the selection with the right mouse button, swap the
     * start and end if the position is before half the selection
     */
    if (cb->state == SELECT_DONE && button == MOUSE_RIGHT)
    {
	/*
	 * If the click is before the start, or the click is inside the
	 * selection and the start is the closest side, set the origin to the
	 * end of the selection.
	 */
	if (clip_compare_pos(row, col, (int)cb->start.lnum, cb->start.col) < 0
		|| (clip_compare_pos(row, col,
					   (int)cb->end.lnum, cb->end.col) < 0
		    && (((cb->start.lnum == cb->end.lnum
			    && cb->end.col - col > col - cb->start.col))
			|| ((diff = (cb->end.lnum - row) -
						   (row - cb->start.lnum)) > 0
			    || (diff == 0 && col < (int)(cb->start.col +
							 cb->end.col) / 2)))))
	{
	    cb->origin_row = (short_u)cb->end.lnum;
	    cb->origin_start_col = cb->end.col - 1;
	    cb->origin_end_col = cb->end.col;
	}
	else
	{
	    cb->origin_row = (short_u)cb->start.lnum;
	    cb->origin_start_col = cb->start.col;
	    cb->origin_end_col = cb->start.col;
	}
	if (cb->mode == SELECT_MODE_WORD && !repeated_click)
	    cb->mode = SELECT_MODE_CHAR;
    }

    /* set state, for when using the right mouse button */
    cb->state = SELECT_IN_PROGRESS;

#ifdef DEBUG_SELECTION
    printf("Selection extending to (%d,%d)\n", row, col);
#endif

    if (repeated_click && ++cb->mode > SELECT_MODE_LINE)
	cb->mode = SELECT_MODE_CHAR;

    switch (cb->mode)
    {
	case SELECT_MODE_CHAR:
	    /* If we're on a different line, find where the line ends */
	    if (row != cb->prev.lnum)
		cb->word_end_col = clip_get_line_end(row);

	    /* See if we are before or after the origin of the selection */
	    if (clip_compare_pos(row, col, cb->origin_row,
						   cb->origin_start_col) >= 0)
	    {
		if (col >= (int)cb->word_end_col)
		    clip_update_modeless_selection(cb, cb->origin_row,
			    cb->origin_start_col, row, (int)Columns);
		else
		    clip_update_modeless_selection(cb, cb->origin_row,
			    cb->origin_start_col, row, col + 1);
	    }
	    else
	    {
		if (col >= (int)cb->word_end_col)
		    clip_update_modeless_selection(cb, row, cb->word_end_col,
			    cb->origin_row, cb->origin_start_col + 1);
		else
		    clip_update_modeless_selection(cb, row, col,
			    cb->origin_row, cb->origin_start_col + 1);
	    }
	    break;

	case SELECT_MODE_WORD:
	    /* If we are still within the same word, do nothing */
	    if (row == cb->prev.lnum && col >= (int)cb->word_start_col
		    && col < (int)cb->word_end_col && !repeated_click)
		return;

	    /* Get new word boundaries */
	    clip_get_word_boundaries(cb, row, col);

	    /* Handle being after the origin point of selection */
	    if (clip_compare_pos(row, col, cb->origin_row,
		    cb->origin_start_col) >= 0)
		clip_update_modeless_selection(cb, cb->origin_row,
			cb->origin_start_col, row, cb->word_end_col);
	    else
		clip_update_modeless_selection(cb, row, cb->word_start_col,
			cb->origin_row, cb->origin_end_col);
	    break;

	case SELECT_MODE_LINE:
	    if (row == cb->prev.lnum && !repeated_click)
		return;

	    if (clip_compare_pos(row, col, cb->origin_row,
		    cb->origin_start_col) >= 0)
		clip_update_modeless_selection(cb, cb->origin_row, 0, row,
			(int)Columns);
	    else
		clip_update_modeless_selection(cb, row, 0, cb->origin_row,
			(int)Columns);
	    break;
    }

    cb->prev.lnum = row;
    cb->prev.col  = col;

#ifdef DEBUG_SELECTION
	printf("Selection is: (%u,%u) to (%u,%u)\n", cb->start.lnum,
		cb->start.col, cb->end.lnum, cb->end.col);
#endif
}

#if 0 /* not used */
/*
 * Called after an Expose event to redraw the selection
 */
    void
clip_redraw_selection(x, y, w, h)
    int	    x;
    int	    y;
    int	    w;
    int	    h;
{
    VimClipboard    *cb = &clip_star;
    int		    row1, col1, row2, col2;
    int		    row;
    int		    start;
    int		    end;

    if (cb->state == SELECT_CLEARED)
	return;

    row1 = check_row(Y_2_ROW(y));
    col1 = check_col(X_2_COL(x));
    row2 = check_row(Y_2_ROW(y + h - 1));
    col2 = check_col(X_2_COL(x + w - 1));

    /* Limit the rows that need to be re-drawn */
    if (cb->start.lnum > row1)
	row1 = cb->start.lnum;
    if (cb->end.lnum < row2)
	row2 = cb->end.lnum;

    /* Look at each row that might need to be re-drawn */
    for (row = row1; row <= row2; row++)
    {
	/* For the first selection row, use the starting selection column */
	if (row == cb->start.lnum)
	    start = cb->start.col;
	else
	    start = 0;

	/* For the last selection row, use the ending selection column */
	if (row == cb->end.lnum)
	    end = cb->end.col;
	else
	    end = Columns;

	if (col1 > start)
	    start = col1;

	if (col2 < end)
	    end = col2 + 1;

	if (end > start)
	    gui_mch_invert_rectangle(row, start, 1, end - start);
    }
}
#endif

# if defined(FEAT_GUI) || defined(PROTO)
/*
 * Redraw part of the selection if character at "row,col" is inside of it.
 * Only used for the GUI.
 */
    void
clip_may_redraw_selection(row, col, len)
    int		row, col;
    int		len;
{
    int		start = col;
    int		end = col + len;

    if (clip_star.state != SELECT_CLEARED
	    && row >= clip_star.start.lnum
	    && row <= clip_star.end.lnum)
    {
	if (row == clip_star.start.lnum && start < (int)clip_star.start.col)
	    start = clip_star.start.col;
	if (row == clip_star.end.lnum && end > (int)clip_star.end.col)
	    end = clip_star.end.col;
	if (end > start)
	    clip_invert_area(row, start, row, end, 0);
    }
}
# endif

/*
 * Called from outside to clear selected region from the display
 */
    void
clip_clear_selection()
{
    VimClipboard    *cb = &clip_star;

    if (cb->state == SELECT_CLEARED)
	return;

    clip_invert_area((int)cb->start.lnum, cb->start.col, (int)cb->end.lnum,
						     cb->end.col, CLIP_CLEAR);
    cb->state = SELECT_CLEARED;
}

/*
 * Clear the selection if any lines from "row1" to "row2" are inside of it.
 */
    void
clip_may_clear_selection(row1, row2)
    int	row1, row2;
{
    if (clip_star.state == SELECT_DONE
	    && row2 >= clip_star.start.lnum
	    && row1 <= clip_star.end.lnum)
	clip_clear_selection();
}

/*
 * Called before the screen is scrolled up or down.  Adjusts the line numbers
 * of the selection.  Call with big number when clearing the screen.
 */
    void
clip_scroll_selection(rows)
    int	    rows;		/* negative for scroll down */
{
    int	    lnum;

    if (clip_star.state == SELECT_CLEARED)
	return;

    lnum = clip_star.start.lnum - rows;
    if (lnum <= 0)
	clip_star.start.lnum = 0;
    else if (lnum >= screen_Rows)	/* scrolled off of the screen */
	clip_star.state = SELECT_CLEARED;
    else
	clip_star.start.lnum = lnum;

    lnum = clip_star.end.lnum - rows;
    if (lnum < 0)			/* scrolled off of the screen */
	clip_star.state = SELECT_CLEARED;
    else if (lnum >= screen_Rows)
	clip_star.end.lnum = screen_Rows - 1;
    else
	clip_star.end.lnum = lnum;
}

/*
 * Invert a region of the display between a starting and ending row and column
 * Values for "how":
 * CLIP_CLEAR:  undo inversion
 * CLIP_SET:    set inversion
 * CLIP_TOGGLE: set inversion if pos1 < pos2, undo inversion otherwise.
 * 0: invert (GUI only).
 */
    static void
clip_invert_area(row1, col1, row2, col2, how)
    int		row1;
    int		col1;
    int		row2;
    int		col2;
    int		how;
{
    int		invert = FALSE;

    if (how == CLIP_SET)
	invert = TRUE;

    /* Swap the from and to positions so the from is always before */
    if (clip_compare_pos(row1, col1, row2, col2) > 0)
    {
	int tmp_row, tmp_col;

	tmp_row = row1;
	tmp_col = col1;
	row1	= row2;
	col1	= col2;
	row2	= tmp_row;
	col2	= tmp_col;
    }
    else if (how == CLIP_TOGGLE)
	invert = TRUE;

    /* If all on the same line, do it the easy way */
    if (row1 == row2)
    {
	clip_invert_rectangle(row1, col1, 1, col2 - col1, invert);
    }
    else
    {
	/* Handle a piece of the first line */
	if (col1 > 0)
	{
	    clip_invert_rectangle(row1, col1, 1, (int)Columns - col1, invert);
	    row1++;
	}

	/* Handle a piece of the last line */
	if (col2 < Columns - 1)
	{
	    clip_invert_rectangle(row2, 0, 1, col2, invert);
	    row2--;
	}

	/* Handle the rectangle thats left */
	if (row2 >= row1)
	    clip_invert_rectangle(row1, 0, row2 - row1 + 1, (int)Columns,
								      invert);
    }
}

/*
 * Invert or un-invert a rectangle of the screen.
 * "invert" is true if the result is inverted.
 */
    static void
clip_invert_rectangle(row, col, height, width, invert)
    int		row;
    int		col;
    int		height;
    int		width;
    int		invert;
{
#ifdef FEAT_GUI
    if (gui.in_use)
	gui_mch_invert_rectangle(row, col, height, width);
    else
#endif
	screen_draw_rectangle(row, col, height, width, invert);
}

/*
 * Copy the currently selected area into the '*' register so it will be
 * available for pasting.
 * When "both" is TRUE also copy to the '+' register.
 */
/*ARGSUSED*/
    void
clip_copy_modeless_selection(both)
    int		both;
{
    char_u	*buffer;
    char_u	*bufp;
    int		row;
    int		start_col;
    int		end_col;
    int		line_end_col;
    int		add_newline_flag = FALSE;
    int		len;
#ifdef FEAT_MBYTE
    char_u	*p;
    int		i;
#endif
    int		row1 = clip_star.start.lnum;
    int		col1 = clip_star.start.col;
    int		row2 = clip_star.end.lnum;
    int		col2 = clip_star.end.col;

    /*
     * Make sure row1 <= row2, and if row1 == row2 that col1 <= col2.
     */
    if (row1 > row2)
    {
	row = row1; row1 = row2; row2 = row;
	row = col1; col1 = col2; col2 = row;
    }
    else if (row1 == row2 && col1 > col2)
    {
	row = col1; col1 = col2; col2 = row;
    }
#ifdef FEAT_MBYTE
    /* correct starting point for being on right halve of double-wide char */
    p = ScreenLines + LineOffset[row1];
    if (enc_dbcs != 0)
	col1 -= (*mb_head_off)(p, p + col1);
    else if (enc_utf8 && p[col1] == 0)
	--col1;
#endif

    /* Create a temporary buffer for storing the text */
    len = (row2 - row1 + 1) * Columns + 1, TRUE;
#ifdef FEAT_MBYTE
    if (enc_dbcs != 0)
	len *= 2;	/* max. 2 bytes per display cell */
    else if (enc_utf8)
	len *= 9;	/* max. 3 bytes per display cell + 2 composing chars */
#endif
    buffer = lalloc((long_u)len, TRUE);
    if (buffer == NULL)	    /* out of memory */
	return;

    /* Process each row in the selection */
    for (bufp = buffer, row = row1; row <= row2; row++)
    {
	if (row == row1)
	    start_col = col1;
	else
	    start_col = 0;

	if (row == row2)
	    end_col = col2;
	else
	    end_col = Columns;

	line_end_col = clip_get_line_end(row);

	/* See if we need to nuke some trailing whitespace */
	if (end_col >= Columns && (row < row2 || end_col > line_end_col))
	{
	    /* Get rid of trailing whitespace */
	    end_col = line_end_col;
	    if (end_col < start_col)
		end_col = start_col;

	    /* If the last line extended to the end, add an extra newline */
	    if (row == row2)
		add_newline_flag = TRUE;
	}

	/* If after the first row, we need to always add a newline */
	if (row > row1)
	    *bufp++ = NL;

	if (row < screen_Rows && end_col <= screen_Columns)
	{
#ifdef FEAT_MBYTE
	    if (enc_dbcs != 0)
	    {
		p = ScreenLines + LineOffset[row];
		for (i = start_col; i < end_col; ++i)
		    if (enc_dbcs == DBCS_JPNU && p[i] == 0x8e)
		    {
			/* single-width double-byte char */
			*bufp++ = 0x8e;
			*bufp++ = ScreenLines2[LineOffset[row] + i];
		    }
		    else
		    {
			*bufp++ = p[i];
			if (MB_BYTE2LEN(p[i]) == 2)
			    *bufp++ = p[++i];
		    }
	    }
	    else if (enc_utf8)
	    {
		int	off;

		off = LineOffset[row];
		for (i = start_col; i < end_col; ++i)
		{
		    /* The base character is either in ScreenLinesUC[] or
		     * ScreenLines[]. */
		    if (ScreenLinesUC[off + i] == 0)
			*bufp++ = ScreenLines[off + i];
		    else
		    {
			bufp += utf_char2bytes(ScreenLinesUC[off + i], bufp);
			if (ScreenLinesC1[off + i] != 0)
			{
			    /* Add one or two composing characters. */
			    bufp += utf_char2bytes(ScreenLinesC1[off + i],
									bufp);
			    if (ScreenLinesC2[off + i] != 0)
				bufp += utf_char2bytes(ScreenLinesC2[off + i],
					bufp);
			}
		    }
		    /* Skip right halve of double-wide character. */
		    if (ScreenLines[off + i + 1] == 0)
			++i;
		}
	    }
	    else
#endif
	    {
		STRNCPY(bufp, ScreenLines + LineOffset[row] + start_col,
							 end_col - start_col);
		bufp += end_col - start_col;
	    }
	}
    }

    /* Add a newline at the end if the selection ended there */
    if (add_newline_flag)
	*bufp++ = NL;

    /* First cleanup any old selection and become the owner. */
    clip_free_selection(&clip_star);
    clip_own_selection(&clip_star);

    /* Yank the text into the '*' register. */
    clip_yank_selection(MCHAR, buffer, (long)(bufp - buffer), &clip_star);

    /* Make the register contents available to the outside world. */
    clip_gen_set_selection(&clip_star);

#ifdef FEAT_X11
    if (both)
    {
	/* Do the same for the '+' register. */
	clip_free_selection(&clip_plus);
	clip_own_selection(&clip_plus);
	clip_yank_selection(MCHAR, buffer, (long)(bufp - buffer), &clip_plus);
	clip_gen_set_selection(&clip_plus);
    }
#endif
    vim_free(buffer);
}

/*
 * Find the starting and ending positions of the word at the given row and
 * column.  Only white-separated words are recognized here.
 */
#define CHAR_CLASS(c)	(c <= ' ' ? ' ' : vim_iswordc(c))

    static void
clip_get_word_boundaries(cb, row, col)
    VimClipboard	*cb;
    int			row;
    int			col;
{
    int		start_class;
    int		temp_col;
    char_u	*p;

    if (row >= screen_Rows || col >= screen_Columns)
	return;

    p = ScreenLines + LineOffset[row];
#ifdef FEAT_MBYTE
    /* With UTF-8 there are zero bytes in the right halve of a double-wide
     * character. */
    if (enc_utf8 && p[col] == 0)
	--col;
#endif
    start_class = CHAR_CLASS(p[col]);

    temp_col = col;
    for ( ; temp_col > 0; temp_col--)
	if (CHAR_CLASS(p[temp_col - 1]) != start_class
#ifdef FEAT_MBYTE
		&& !(enc_utf8 && p[temp_col - 1] == 0)
#endif
		)
	    break;
    cb->word_start_col = temp_col;

    temp_col = col;
    for ( ; temp_col < screen_Columns; temp_col++)
	if (CHAR_CLASS(p[temp_col]) != start_class
#ifdef FEAT_MBYTE
		&& !(enc_utf8 && p[temp_col] == 0)
#endif
		)
	    break;
    cb->word_end_col = temp_col;
}

/*
 * Find the column position for the last non-whitespace character on the given
 * line.
 */
    static int
clip_get_line_end(row)
    int		row;
{
    int	    i;

    if (row >= screen_Rows)
	return 0;
    for (i = screen_Columns; i > 0; i--)
	if (ScreenLines[LineOffset[row] + i - 1] != ' ')
	    break;
    return i;
}

/*
 * Update the currently selected region by adding and/or subtracting from the
 * beginning or end and inverting the changed area(s).
 */
    static void
clip_update_modeless_selection(cb, row1, col1, row2, col2)
    VimClipboard    *cb;
    int		    row1;
    int		    col1;
    int		    row2;
    int		    col2;
{
    /* See if we changed at the beginning of the selection */
    if (row1 != cb->start.lnum || col1 != (int)cb->start.col)
    {
	clip_invert_area(row1, col1, (int)cb->start.lnum, cb->start.col,
								 CLIP_TOGGLE);
	cb->start.lnum = row1;
	cb->start.col  = col1;
    }

    /* See if we changed at the end of the selection */
    if (row2 != cb->end.lnum || col2 != (int)cb->end.col)
    {
	clip_invert_area((int)cb->end.lnum, cb->end.col, row2, col2,
								 CLIP_TOGGLE);
	cb->end.lnum = row2;
	cb->end.col  = col2;
    }
}

    int
clip_gen_own_selection(cbd)
    VimClipboard	*cbd;
{
#ifdef FEAT_XCLIPBOARD
# ifdef FEAT_GUI
    if (gui.in_use)
	return clip_mch_own_selection(cbd);
    else
# endif
	return clip_xterm_own_selection(cbd);
#else
    return clip_mch_own_selection(cbd);
#endif
}

    void
clip_gen_lose_selection(cbd)
    VimClipboard	*cbd;
{
#ifdef FEAT_XCLIPBOARD
# ifdef FEAT_GUI
    if (gui.in_use)
	clip_mch_lose_selection(cbd);
    else
# endif
	clip_xterm_lose_selection(cbd);
#else
    clip_mch_lose_selection(cbd);
#endif
}

    void
clip_gen_set_selection(cbd)
    VimClipboard	*cbd;
{
#ifdef FEAT_XCLIPBOARD
# ifdef FEAT_GUI
    if (gui.in_use)
	clip_mch_set_selection(cbd);
    else
# endif
	clip_xterm_set_selection(cbd);
#else
    clip_mch_set_selection(cbd);
#endif
}

    void
clip_gen_request_selection(cbd)
    VimClipboard	*cbd;
{
#ifdef FEAT_XCLIPBOARD
# ifdef FEAT_GUI
    if (gui.in_use)
	clip_mch_request_selection(cbd);
    else
# endif
	clip_xterm_request_selection(cbd);
#else
    clip_mch_request_selection(cbd);
#endif
}

#endif /* FEAT_CLIPBOARD */

/*****************************************************************************
 * Functions that handle the input buffer.
 * This is used for any GUI version, and the unix terminal version.
 *
 * For Unix, the input characters are buffered to be able to check for a
 * CTRL-C.  This should be done with signals, but I don't know how to do that
 * in a portable way for a tty in RAW mode.
 */

#if defined(UNIX) || defined(FEAT_GUI) || defined(OS2) || defined(VMS)

/*
 * Internal typeahead buffer.  Includes extra space for long key code
 * descriptions which would otherwise overflow.  The buffer is considered full
 * when only this extra space (or part of it) remains.
 */
#ifdef VMS
# define INBUFLEN 10000 /* for proper cut/paste between X windows in ch. mode */
#else
# ifdef FEAT_SUN_WORKSHOP
   /*
    * Sun WorkShop stuffs debugger commands into the input buffer. This requires
    * a larger buffer...
    */
#  define INBUFLEN 4096
# else
#  define INBUFLEN 250
# endif
#endif

static char_u	inbuf[INBUFLEN + MAX_KEY_CODE_LEN];
static int	inbufcount = 0;	    /* number of chars in inbuf[] */

/*
 * vim_is_input_buf_full(), vim_is_input_buf_empty(), add_to_input_buf(), and
 * trash_input_buf() are functions for manipulating the input buffer.  These
 * are used by the gui_* calls when a GUI is used to handle keyboard input.
 */

    int
vim_is_input_buf_full()
{
    return (inbufcount >= INBUFLEN);
}

    int
vim_is_input_buf_empty()
{
    return (inbufcount == 0);
}

#if defined(FEAT_OLE) || defined(PROTO)
    int
vim_free_in_input_buf()
{
    return (INBUFLEN - inbufcount);
}
#endif

#if defined(FEAT_GUI_GTK) || defined(PROTO)
    int
vim_used_in_input_buf()
{
    return inbufcount;
}
#endif

#if defined(FEAT_GUI) || defined(FEAT_SIGNS) || defined(FEAT_MOUSE_GPM) \
	|| defined(PROTO) || defined(FEAT_XCLIPBOARD) || defined(VMS) \
	|| defined(FEAT_SNIFF)
/*
 * Add the given bytes to the input buffer
 * Special keys start with CSI.  A real CSI must have been translated to
 * CSI KS_EXTRA KE_CSI.  K_SPECIAL doesn't require translation.
 */
    void
add_to_input_buf(s, len)
    char_u  *s;
    int	    len;
{
    if (inbufcount + len > INBUFLEN + MAX_KEY_CODE_LEN)
	return;	    /* Shouldn't ever happen! */

#ifdef FEAT_HANGULIN
    if ((State & (INSERT|CMDLINE)) && hangul_input_state_get())
	if ((len = hangul_input_process(s, len)) == 0)
	    return;
#endif

    while (len--)
	inbuf[inbufcount++] = *s++;
}
#endif

#if defined(FEAT_HANGULIN) || defined(PROTO)
    void
push_raw_key (s, len)
    char_u  *s;
    int	    len;
{
    while (len--)
	inbuf[inbufcount++] = *s++;
}
#endif

#if defined(FEAT_GUI) || defined(PROTO)
/* Remove everything from the input buffer.  Called when ^C is found */
    void
trash_input_buf()
{
    inbufcount = 0;
}
#endif

/*
 * Read as much data from the input buffer as possible up to maxlen, and store
 * it in buf.
 * Note: this function used to be Read() in unix.c
 */
    int
read_from_input_buf(buf, maxlen)
    char_u  *buf;
    long    maxlen;
{
    if (inbufcount == 0)	/* if the buffer is empty, fill it */
	fill_input_buf(TRUE);
    if (maxlen > inbufcount)
	maxlen = inbufcount;
    mch_memmove(buf, inbuf, (size_t)maxlen);
    inbufcount -= maxlen;
    if (inbufcount)
	mch_memmove(inbuf, inbuf + maxlen, (size_t)inbufcount);
    return (int)maxlen;
}

    void
fill_input_buf(exit_on_error)
    int	exit_on_error;
{
#if defined(UNIX) || defined(OS2) || defined(VMS)
    int		len;
    int		try;
    static int	did_read_something = FALSE;
#endif
#ifdef VMS
    extern char ibuf[];
#endif

#ifdef FEAT_GUI
    if (gui.in_use)
    {
	gui_mch_update();
	return;
    }
#endif
#if defined(UNIX) || defined(OS2) || defined(VMS)
    if (vim_is_input_buf_full())
	return;
    /*
     * Fill_input_buf() is only called when we really need a character.
     * If we can't get any, but there is some in the buffer, just return.
     * If we can't get any, and there isn't any in the buffer, we give up and
     * exit Vim.
     */
# ifdef __BEOS__
    /*
     * On the BeBox version (for now), all input is secretly performed within
     * beos_select() which is called from RealWaitForChar().
     */
    while (!vim_is_input_buf_full() && RealWaitForChar(read_cmd_fd, 0, NULL))
	    ;
    len = inbufcount;
    inbufcount = 0;
# else

#  ifdef FEAT_SNIFF
    if (sniff_request_waiting)
    {
	add_to_input_buf((char_u *)"\233sniff",6); /* results in K_SNIFF */
	sniff_request_waiting = 0;
	want_sniff_request = 0;
	return;
    }
#  endif
#  ifdef VMS
    while (!vim_is_input_buf_full() && RealWaitForChar(0, 0L))
    {
	add_to_input_buf((char_u *)ibuf, 1);
    }
    if (inbufcount < 1 && !exit_on_error)
	return;
    len = inbufcount;
    inbufcount = 0;
#  else

    len = 0;	/* to avoid gcc warning */
    for (try = 0; try < 100; ++try)
    {
	len = read(read_cmd_fd, (char *)inbuf + inbufcount,
		(size_t)((INBUFLEN - inbufcount)
#  ifdef FEAT_MBYTE
		/ input_conv.vc_factor
#  endif
		));
	if (len > 0 || got_int)
	    break;
	/*
	 * If reading stdin results in an error, continue reading stderr.
	 * This helps when using "foo | xargs vim".
	 */
	if (!did_read_something && !isatty(read_cmd_fd) && read_cmd_fd == 0)
	    read_cmd_fd = 2;
	if (!exit_on_error)
	    return;
    }
#  endif /* VMS */
# endif
    if (len <= 0 && !got_int)
	read_error_exit();
    did_read_something = TRUE;
    if (got_int)
    {
	inbuf[inbufcount] = 3;
	inbufcount = 1;
    }
    else
    {
# ifdef FEAT_MBYTE
	/* May perform conversion on the input characters. */
	if (input_conv.vc_type != CONV_NONE)
	    len = convert_input(inbuf + inbufcount, len, INBUFLEN - inbufcount);
# endif
	while (len-- > 0)
	{
	    /*
	     * if a CTRL-C was typed, remove it from the buffer and set got_int
	     */
	    if (inbuf[inbufcount] == 3 && ctrl_c_interrupts)
	    {
		/* remove everything typed before the CTRL-C */
		mch_memmove(inbuf, inbuf + inbufcount, (size_t)(len + 1));
		inbufcount = 0;
		got_int = TRUE;
	    }
	    ++inbufcount;
	}
    }
#endif /* UNIX or OS2 or VMS*/
}
#endif /* defined(UNIX) || defined(FEAT_GUI) || defined(OS2)  || defined(VMS) */

/*
 * Exit because of an input read error.
 */
    void
read_error_exit()
{
    if (silent_mode)	/* Normal way to exit for "ex -s" */
	getout(0);
    STRCPY(IObuff, _("Vim: Error reading input, exiting...\n"));
    preserve_exit();
}

#if defined(CURSOR_SHAPE) || defined(PROTO)
/*
 * May update the shape of the cursor.
 */
    void
ui_cursor_shape()
{
# ifdef FEAT_GUI
    if (gui.in_use)
	gui_update_cursor_later();
# endif
# ifdef MCH_CURSOR_SHAPE
    mch_update_cursor();
# endif
}
#endif

#if defined(FEAT_CLIPBOARD) || defined(FEAT_GUI) || defined(PROTO)
/*
 * Check bounds for column number
 */
    int
check_col(col)
    int	    col;
{
    if (col < 0)
	return 0;
    if (col >= (int)screen_Columns)
	return (int)screen_Columns - 1;
    return col;
}

/*
 * Check bounds for row number
 */
    int
check_row(row)
    int	    row;
{
    if (row < 0)
	return 0;
    if (row >= (int)screen_Rows)
	return (int)screen_Rows - 1;
    return row;
}
#endif

/*
 * Stuff for the X clipboard.  Shared between VMS and Unix.
 */

#if defined(FEAT_XCLIPBOARD) || defined(FEAT_GUI_X11) || defined(PROTO)
# include <X11/Xatom.h>
# include <X11/Intrinsic.h>

# if defined(FEAT_GUI) && defined(FEAT_XCLIPBOARD)
#  define X_DISPLAY	gui.in_use ? gui.dpy : xterm_dpy
# else
#  ifdef FEAT_GUI
#   define X_DISPLAY	gui.dpy
#  else
#   define X_DISPLAY	xterm_dpy
#  endif
# endif

/*
 * Open the application context (if it hasn't been opened yet).
 * Used for Motif and Athena GUI and the xterm clipboard.
 */
    void
open_app_context()
{
    if (app_context == NULL)
    {
	XtToolkitInitialize();
	app_context = XtCreateApplicationContext();
    }
}

static Atom	vim_atom;	/* Vim's own special selection format */
static Atom	compound_text_atom;
static Atom	text_atom;
static Atom	targets_atom;

    void
x11_setup_atoms(dpy)
    Display	*dpy;
{
    vim_atom           = XInternAtom(dpy, VIM_ATOM_NAME,   False);
    compound_text_atom = XInternAtom(dpy, "COMPOUND_TEXT", False);
    text_atom          = XInternAtom(dpy, "TEXT",          False);
    targets_atom       = XInternAtom(dpy, "TARGETS",       False);
    clip_star.sel_atom = XA_PRIMARY;
    clip_plus.sel_atom = XInternAtom(dpy, "CLIPBOARD",     False);
}

/*
 * X Selection stuff, for cutting and pasting text to other windows.
 */

static void  clip_x11_request_selection_cb __ARGS((Widget, XtPointer, Atom *, Atom *, XtPointer, long_u *, int *));

/* ARGSUSED */
    static void
clip_x11_request_selection_cb(w, success, sel_atom, type, value, length,
			      format)
    Widget	w;
    XtPointer	success;
    Atom	*sel_atom;
    Atom	*type;
    XtPointer	value;
    long_u	*length;
    int		*format;
{
    int		motion_type;
    long_u	len;
    char_u	*p;
    char	**text_list = NULL;
    VimClipboard	*cbd;

    if (*sel_atom == clip_plus.sel_atom)
	cbd = &clip_plus;
    else
        cbd = &clip_star;

    if (value == NULL || *length == 0)
    {
	clip_free_selection(cbd);	/* ???  [what's the query?] */
	*(int *)success = FALSE;
	return;
    }
    motion_type = MCHAR;
    p = (char_u *)value;
    len = *length;
    if (*type == vim_atom)
    {
	motion_type = *p++;
	len--;
    }
    else if (*type == compound_text_atom || (
#ifdef FEAT_MBYTE
		enc_dbcs != 0 &&
#endif
		*type == text_atom))
    {
	XTextProperty	text_prop;
	int		n_text = 0;
	int		status;

	text_prop.value = (unsigned char *)value;
	text_prop.encoding = *type;
	text_prop.format = *format;
	text_prop.nitems = STRLEN(value);
	status = XmbTextPropertyToTextList(X_DISPLAY, &text_prop,
							 &text_list, &n_text);
	if (status != Success || n_text < 1)
	{
	    *(int *)success = FALSE;
	    return;
	}
	p = (char_u *)text_list[0];
	len = STRLEN(p);
    }
    clip_yank_selection(motion_type, p, (long)len, cbd);

    if (text_list != NULL)
	XFreeStringList(text_list);

    XtFree((char *)value);
    *(int *)success = TRUE;
}

    void
clip_x11_request_selection(myShell, dpy, cbd)
    Widget	myShell;
    Display	*dpy;
    VimClipboard	*cbd;
{
    XEvent	event;
    Atom	type;
    static int	success;
    int		i;
    int		nbytes = 0;
    char_u	*buffer;

    for (i = 0; i < 4; i++)
    {
	switch (i)
	{
	    case 0:  type = vim_atom;	break;
	    case 1:  type = compound_text_atom; break;
	    case 2:  type = text_atom;	break;
	    default: type = XA_STRING;
	}
	XtGetSelectionValue(myShell, cbd->sel_atom, type,
	    clip_x11_request_selection_cb, (XtPointer)&success, CurrentTime);

	/* Make sure the request for the selection goes out before waiting for
	 * a response. */
	XFlush(dpy);

	/*
	 * Wait for result of selection request, otherwise if we type more
	 * characters, then they will appear before the one that requested the
	 * paste!  Don't worry, we will catch up with any other events later.
	 */
	for (;;)
	{
	    if (XCheckTypedEvent(dpy, SelectionNotify, &event))
		break;

	    /* Do we need this?  Probably not. */
	    XSync(dpy, False);
	}

	/* this is where clip_x11_request_selection_cb() is actually called */
	XtDispatchEvent(&event);

	if (success)
	    return;
    }

    /* Final fallback position - use the X CUT_BUFFER0 store */
    buffer = (char_u *)XFetchBuffer(dpy, &nbytes, 0);
    if (nbytes > 0)
    {
        /* Got something */
        clip_yank_selection(MCHAR, buffer, (long)nbytes, cbd);
        XFree((void *)buffer);
        if (p_verbose > 0)
            smsg((char_u *)_("Used CUT_BUFFER0 instead of empty selection") );
    }
}

static Boolean	clip_x11_convert_selection_cb __ARGS((Widget, Atom *, Atom *, Atom *, XtPointer *, long_u *, int *));

/* ARGSUSED */
    static Boolean
clip_x11_convert_selection_cb(w, sel_atom, target, type, value, length, format)
    Widget	w;
    Atom	*sel_atom;
    Atom	*target;
    Atom	*type;
    XtPointer	*value;
    long_u	*length;
    int		*format;
{
    char_u	*string;
    char_u	*result;
    int		motion_type;
    VimClipboard	*cbd;

    if (*sel_atom == clip_plus.sel_atom)
	cbd = &clip_plus;
    else
        cbd = &clip_star;

    if (!cbd->owned)
	return False;	    /* Shouldn't ever happen */

    /* requestor wants to know what target types we support */
    if (*target == targets_atom)
    {
	Atom *array;

	if ((array = (Atom *)XtMalloc((unsigned)(sizeof(Atom) * 5))) == NULL)
	    return False;
	*value = (XtPointer)array;
	array[0] = XA_STRING;
	array[1] = targets_atom;
	array[2] = vim_atom;
	array[3] = text_atom;
	array[4] = compound_text_atom;
	*type = XA_ATOM;
	*format = sizeof(Atom) * 8;
	*length = 5;
	return True;
    }

    if (       *target != XA_STRING
	    && *target != vim_atom
	    && *target != text_atom
	    && *target != compound_text_atom)
	return False;

    clip_get_selection(cbd);
    motion_type = clip_convert_selection(&string, length, cbd);
    if (motion_type < 0)
	return False;

    /* For our own format, the first byte contains the motion type */
    if (*target == vim_atom)
	(*length)++;

    *value = XtMalloc((Cardinal)*length);
    result = (char_u *)*value;
    if (result == NULL)
    {
	vim_free(string);
	return False;
    }

    if (*target == XA_STRING)
    {
	mch_memmove(result, string, (size_t)(*length));
	*type = XA_STRING;
    }
    else if (*target == compound_text_atom
	    || *target == text_atom)
    {
	XTextProperty	text_prop;
	char		*string_nt = (char *)alloc((unsigned)*length + 1);

	/* create NUL terminated string which XmbTextListToTextProperty wants */
	mch_memmove(string_nt, string, (size_t)*length);
	string_nt[*length] = NUL;
	XmbTextListToTextProperty(X_DISPLAY, (char **)&string_nt, 1,
					      XCompoundTextStyle, &text_prop);
	vim_free(string_nt);
	XtFree(*value);			/* replace with COMPOUND text */
	*value = (XtPointer)(text_prop.value);	/*    from plain text */
	*length = text_prop.nitems;
	*type = compound_text_atom;
    }
    else
    {
	result[0] = motion_type;
	mch_memmove(result + 1, string, (size_t)(*length - 1));
	*type = vim_atom;
    }
    *format = 8;	    /* 8 bits per char */
    vim_free(string);
    return True;
}

static void  clip_x11_lose_ownership_cb __ARGS((Widget, Atom *));

/* ARGSUSED */
    static void
clip_x11_lose_ownership_cb(w, sel_atom)
    Widget  w;
    Atom    *sel_atom;
{
    if (*sel_atom == clip_plus.sel_atom)
        clip_lose_selection(&clip_plus);
    else
        clip_lose_selection(&clip_star);
}

    void
clip_x11_lose_selection(myShell, cbd)
    Widget	myShell;
    VimClipboard	*cbd;
{
    XtDisownSelection(myShell, cbd->sel_atom, CurrentTime);
}

    int
clip_x11_own_selection(myShell, cbd)
    Widget	myShell;
    VimClipboard	*cbd;
{
    if (XtOwnSelection(myShell, cbd->sel_atom, CurrentTime,
	    clip_x11_convert_selection_cb, clip_x11_lose_ownership_cb,
							       NULL) == False)
	return FAIL;
    return OK;
}

/*
 * Send the current selection to the clipboard.  Do nothing for X because we
 * will fill in the selection only when requested by another app.
 */
/*ARGSUSED*/
    void
clip_x11_set_selection(cbd)
    VimClipboard *cbd;
{
}
#endif

#if defined(FEAT_MOUSE) || defined(PROTO)
static int mouse_comp_pos __ARGS((int *rowp, int *colp, linenr_T *lnump));

/*
 * Move the cursor to the specified row and column on the screen.
 * Change current window if neccesary.	Returns an integer with the
 * CURSOR_MOVED bit set if the cursor has moved or unset otherwise.
 *
 * The MOUSE_FOLD_CLOSE bit is set when clicked on the '-' in a fold column.
 * The MOUSE_FOLD_OPEN bit is set when clicked on the '+' in a fold column.
 *
 * If flags has MOUSE_FOCUS, then the current window will not be changed, and
 * if the mouse is outside the window then the text will scroll, or if the
 * mouse was previously on a status line, then the status line may be dragged.
 *
 * If flags has MOUSE_MAY_VIS, then VIsual mode will be started before the
 * cursor is moved unless the cursor was on a status line.
 * This function returns one of IN_UNKNOWN, IN_BUFFER, IN_STATUS_LINE or
 * IN_SEP_LINE depending on where the cursor was clicked.
 *
 * If flags has MOUSE_MAY_STOP_VIS, then Visual mode will be stopped, unless
 * the mouse is on the status line of the same window.
 *
 * If flags has MOUSE_DID_MOVE, nothing is done if the mouse didn't move since
 * the last call.
 *
 * If flags has MOUSE_SETPOS, nothing is done, only the current position is
 * remembered.
 */
    int
jump_to_mouse(flags, inclusive, which_button)
    int		flags;
    int		*inclusive;	/* used for inclusive operator, can be NULL */
    int		which_button;	/* MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE */
{
    static int	on_status_line = 0;	/* #lines below bottom of window */
#ifdef FEAT_VERTSPLIT
    static int	on_sep_line = 0;	/* on separator right of window */
#endif
    static int	prev_row = -1;
    static int	prev_col = -1;

    win_T	*wp, *old_curwin;
    pos_T	old_cursor;
    int		count;
    int		first;
    int		row = mouse_row;
    int		col = mouse_col;
#ifdef FEAT_FOLDING
    int		mouse_char;
#endif

    mouse_past_bottom = FALSE;
    mouse_past_eol = FALSE;

    if ((flags & MOUSE_DID_MOVE)
	    && prev_row == mouse_row
	    && prev_col == mouse_col)
    {
retnomove:
	/* before moving the cursor for a left click wich is NOT in a status
	 * line, stop Visual mode */
	if (on_status_line)
	    return IN_STATUS_LINE;
#ifdef FEAT_VERTSPLIT
	if (on_sep_line)
	    return IN_SEP_LINE;
#endif
#ifdef FEAT_VISUAL
	if (flags & MOUSE_MAY_STOP_VIS)
	{
	    end_visual_mode();
	    redraw_curbuf_later(INVERTED);	/* delete the inversion */
	}
#endif
#if defined(FEAT_CMDWIN) && defined(FEAT_CLIPBOARD)
	/* Continue a modeless selection in another window. */
	if (cmdwin_type != 0 && row < W_WINROW(curwin))
	    return IN_OTHER_WIN;
#endif
	return IN_BUFFER;
    }

    prev_row = mouse_row;
    prev_col = mouse_col;

    if (flags & MOUSE_SETPOS)
	goto retnomove;				/* ugly goto... */

#ifdef FEAT_FOLDING
    /* Remember the character under the mouse, it might be a '-' or '+' in the
     * fold column. */
    if (row >= 0 && row < Rows && col >= 0 && col <= Columns)
	mouse_char = ScreenLines[LineOffset[row] + col];
    else
	mouse_char = ' ';
#endif

    old_curwin = curwin;
    old_cursor = curwin->w_cursor;

    if (!(flags & MOUSE_FOCUS))
    {
	if (row < 0 || col < 0)			/* check if it makes sense */
	    return IN_UNKNOWN;

#ifdef FEAT_WINDOWS
	/* find the window where the row is in */
	wp = mouse_find_win(&row, &col);
#else
	wp = firstwin;
#endif
	/*
	 * winpos and height may change in win_enter()!
	 */
	if (row >= wp->w_height)		/* In (or below) status line */
	    on_status_line = row - wp->w_height + 1;
	else
	    on_status_line = 0;
#ifdef FEAT_VERTSPLIT
	if (col >= wp->w_width)		/* In separator line */
	    on_sep_line = col - wp->w_width + 1;
	else
	    on_sep_line = 0;

	/* The rightmost character of the status line might be a vertical
	 * separator character if there is no connecting window to the right. */
	if (on_status_line && on_sep_line)
	{
	    if (stl_connected(wp))
		on_sep_line = 0;
	    else
		on_status_line = 0;
	}
#endif

#ifdef FEAT_VISUAL
	/* Before jumping to another buffer, or moving the cursor for a left
	 * click, stop Visual mode. */
	if (VIsual_active
		&& (wp->w_buffer != curwin->w_buffer
		    || (!on_status_line
# ifdef FEAT_VERTSPLIT
			&& !on_sep_line
# endif
# ifdef FEAT_FOLDING
			&& (
#  ifdef FEAT_RIGHTLEFT
			    wp->w_p_rl ? col < W_WIDTH(wp) - wp->w_p_fdc :
#  endif
			    col >= wp->w_p_fdc
#  ifdef FEAT_CMDWIN
				  + (cmdwin_type == 0 && wp == curwin ? 0 : 1)
#  endif
			    )
# endif
			&& (flags & MOUSE_MAY_STOP_VIS))))
	{
	    end_visual_mode();
	    redraw_curbuf_later(INVERTED);	/* delete the inversion */
	}
#endif
#ifdef FEAT_CMDWIN
	if (cmdwin_type != 0 && wp != curwin)
	{
	    /* A click outside the command-line window: Use modeless
	     * selection if possible. */
# ifdef FEAT_CLIPBOARD
	    return IN_OTHER_WIN;
# else
	    row = 0;
	    col += wp->w_wincol;
	    wp = curwin;
	    on_status_line = 0;
	    on_sep_line = 0;
# endif
	}
#endif
#ifdef FEAT_WINDOWS
	win_enter(wp, TRUE);			/* can make wp invalid! */
# ifdef CHECK_DOUBLE_CLICK
	/* set topline, to be able to check for double click ourselves */
	if (curwin != old_curwin)
	    set_mouse_topline(curwin);
# endif
#endif
	if (on_status_line)			/* In (or below) status line */
	{
	    /* Don't use start_arrow() if we're in the same window */
	    if (curwin == old_curwin)
		return IN_STATUS_LINE;
	    else
		return IN_STATUS_LINE | CURSOR_MOVED;
	}
#ifdef FEAT_VERTSPLIT
	if (on_sep_line)			/* In (or below) status line */
	{
	    /* Don't use start_arrow() if we're in the same window */
	    if (curwin == old_curwin)
		return IN_SEP_LINE;
	    else
		return IN_SEP_LINE | CURSOR_MOVED;
	}
#endif

	curwin->w_cursor.lnum = curwin->w_topline;
#ifdef FEAT_GUI
	/* remember topline, needed for double click */
	gui_prev_topline = curwin->w_topline;
# ifdef FEAT_DIFF
	gui_prev_topfill = curwin->w_topfill;
# endif
#endif
    }
    else if (on_status_line && which_button == MOUSE_LEFT)
    {
#ifdef FEAT_WINDOWS
	/* Drag the status line */
	count = row - curwin->w_winrow - curwin->w_height + 1 - on_status_line;
	win_drag_status_line(count);
#endif
	return IN_STATUS_LINE;			/* Cursor didn't move */
    }
#ifdef FEAT_VERTSPLIT
    else if (on_sep_line && which_button == MOUSE_LEFT)
    {
	/* Drag the separator column */
	count = col - curwin->w_wincol - curwin->w_width + 1 - on_sep_line;
	win_drag_vsep_line(count);
	return IN_SEP_LINE;			/* Cursor didn't move */
    }
#endif
    else /* keep_window_focus must be TRUE */
    {
#ifdef FEAT_VISUAL
	/* before moving the cursor for a left click, stop Visual mode */
	if (flags & MOUSE_MAY_STOP_VIS)
	{
	    end_visual_mode();
	    redraw_curbuf_later(INVERTED);	/* delete the inversion */
	}
#endif

#if defined(FEAT_CMDWIN) && defined(FEAT_CLIPBOARD)
	/* Continue a modeless selection in another window. */
	if (cmdwin_type != 0 && row < W_WINROW(curwin))
	    return IN_OTHER_WIN;
#endif

	row -= W_WINROW(curwin);
#ifdef FEAT_VERTSPLIT
	col -= W_WINCOL(curwin);
#endif

	/*
	 * When clicking beyond the end of the window, scroll the screen.
	 * Scroll by however many rows outside the window we are.
	 */
	if (row < 0)
	{
	    count = 0;
	    for (first = TRUE; curwin->w_topline > 1; )
	    {
#ifdef FEAT_DIFF
		if (curwin->w_topfill < diff_check(curwin, curwin->w_topline))
		    ++count;
		else
#endif
		    count += plines(curwin->w_topline - 1);
		if (!first && count > -row)
		    break;
		first = FALSE;
#ifdef FEAT_FOLDING
		hasFolding(curwin->w_topline, &curwin->w_topline, NULL);
#endif
#ifdef FEAT_DIFF
		if (curwin->w_topfill < diff_check(curwin, curwin->w_topline))
		    ++curwin->w_topfill;
		else
#endif
		{
		    --curwin->w_topline;
#ifdef FEAT_DIFF
		    curwin->w_topfill = 0;
#endif
		}
	    }
	    curwin->w_valid &=
		      ~(VALID_WROW|VALID_CROW|VALID_BOTLINE|VALID_BOTLINE_AP);
	    redraw_later(VALID);
	    row = 0;
	}
	else if (row >= curwin->w_height)
	{
	    count = 0;
	    for (first = TRUE; curwin->w_topline < curbuf->b_ml.ml_line_count; )
	    {
#ifdef FEAT_DIFF
		if (curwin->w_topfill > 0)
		    ++count;
		else
#endif
		    count += plines(curwin->w_topline);
		if (!first && count > row - curwin->w_height + 1)
		    break;
		first = FALSE;
#ifdef FEAT_FOLDING
		if (hasFolding(curwin->w_topline, NULL, &curwin->w_topline)
			&& curwin->w_topline == curbuf->b_ml.ml_line_count)
		    break;
#endif
#ifdef FEAT_DIFF
		if (curwin->w_topfill > 0)
		    --curwin->w_topfill;
		else
#endif
		{
		    ++curwin->w_topline;
#ifdef FEAT_DIFF
		    curwin->w_topfill =
				   diff_check_fill(curwin, curwin->w_topline);
#endif
		}
	    }
	    redraw_later(VALID);
	    curwin->w_valid &=
		      ~(VALID_WROW|VALID_CROW|VALID_BOTLINE|VALID_BOTLINE_AP);
	    row = curwin->w_height - 1;
	}
	else if (row == 0)
	{
	    /* When dragging the mouse, while the text has been scrolled up as
	     * far as it goes, moving the mouse in the top line should scroll
	     * the text down (done later when recomputing w_topline). */
	    if (mouse_dragging
		    && curwin->w_cursor.lnum
				       == curwin->w_buffer->b_ml.ml_line_count
		    && curwin->w_cursor.lnum == curwin->w_topline)
		curwin->w_valid &= ~(VALID_TOPLINE);
	}
    }

#ifdef FEAT_FOLDING
    /* Check for position outside of the fold column. */
    if (
# ifdef FEAT_RIGHTLEFT
	    curwin->w_p_rl ? col < W_WIDTH(curwin) - curwin->w_p_fdc :
# endif
	    col >= curwin->w_p_fdc
#  ifdef FEAT_CMDWIN
				+ (cmdwin_type == 0 ? 0 : 1)
#  endif
       )
	mouse_char = ' ';
#endif

    /* compute the position in the buffer line from the posn on the screen */
    if (mouse_comp_pos(&row, &col, &curwin->w_cursor.lnum))
	mouse_past_bottom = TRUE;

#ifdef FEAT_VISUAL
    /* Start Visual mode before coladvance(), for when 'sel' != "old" */
    if ((flags & MOUSE_MAY_VIS) && !VIsual_active)
    {
	check_visual_highlight();
	VIsual = old_cursor;
	VIsual_active = TRUE;
	VIsual_reselect = TRUE;
	/* if 'selectmode' contains "mouse", start Select mode */
	may_start_select('o');
	setmouse();
	if (p_smd)
	    redraw_cmdline = TRUE;	/* show visual mode later */
    }
#endif

    curwin->w_curswant = col;
    curwin->w_set_curswant = FALSE;	/* May still have been TRUE */
    if (coladvance(col) == FAIL)	/* Mouse click beyond end of line */
    {
	if (inclusive != NULL)
	    *inclusive = TRUE;
	mouse_past_eol = TRUE;
    }
    else if (inclusive != NULL)
	*inclusive = FALSE;

    count = IN_BUFFER;
    if (curwin != old_curwin || curwin->w_cursor.lnum != old_cursor.lnum
	    || curwin->w_cursor.col != old_cursor.col)
	count |= CURSOR_MOVED;		/* Cursor has moved */

#ifdef FEAT_FOLDING
    if (mouse_char == '+')
	count |= MOUSE_FOLD_OPEN;
    else if (mouse_char != ' ')
	count |= MOUSE_FOLD_CLOSE;
#endif

    return count;
}

/*
 * Compute the position in the buffer line from the posn on the screen.
 * Only works for current window.
 * Returns TRUE if the position is below the last line.
 */
    static int
mouse_comp_pos(rowp, colp, lnump)
    int		*rowp;
    int		*colp;
    linenr_T	*lnump;
{
    int		col = *colp;
    int		row = *rowp;
    linenr_T	lnum;
    int		retval = FALSE;
    int		off;
    int		count;

#ifdef FEAT_RIGHTLEFT
    if (curwin->w_p_rl)
	col = W_WIDTH(curwin) - 1 - col;
#endif

    lnum = curwin->w_topline;

    while (row > 0)
    {
#ifdef FEAT_DIFF
	/* Don't include filler lines in "count" */
	if (curwin->w_p_diff && !hasFolding(lnum, NULL, NULL))
	{
	    if (lnum == curwin->w_topline)
		row -= curwin->w_topfill;
	    else
		row -= diff_check_fill(curwin, lnum);
	    count = plines_nofill(lnum);
	}
	else
#endif
	    count = plines(lnum);
	if (count > row)
	{
	    /* Position is in this buffer line.  Compute the column
	     * without wrapping. */
	    off = curwin_col_off() - curwin_col_off2();
	    if (col < off)
		col = off;
	    col += row * (W_WIDTH(curwin) - off);
	    /* add skip column (for long wrapping line) */
	    col += curwin->w_skipcol;
	    break;
	}
#ifdef FEAT_FOLDING
	(void)hasFolding(lnum, NULL, &lnum);
#endif
	if (lnum == curbuf->b_ml.ml_line_count)
	{
	    retval = TRUE;
	    break;		/* past end of file */
	}
	row -= count;
	++lnum;
    }

    if (!curwin->w_p_wrap)
	col += curwin->w_leftcol;

    /* skip line number and fold column in front of the line */
    col -= curwin_col_off();
    if (col < 0)
	col = 0;

    *colp = col;
    *rowp = row;
    *lnump = lnum;
    return retval;
}

#ifdef FEAT_WINDOWS
/*
 * Find the window at screen position "*rowp" and "*colp".  The positions are
 * updated to become relative to the top-left of the window.
 */
/*ARGSUSED*/
    win_T *
mouse_find_win(rowp, colp)
    int		*rowp;
    int		*colp;
{
    frame_T	*fp;

    fp = topframe;
    for (;;)
    {
	if (fp->fr_layout == FR_LEAF)
	    break;
#ifdef FEAT_VERTSPLIT
	if (fp->fr_layout == FR_ROW)
	{
	    for (fp = fp->fr_child; fp->fr_next != NULL; fp = fp->fr_next)
	    {
		if (*colp < fp->fr_width)
		    break;
		*colp -= fp->fr_width;
	    }
	}
#endif
	else    /* fr_layout == FR_COL */
	{
	    for (fp = fp->fr_child; fp->fr_next != NULL; fp = fp->fr_next)
	    {
		if (*rowp < fp->fr_height)
		    break;
		*rowp -= fp->fr_height;
	    }
	}
    }
    return fp->fr_win;
}
#endif

#if defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_GTK) || defined (FEAT_GUI_MAC) \
	|| defined(FEAT_GUI_ATHENA) || defined(FEAT_GUI_MSWIN) \
	|| defined(FEAT_GUI_PHOTON) || defined(PROTO)
/*
 * Translate window coordinates to buffer position without any side effects
 */
    int
get_fpos_of_mouse(mpos)
    pos_T	*mpos;
{
    win_T	*wp;
    int		count;
    char_u	*ptr;
    int		row = mouse_row;
    int		col = mouse_col;

    if (row < 0 || col < 0)		/* check if it makes sense */
	return IN_UNKNOWN;

#ifdef FEAT_WINDOWS
    /* find the window where the row is in */
    wp = mouse_find_win(&row, &col);
#else
    wp = firstwin;
#endif
    /*
     * winpos and height may change in win_enter()!
     */
    if (row >= wp->w_height)	/* In (or below) status line */
	return IN_STATUS_LINE;
#ifdef FEAT_VERTSPLIT
    if (col >= wp->w_width)	/* In vertical separator line */
	return IN_SEP_LINE;
#endif

    if (wp != curwin)
	return IN_UNKNOWN;

    /* compute the position in the buffer line from the posn on the screen */
    if (mouse_comp_pos(&row, &col, &mpos->lnum))
	return IN_STATUS_LINE; /* past bottom */

    /* try to advance to the specified column */
    mpos->col = 0;
    count = 0;
    ptr = ml_get_buf(wp->w_buffer, mpos->lnum, FALSE);
    while (count <= col && *ptr != NUL)
    {
	++mpos->col;
	count += win_lbr_chartabsize(wp, ptr, count, NULL);
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    ptr += (*mb_ptr2len_check)(ptr);
	else
#endif
	    ++ptr;
    }
    if (mpos->col == 0)
	return IN_BUFFER;
    --mpos->col;
    return IN_BUFFER;
}
#endif

#endif /* FEAT_MOUSE */

#if defined(FEAT_GUI) || defined(WIN32) || defined(PROTO)
/*
 * Called when focus changed.  Used for the GUI or for systems where this can
 * be done in the console (Win32).
 */
    void
ui_focus_change(in_focus)
    int		in_focus;	/* TRUE if focus gained. */
{
    static time_t	last_time = (time_t)0;
    int			need_redraw = FALSE;

    /* When activated: Check if any file was modified outside of Vim.
     * Only do this when not done within the last two seconds (could get
     * several events in a row). */
    if (in_focus && last_time + 2 < time(NULL))
    {
	need_redraw = check_timestamps(
# ifdef FEAT_GUI
		gui.in_use
# else
		FALSE
# endif
		);
	last_time = time(NULL);
    }

#ifdef FEAT_AUTOCMD
    /*
     * Fire the focus gained/lost autocommand.
     */
    need_redraw |= apply_autocmds(in_focus ? EVENT_FOCUSGAINED
				: EVENT_FOCUSLOST, NULL, NULL, FALSE, curbuf);
#endif

    if (need_redraw)
    {
	/* Something was executed, make sure the cursor is put back where it
	 * belongs. */
	need_wait_return = FALSE;

	if (State & CMDLINE)
	    redrawcmdline();
	else if (State == HITRETURN || State == SETWSIZE || State == ASKMORE
		|| State == EXTERNCMD || State == CONFIRM || exmode_active)
	    repeat_message();
	else if ((State & NORMAL) || (State & INSERT))
	{
	    if (must_redraw != 0)
		update_screen(0);
	    setcursor();
	}
	cursor_on();	    /* redrawing may have switched it off */
	out_flush();
    }
}
#endif
