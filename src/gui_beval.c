/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Visual Workshop integration by Gordon Prieur
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#if (defined(FEAT_BEVAL) && \
    (defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_ATHENA))) || defined(PROTO)

#include <X11/keysym.h>
#ifdef FEAT_GUI_MOTIF
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/List.h>
#include <Xm/Label.h>
#include <Xm/AtomMgr.h>
#include <Xm/Protocols.h>
#else
    /* Assume Athena */
#include <X11/Shell.h>
#include <X11/Xaw/Label.h>
#endif

#include "gui_beval.h"

extern Widget vimShell;

static void addEventHandler __ARGS((Widget, BalloonEval *));
static void removeEventHandler __ARGS((BalloonEval *));
static void pointerEventEH __ARGS((Widget, XtPointer, XEvent *, Boolean *));
static void pointerEvent __ARGS((BalloonEval *, XEvent *));
static void timerRoutine __ARGS((XtPointer, XtIntervalId *));
static void cancelBalloon __ARGS((BalloonEval *));
static void requestBalloon __ARGS((BalloonEval *));
static void drawBalloon __ARGS((BalloonEval *));
static void undrawBalloon __ARGS((BalloonEval *beval));
static void createBalloonEvalWindow __ARGS((BalloonEval *));



/*
 * Create a balloon-evaluation area for a Widget.
 * There can be either a "msg" for a fixed string or "msgCB" to generate a
 * message by calling this callback function.
 * When "msg" is not NULL it must remain valid for as long as the balloon is
 * used.  It is not freed here.
 * Returns a pointer to the resulting object (NULL when out of memory).
 */
    BalloonEval *
gui_mch_create_beval_area(target, msg, msgCB, clientData)
    Widget	target;
    char_u	*msg;
    void	(*msgCB)__ARGS((BalloonEval *, int));
    XtPointer	clientData;
{
    char	*display_name;	    /* get from gui.dpy */
    int		screen_num;
    char	*p;
    BalloonEval	*beval;

    if (msg != NULL && msgCB != NULL)
    {
	EMSG(_("E232: Cannot create BalloonEval with both message and callback"));
	return NULL;
    }

    beval = (BalloonEval *)alloc(sizeof(BalloonEval));
    if (beval != NULL)
    {
	beval->target = target;
	beval->balloonShell = NULL;
	beval->timerID = (XtIntervalId)NULL;
	beval->appContext = XtWidgetToApplicationContext(target);
	beval->showState = ShS_NEUTRAL;
	beval->x = 0;
	beval->y = 0;
	beval->msg = msg;
	beval->msgCB = msgCB;
	beval->clientData = clientData;

	/*
	 * Set up event handler which will keep its eyes on the pointer,
	 * and when the pointer rests in a certain spot for a given time
	 * interval, show the beval.
	 */
	addEventHandler(target, beval);
	createBalloonEvalWindow(beval);

	/*
	 * Now create and save the screen width and height. Used in drawing.
	 */
	display_name = DisplayString(gui.dpy);
	p = strrchr(display_name, '.');
	if (p++ != NULL)
	    screen_num = atoi(p);
	else
	    screen_num = 0;
	beval->screen_width = DisplayWidth(gui.dpy, screen_num);
	beval->screen_height = DisplayHeight(gui.dpy, screen_num);
    }

    return beval;
}

/*
 * Destroy a ballon-eval and free its associated memory.
 */
    void
gui_mch_destroy_beval_area(beval)
    BalloonEval	*beval;
{
    cancelBalloon(beval);
    removeEventHandler(beval);
    /* Children will automatically be destroyed */
    XtDestroyWidget(beval->balloonShell);
    vim_free(beval);
}

    void
gui_mch_enable_beval_area(beval)
    BalloonEval	*beval;
{
    if (beval != NULL)
	addEventHandler(beval->target, beval);
}

    void
gui_mch_disable_beval_area(beval)
    BalloonEval	*beval;
{
    if (beval != NULL)
	removeEventHandler(beval);
}

#if defined(FEAT_SUN_WORKSHOP) || defined(PROTO)
    Boolean
