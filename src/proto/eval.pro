/* eval.c */
void set_internal_string_var __ARGS((char_u *name, char_u *value));
int eval_to_bool __ARGS((char_u *arg, int *error, char_u **nextcmd, int skip));
char_u *eval_to_string __ARGS((char_u *arg, char_u **nextcmd));
char_u *eval_to_string_safe __ARGS((char_u *arg, char_u **nextcmd));
int eval_to_number __ARGS((char_u *expr));
int eval_foldexpr __ARGS((char_u *arg, int *cp));
void ex_let __ARGS((exarg_t *eap));
void set_context_for_expression __ARGS((expand_t *xp, char_u *arg, cmdidx_t cmdidx));
void ex_call __ARGS((exarg_t *eap));
void ex_unlet __ARGS((exarg_t *eap));
int do_unlet __ARGS((char_u *name));
char_u *get_user_var_name __ARGS((expand_t *xp, int idx));
char_u *get_function_name __ARGS((expand_t *xp, int idx));
char_u *get_expr_name __ARGS((expand_t *xp, int idx));
void set_vim_var_nr __ARGS((int idx, long val));
void set_vcount __ARGS((long count, long count1));
void set_vim_var_string __ARGS((int idx, char_u *val, int len));
char_u *set_cmdarg __ARGS((exarg_t *eap, char_u *oldarg));
char_u *get_var_value __ARGS((char_u *name));
void new_script_vars __ARGS((scid_t id));
void var_init __ARGS((garray_t *gap));
void var_clear __ARGS((garray_t *gap));
void ex_echo __ARGS((exarg_t *eap));
void ex_echohl __ARGS((exarg_t *eap));
void ex_execute __ARGS((exarg_t *eap));
void ex_function __ARGS((exarg_t *eap));
char_u *get_user_func_name __ARGS((expand_t *xp, int idx));
void ex_delfunction __ARGS((exarg_t *eap));
void *save_funccal __ARGS((void));
void restore_funccal __ARGS((void *fc));
void ex_return __ARGS((exarg_t *eap));
char_u *get_func_line __ARGS((int c, void *cookie, int indent));
int func_has_ended __ARGS((void *cookie));
int func_has_abort __ARGS((void *cookie));
int read_viminfo_varlist __ARGS((vir_t *virp, int writing));
void write_viminfo_varlist __ARGS((FILE *fp));
int store_session_globals __ARGS((FILE *fd));
int eval_charconvert __ARGS((char_u *enc_from, char_u *enc_to, char_u *fname_from, char_u *fname_to));
int modify_fname __ARGS((char_u *src, int *usedlen, char_u **fnamep, char_u **bufp, int *fnamelen));
char_u *do_string_sub __ARGS((char_u *str, char_u *pat, char_u *sub, char_u *flags));
/* vim: set ft=c : */
