/* ex_docmd.c */
void do_exmode __ARGS((int improved));
int do_cmdline __ARGS((char_u *cmdline, char_u *(*getline)(int, void *, int), void *cookie, int flags));
int cmd_exists __ARGS((char_u *name));
void handle_swap_exists __ARGS((buf_t *old_curbuf));
char_u *set_one_cmd_context __ARGS((expand_t *xp, char_u *buff));
char_u *skip_range __ARGS((char_u *cmd, int *ctx));
int expand_filename __ARGS((exarg_t *eap, char_u **cmdlinep, char_u **errormsgp));
void separate_nextcmd __ARGS((exarg_t *eap));
int autowrite __ARGS((buf_t *buf, int forceit));
void autowrite_all __ARGS((void));
int check_changed __ARGS((buf_t *buf, int checkaw, int mult_win, int forceit, int allbuf));
void dialog_changed __ARGS((buf_t *buf, int checkall));
int can_abandon __ARGS((buf_t *buf, int forceit));
int check_changed_any __ARGS((int hidden));
int check_fname __ARGS((void));
int buf_write_all __ARGS((buf_t *buf, int forceit));
int rem_backslash __ARGS((char_u *str));
void backslash_halve __ARGS((char_u *p));
char_u *backslash_halve_save __ARGS((char_u *p));
void check_arg_idx __ARGS((win_t *win));
int ends_excmd __ARGS((int c));
char_u *find_nextcmd __ARGS((char_u *p));
char_u *check_nextcmd __ARGS((char_u *p));
int do_source __ARGS((char_u *fname, int check_other, int is_vimrc));
char *fgets_cr __ARGS((char *s, int n, FILE *stream));
char_u *getsourceline __ARGS((int c, void *cookie, int indent));
char_u *get_command_name __ARGS((expand_t *xp, int idx));
void uc_clear __ARGS((garray_t *gap));
char_u *get_user_commands __ARGS((expand_t *xp, int idx));
char_u *get_user_cmd_flags __ARGS((expand_t *xp, int idx));
char_u *get_user_cmd_nargs __ARGS((expand_t *xp, int idx));
char_u *get_user_cmd_complete __ARGS((expand_t *xp, int idx));
void not_exiting __ARGS((void));
void do_argfile __ARGS((exarg_t *eap, int argn));
void handle_drop __ARGS((int filec, char_u **filev, int split));
void alist_init __ARGS((alist_t *al));
void alist_unlink __ARGS((alist_t *al));
void alist_new __ARGS((void));
void alist_expand __ARGS((void));
void alist_set __ARGS((alist_t *al, int count, char_u **files, int use_curbuf));
void alist_add __ARGS((alist_t *al, char_u *fname, int set_fnum));
void alist_slash_adjust __ARGS((void));
char_u *eval_vars __ARGS((char_u *src, int *usedlen, linenr_t *lnump, char_u **errormsg, char_u *srcstart));
char_u *expand_sfile __ARGS((char_u *arg));
int put_eol __ARGS((FILE *fd));
int put_line __ARGS((FILE *fd, char *s));
void cmd_runtime __ARGS((char_u *name, int all));
void dialog_msg __ARGS((char_u *buff, char *format, char_u *fname));
void set_lang_var __ARGS((void));
linenr_t dbg_find_breakpoint __ARGS((int file, char_u *fname, linenr_t after));
void dbg_breakpoint __ARGS((char_u *name, linenr_t lnum));
/* vim: set ft=c : */