gui_mch_get_beval_info(beval, filename, line, text, index)
    BalloonEval	*beval;
    char_u     **filename;
    int		*line;
    char_u     **text;
    int		*index;
{
    win_T	*wp;
    int		row, col;
    int		row_off;
    int		i;
    int		top_off;
    char_u	*lbuf;

    top_off = Y_2_ROW(beval->y);
    for (wp = firstwin; wp != NULL; wp = W_NEXT(wp))
	if (W_WINROW(wp) <= top_off && top_off < (W_WINROW(wp) + wp->w_height))
	    break;

    if (wp != NULL)
    {
	row_off = top_off - W_WINROW(wp) + 1;
	i = 0;
	row = 0;
	while (i < row_off && row < wp->w_lines_valid)
	{
	    i += wp->w_lines[row].wl_size;
	    row++;
	}

	if (wp->w_lines[row - 1].wl_size > 1)
	    col = ((top_off - (i - (int)(wp->w_lines[row - 1]).wl_size)) *
		    gui.num_cols) + X_2_COL(beval->x) + 1;
	else
	    col = X_2_COL(beval->x) + 1;
	if (wp->w_p_nu)
	    col -= 8;

	if (!wp->w_p_wrap)
	    col += wp->w_leftcol;

	row += wp->w_topline - 1;
	if (col > 0)
	{
	    lbuf = ml_get_buf(wp->w_buffer, (linenr_T)row, FALSE);
	    win_linetabsize(wp, lbuf, (colnr_T)MAXCOL);
	    if (i >= col)		/* don't send if past end of line */
	    {
		*filename = wp->w_buffer->b_ffname;
		*line = row;
		*text = lbuf;
		*index = col;
		beval->ts = wp->w_buffer->b_p_ts;
		return True;
	    }
	}
    }

    return False;
}

/*
 * Show a balloon with "msg".
 */
    void
gui_mch_post_balloon(beval, msg)
    BalloonEval	*beval;
    char_u	*msg;
{
    beval->msg = msg;
    if (msg != NULL)
	drawBalloon(beval);
    else
	undrawBalloon(beval);
}
#endif

    static void
addEventHandler(target, beval)
    Widget	target;
    BalloonEval	*beval;
{
    XtAddEventHandler(target,
			PointerMotionMask | EnterWindowMask |
			LeaveWindowMask | ButtonPressMask | KeyPressMask |
			KeyReleaseMask,
			False,
			pointerEventEH, (XtPointer)beval);
}

    static void
removeEventHandler(beval)
    BalloonEval	*beval;
{
    XtRemoveEventHandler(beval->target,
			PointerMotionMask | EnterWindowMask |
			LeaveWindowMask | ButtonPressMask | KeyPressMask |
			KeyReleaseMask,
			False,
			pointerEventEH, (XtPointer)beval);
}


/*
 * The X event handler. All it does is call the real event handler.
 */
/*ARGSUSED*/
    static void
pointerEventEH(w, client_data, event, unused)
    Widget	w;
    XtPointer	client_data;
    XEvent	*event;
    Boolean	*unused;
{
    BalloonEval *beval = (BalloonEval *)client_data;
    pointerEvent(beval, event);
}


/*
 * The real event handler. Called by pointerEventEH() whenever an event we are
 * interested in ocurrs.
 */

    static void
