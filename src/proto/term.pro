/* term.c */
int set_termname __ARGS((char_u *term));
void set_mouse_termcode __ARGS((int n, char_u *s));
void del_mouse_termcode __ARGS((int n));
void getlinecol __ARGS((void));
int add_termcap_entry __ARGS((char_u *name, int force));
int term_is_8bit __ARGS((char_u *name));
int term_is_gui __ARGS((char_u *name));
char_u *tltoa __ARGS((unsigned long i));
void termcapinit __ARGS((char_u *name));
void out_flush __ARGS((void));
void out_trash __ARGS((void));
void out_char __ARGS((unsigned c));
void out_str_nf __ARGS((char_u *s));
void out_str __ARGS((char_u *s));
void term_windgoto __ARGS((int row, int col));
void term_cursor_right __ARGS((int i));
void term_append_lines __ARGS((int line_count));
void term_delete_lines __ARGS((int line_count));
void term_set_winpos __ARGS((int x, int y));
void term_set_winsize __ARGS((int width, int height));
void term_fg_color __ARGS((int n));
void term_bg_color __ARGS((int n));
void term_settitle __ARGS((char_u *title));
void ttest __ARGS((int pairs));
void add_long_to_buf __ARGS((long_u val, char_u *dst));
int get_long_from_buf __ARGS((char_u *buf, long_u *val));
void check_winsize __ARGS((void));
void set_winsize __ARGS((int width, int height, int mustset));
void settmode __ARGS((int tmode));
void starttermcap __ARGS((void));
void stoptermcap __ARGS((void));
int swapping_screen __ARGS((void));
void setmouse __ARGS((void));
int mouse_has __ARGS((int c));
int mouse_model_popup __ARGS((void));
void scroll_start __ARGS((void));
void cursor_on __ARGS((void));
void cursor_off __ARGS((void));
void scroll_region_set __ARGS((WIN *wp, int off));
void scroll_region_reset __ARGS((void));
void clear_termcodes __ARGS((void));
void add_termcode __ARGS((char_u *name, char_u *string, int use_8bit));
char_u *find_termcode __ARGS((char_u *name));
char_u *get_termcode __ARGS((int i));
void del_termcode __ARGS((char_u *name));
int check_termcode __ARGS((int max_offset, char_u *buf, int buflen));
char_u *replace_termcodes __ARGS((char_u *from, char_u **bufp, int from_part, int do_lt));
int find_term_bykeys __ARGS((char_u *src));
void show_termcodes __ARGS((void));
int show_one_termcode __ARGS((char_u *name, char_u *code, int printit));
char_u *translate_mapping __ARGS((char_u *str, int expmap));
void update_tcap __ARGS((int attr));
