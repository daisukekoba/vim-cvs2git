/* option.c */
void set_init_1 __ARGS((void));
void set_string_default __ARGS((char *name, char_u *val));
void set_number_default __ARGS((char *name, long val));
void set_init_2 __ARGS((void));
void set_init_3 __ARGS((void));
void init_gui_options __ARGS((void));
void set_title_defaults __ARGS((void));
int do_set __ARGS((char_u *arg, int opt_flags));
void set_options_bin __ARGS((int oldval, int newval, int global));
int get_viminfo_parameter __ARGS((int type));
char_u *find_viminfo_parameter __ARGS((int type));
void check_options __ARGS((void));
void check_buf_options __ARGS((buf_t *buf));
void free_string_option __ARGS((char_u *p));
void clear_string_option __ARGS((char_u **pp));
void set_term_option_alloced __ARGS((char_u **p));
void set_string_option_direct __ARGS((char_u *name, int opt_idx, char_u *val, int opt_flags));
char_u *check_stl_option __ARGS((char_u *s));
int get_option_value __ARGS((char_u *name, long *numval, char_u **stringval));
void set_option_value __ARGS((char_u *name, long number, char_u *string, int local));
char_u *get_term_code __ARGS((char_u *tname));
char_u *get_highlight_default __ARGS((void));
int makeset __ARGS((FILE *fd, int opt_flags));
void clear_termoptions __ARGS((void));
void set_term_defaults __ARGS((void));
void comp_col __ARGS((void));
char_u *get_equalprg __ARGS((void));
void win_copy_options __ARGS((win_t *wp_from, win_t *wp_to));
void copy_winopt __ARGS((winopt_t *from, winopt_t *to));
void check_win_options __ARGS((win_t *win));
void check_winopt __ARGS((winopt_t *wop));
void clear_winopt __ARGS((winopt_t *wop));
void buf_copy_options __ARGS((buf_t *buf, int flags));
void set_context_in_set_cmd __ARGS((expand_t *xp, char_u *arg, int opt_flags));
int ExpandSettings __ARGS((expand_t *xp, regmatch_t *regmatch, int *num_file, char_u ***file));
int ExpandOldSetting __ARGS((int *num_file, char_u ***file));
int has_format_option __ARGS((int x));
int shortmess __ARGS((int x));
void vimrc_found __ARGS((void));
void change_compatible __ARGS((int on));
int option_was_set __ARGS((char_u *name));
int can_bs __ARGS((int what));
void save_file_ff __ARGS((buf_t *buf));
int file_ff_differs __ARGS((buf_t *buf));
int check_ff_value __ARGS((char_u *p));
/* vim: set ft=c : */