pointerEvent(beval, event)
    BalloonEval	*beval;
    XEvent	*event;
{
    Position	distance;	    /* a measure of how much the ponter moved */
    Position	delta;		    /* used to compute distance */

    switch (event->type)
    {
	case EnterNotify:
	case MotionNotify:
	    delta = event->xmotion.x - beval->x;
	    if (delta < 0)
		delta = -delta;
	    distance = delta;
	    delta = event->xmotion.y - beval->y;
	    if (delta < 0)
		delta = -delta;
	    distance += delta;
	    if (distance > 4)
	    {
		/*
		 * Moved out of the balloon location: cancel it.
		 * Remember button state
		 */
		beval->state = event->xmotion.state;
		if (beval->state & (Button1Mask|Button2Mask|Button3Mask))
		{
		    /* Mouse buttons are pressed - no balloon now */
		    cancelBalloon(beval);
		}
		else if (beval->state & (Mod1Mask|Mod2Mask|Mod3Mask))
		{
		    /*
		     * Alt is pressed -- enter super-evaluate-mode,
		     * where there is no time delay
		     */
		    beval->x = event->xmotion.x;
		    beval->y = event->xmotion.y;
		    beval->x_root = event->xmotion.x_root;
		    beval->y_root = event->xmotion.y_root;
		    cancelBalloon(beval);
		    if (beval->msgCB != NULL)
		    {
			beval->showState = ShS_PENDING;
			(*beval->msgCB)(beval, beval->state);
		    }
		}
		else
		{
		    beval->x = event->xmotion.x;
		    beval->y = event->xmotion.y;
		    beval->x_root = event->xmotion.x_root;
		    beval->y_root = event->xmotion.y_root;
		    cancelBalloon(beval);
		    beval->timerID = XtAppAddTimeOut( beval->appContext,
					(long_u)p_bdlay, timerRoutine, beval);
		}
	    }
	    break;

	case KeyPress:
	    if (beval->showState == ShS_SHOWING && beval->msgCB != NULL)
	    {
		Modifiers   modifier;
		KeySym	    keysym;

		XtTranslateKeycode(gui.dpy,
				       event->xkey.keycode, event->xkey.state,
				       &modifier, &keysym);
		if (keysym == XK_Shift_L || keysym == XK_Shift_R)
		{
		    beval->showState = ShS_UPDATE_PENDING;
		    (*beval->msgCB)(beval, ShiftMask);
		}
		else if (keysym == XK_Control_L || keysym == XK_Control_R)
		{
		    beval->showState = ShS_UPDATE_PENDING;
		    (*beval->msgCB)(beval, ControlMask);
		}
		else
		    cancelBalloon(beval);
	    }
	    else
		cancelBalloon(beval);
	    break;

	case KeyRelease:
	    if (beval->showState == ShS_SHOWING && beval->msgCB != NULL)
	    {
		Modifiers modifier;
		KeySym keysym;

		XtTranslateKeycode(gui.dpy, event->xkey.keycode,
				event->xkey.state, &modifier, &keysym);
		if ((keysym == XK_Shift_L) || (keysym == XK_Shift_R)) {
		    beval->showState = ShS_UPDATE_PENDING;
		    (*beval->msgCB)(beval, 0);
		}
		else if ((keysym == XK_Control_L) || (keysym == XK_Control_R))
		{
		    beval->showState = ShS_UPDATE_PENDING;
		    (*beval->msgCB)(beval, 0);
		}
		else
		    cancelBalloon(beval);
	    }
	    else
		cancelBalloon(beval);
	    break;

	case LeaveNotify:
		/* Ignore LeaveNotify events that are not "normal".
		 * Apparently we also get it when somebody else grabs focus.
		 * Happens for me every two seconds (some clipboard tool?) */
		if (event->xcrossing.mode == NotifyNormal)
		    cancelBalloon(beval);
		break;

	case ButtonPress:
		cancelBalloon(beval);
		break;

	default:
	    break;
    }
}

/*ARGSUSED*/
    static void
timerRoutine(dx, id)
    XtPointer	    dx;
    XtIntervalId    *id;
{
    BalloonEval *beval = (BalloonEval *)dx;

    beval->timerID = (XtIntervalId)NULL;

    /*
     * If the timer event happens then the mouse has stopped long enough for
     * a request to be started. The request will only send to the debugger if
     * there the mouse is pointing at real data.
     */
    requestBalloon(beval);
}

    static void
requestBalloon(beval)
    BalloonEval	*beval;
{
    if (beval->showState != ShS_PENDING)
    {
	/* Determine the beval to display */
	if (beval->msg != NULL)
	    drawBalloon(beval);
	else if (beval->msgCB != NULL)
	{
	    beval->showState = ShS_PENDING;
	    (*beval->msgCB)(beval, beval->state);
	}
    }
}

/*
 * Draw a balloon.
 */
    static void
