/* screen.c */
void redraw_later __ARGS((int type));
void redraw_later_clear __ARGS((void));
void redraw_all_later __ARGS((int type));
void redraw_curbuf_later __ARGS((int type));
void redraw_buf_later __ARGS((int type, buf_t *buf));
void redrawWinline __ARGS((linenr_t lnum, int invalid));
void update_curbuf __ARGS((int type));
void update_screen __ARGS((int type));
void update_debug_sign __ARGS((buf_t *buf, linenr_t lnum));
void updateWindow __ARGS((win_t *wp));
void status_redraw_all __ARGS((void));
void redraw_statuslines __ARGS((void));
void win_redr_status_matches __ARGS((expand_t *xp, int num_matches, char_u **matches, int match));
void win_redr_status __ARGS((win_t *wp));
int build_stl_str __ARGS((win_t *wp, char_u *out, char_u *fmt, int fillchar, int maxlen));
void screen_putchar __ARGS((int c, int row, int col, int attr));
int screen_getchar __ARGS((int row, int col, int *attrp));
void screen_puts __ARGS((char_u *text, int row, int col, int attr));
void screen_start __ARGS((void));
void screen_down __ARGS((void));
void screen_stop_highlight __ARGS((void));
void reset_cterm_colors __ARGS((void));
void screen_fill __ARGS((int start_row, int end_row, int start_col, int end_col, int c1, int c2, int attr));
void screenalloc __ARGS((int clear));
void screenclear __ARGS((void));
void update_topline_redraw __ARGS((void));
void update_topline __ARGS((void));
void update_curswant __ARGS((void));
void windgoto __ARGS((int row, int col));
void setcursor __ARGS((void));
void scroll_cursor_top __ARGS((int min_scroll, int always));
void scroll_cursor_bot __ARGS((int min_scroll, int set_topbot));
void scroll_cursor_halfway __ARGS((int atend));
void cursor_correct __ARGS((void));
void changed_window_setting __ARGS((void));
void changed_cline_bef_curs __ARGS((void));
void changed_cline_bef_curs_win __ARGS((win_t *wp));
void changed_line_abv_curs __ARGS((void));
void changed_line_abv_curs_win __ARGS((win_t *wp));
void set_topline __ARGS((win_t *wp, linenr_t lnum));
void validate_botline __ARGS((void));
void validate_botline_win __ARGS((win_t *wp));
void invalidate_botline __ARGS((void));
void invalidate_botline_win __ARGS((win_t *wp));
void approximate_botline_win __ARGS((win_t *wp));
int cursor_valid __ARGS((void));
void validate_cursor __ARGS((void));
void validate_cline_row __ARGS((void));
void validate_virtcol __ARGS((void));
void validate_cheight __ARGS((void));
void validate_cursor_col __ARGS((void));
int win_col_off __ARGS((win_t *wp));
int curwin_col_off __ARGS((void));
int win_col_off2 __ARGS((win_t *wp));
int curwin_col_off2 __ARGS((void));
void curs_columns __ARGS((int scroll));
void scrolldown __ARGS((long line_count));
void scrollup __ARGS((long line_count));
void scrolldown_clamp __ARGS((void));
void scrollup_clamp __ARGS((void));
int win_ins_lines __ARGS((win_t *wp, int row, int line_count, int invalid, int mayclear));
int win_del_lines __ARGS((win_t *wp, int row, int line_count, int invalid, int mayclear));
void win_rest_invalid __ARGS((win_t *wp));
int screen_del_lines __ARGS((int off, int row, int line_count, int end, int force));
int showmode __ARGS((void));
void unshowmode __ARGS((int force));
void showruler __ARGS((int always));
void check_for_delay __ARGS((int check_msg_scroll));
int screen_valid __ARGS((int clear));
int jump_to_mouse __ARGS((int flags, int *inclusive));
win_t *mouse_find_win __ARGS((int *rowp, int *colp));
int get_fpos_of_mouse __ARGS((pos_t *mpos));
int redrawing __ARGS((void));
int messaging __ARGS((void));
int onepage __ARGS((int dir, long count));
void halfpage __ARGS((int flag, linenr_t Prenum));
void ex_intro __ARGS((exarg_t *eap));
