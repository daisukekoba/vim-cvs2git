/* ex_cmds2.c */
void do_debug __ARGS((char_u *cmd));
void ex_debug __ARGS((exarg_T *eap));
void dbg_check_breakpoint __ARGS((exarg_T *eap));
void ex_breakadd __ARGS((exarg_T *eap));
void ex_breakdel __ARGS((exarg_T *eap));
void ex_breaklist __ARGS((exarg_T *eap));
linenr_T dbg_find_breakpoint __ARGS((int file, char_u *fname, linenr_T after));
void dbg_breakpoint __ARGS((char_u *name, linenr_T lnum));
int autowrite __ARGS((buf_T *buf, int forceit));
void autowrite_all __ARGS((void));
int check_changed __ARGS((buf_T *buf, int checkaw, int mult_win, int forceit, int allbuf));
void dialog_changed __ARGS((buf_T *buf, int checkall));
int can_abandon __ARGS((buf_T *buf, int forceit));
int check_changed_any __ARGS((int hidden));
int check_fname __ARGS((void));
int buf_write_all __ARGS((buf_T *buf, int forceit));
void check_arg_idx __ARGS((win_T *win));
void ex_args __ARGS((exarg_T *eap));
void ex_previous __ARGS((exarg_T *eap));
void ex_rewind __ARGS((exarg_T *eap));
void ex_last __ARGS((exarg_T *eap));
void ex_argument __ARGS((exarg_T *eap));
void do_argfile __ARGS((exarg_T *eap, int argn));
void ex_next __ARGS((exarg_T *eap));
void ex_argedit __ARGS((exarg_T *eap));
void ex_argadd __ARGS((exarg_T *eap));
void ex_argdelete __ARGS((exarg_T *eap));
void ex_listdo __ARGS((exarg_T *eap));
int do_source __ARGS((char_u *fname, int check_other, int is_vimrc));
void ex_scriptnames __ARGS((exarg_T *eap));
char_u *get_scriptname __ARGS((scid_T id));
char *fgets_cr __ARGS((char *s, int n, FILE *stream));
char_u *getsourceline __ARGS((int c, void *cookie, int indent));
void ex_scriptencoding __ARGS((exarg_T *eap));
void ex_finish __ARGS((exarg_T *eap));
int source_finished __ARGS((void *cookie));
void ex_checktime __ARGS((exarg_T *eap));
/* vim: set ft=c : */