drawBalloon(beval)
    BalloonEval	*beval;
{
    Dimension	w;
    Dimension	h;
    Position tx;
    Position ty;

    if (beval->msg != NULL)
    {
	/* Show the Balloon */

	/* Calculate the label's width and height */
#ifdef FEAT_GUI_MOTIF
	XmString s;

	s = XmStringCreateLocalized((char *)beval->msg);
	XmStringExtent(gui.balloonEval_fontList, s, &w, &h);
	w += gui.border_offset << 1;
	h += gui.border_offset << 1;
	XtVaSetValues(beval->balloonLabel, XmNlabelString, s, NULL);
	XmStringFree(s);
#else
	/* Athena */
	/* Assume XtNinternational == True */
	XFontSet	fset;
	XFontSetExtents *ext;

	XtVaGetValues(beval->balloonLabel, XtNfontSet, &fset, NULL);
	ext = XExtentsOfFontSet(fset);
	h = ext->max_ink_extent.height;
	w = XmbTextEscapement(fset, (char *)beval->msg,
						     (int)STRLEN(beval->msg));
	XtVaSetValues(beval->balloonLabel, XtNlabel, beval->msg, NULL);
#endif

	/* Compute position of the balloon area */
	tx = beval->x_root + EVAL_OFFSET_X;
	ty = beval->y_root + EVAL_OFFSET_Y;
	if ((tx + w) > beval->screen_width)
	    tx = beval->screen_width - w;
	if ((ty + h) > beval->screen_height)
	    ty = beval->screen_height - h;
#ifdef FEAT_GUI_MOTIF
	XtVaSetValues(beval->balloonShell,
		XmNx, tx,
		XmNy, ty,
		NULL);
#else
	/* Athena */
	XtVaSetValues(beval->balloonShell,
		XtNx, tx,
		XtNy, ty,
		NULL);
#endif

	XtPopup(beval->balloonShell, XtGrabNone);

	beval->showState = ShS_SHOWING;
    }
}

/*
 * Undraw a balloon.
 */
    static void
undrawBalloon(beval)
    BalloonEval *beval;
{
    if (beval->balloonShell != (Widget)0)
	XtPopdown(beval->balloonShell);
    beval->showState = ShS_NEUTRAL;
}

    static void
cancelBalloon(beval)
    BalloonEval	*beval;
{
    if (beval->showState == ShS_SHOWING
	    || beval->showState == ShS_UPDATE_PENDING)
	undrawBalloon(beval);

    if (beval->timerID != (XtIntervalId)NULL)
    {
	XtRemoveTimeOut(beval->timerID);
	beval->timerID = (XtIntervalId)NULL;
    }
    beval->showState = ShS_NEUTRAL;
}


    static void
createBalloonEvalWindow(beval)
    BalloonEval	*beval;
{
    Arg		args[12];
    int		n;

    n = 0;
#ifdef FEAT_GUI_MOTIF
    XtSetArg(args[n], XmNallowShellResize, True); n++;
    beval->balloonShell = XtAppCreateShell("balloonEval", "BalloonEval",
		    overrideShellWidgetClass, gui.dpy, args, n);
#else
    /* Athena */
    XtSetArg(args[n], XtNallowShellResize, True); n++;
    beval->balloonShell = XtAppCreateShell("balloonEval", "BalloonEval",
		    overrideShellWidgetClass, gui.dpy, args, n);
    n = 0;
#endif

#ifdef FEAT_GUI_MOTIF
    XtSetArg(args[n], XmNforeground, gui.balloonEval_fg_pixel); n++;
    XtSetArg(args[n], XmNbackground, gui.balloonEval_bg_pixel); n++;
    XtSetArg(args[n], XmNfontList, gui.balloonEval_fontList); n++;
    XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
    beval->balloonLabel = XtCreateManagedWidget("balloonLabel",
		    xmLabelWidgetClass, beval->balloonShell, args, n);
#elif FEAT_GUI_ATHENA
    XtSetArg(args[n], XtNforeground, gui.balloonEval_fg_pixel); n++;
    XtSetArg(args[n], XtNbackground, gui.balloonEval_bg_pixel); n++;
    XtSetArg(args[n], XtNinternational, True); n++;
    XtSetArg(args[n], XtNfontSet, gui.balloonEval_fontList); n++;
    beval->balloonLabel = XtCreateManagedWidget("balloonLabel",
		    labelWidgetClass, beval->balloonShell, args, n);
#endif
}

#endif /* FEAT_BEVAL */