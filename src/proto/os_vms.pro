/* os_vms.c */
void mch_write __ARGS((char_u *s, int len));
int mch_inchar __ARGS((char_u *buf, int maxlen, long wtime));
int mch_char_avail __ARGS((void));
void mch_delay __ARGS((long msec, int ignoreinput));
void mch_resize __ARGS((void));
void mch_suspend __ARGS((void));
void mch_shellinit __ARGS((void));
void reset_signals __ARGS((void));
int mch_check_win __ARGS((int argc, char **argv));
int mch_check_input __ARGS((void));
int mch_can_restore_title __ARGS((void));
int mch_can_restore_icon __ARGS((void));
void mch_settitle __ARGS((char_u *title, char_u *icon));
void mch_restore_title __ARGS((int which));
int vim_is_xterm __ARGS((char_u *name));
int vim_is_iris __ARGS((char_u *name));
int vim_is_vt300 __ARGS((char_u *name));
int vim_is_fastterm __ARGS((char_u *name));
int mch_get_user_name __ARGS((char_u *s, int len));
void mch_get_host_name __ARGS((char_u *s, int len));
long mch_get_pid __ARGS((void));
int mch_dirname __ARGS((char_u *buf, int len));
int mch_FullName __ARGS((char_u *fname, char_u *buf, int len, int force));
int mch_isFullName __ARGS((char_u *fname));
long getperm __ARGS((char_u *name));
int mch_setperm __ARGS((char_u *name, long perm));
void mch_hide __ARGS((char_u *name));
int mch_isdir __ARGS((char_u *name));
int mch_can_exe __ARGS((char_u *name));
int mch_nodetype __ARGS((char_u *name));
void mch_init __ARGS((void));
void mch_windexit __ARGS((int r));
void mch_settmode __ARGS((int raw));
void vul_desc __ARGS((DESC *des, char *str));
int get_tt_width __ARGS((char *dev));
void get_stty __ARGS((void));
void mch_setmouse __ARGS((int on));
void check_mouse_termcode __ARGS((void));
int use_xterm_mouse __ARGS((void));
int use_dec_mouse __ARGS((void));
int mch_screenmode __ARGS((char_u *arg));
int mch_get_shellsize __ARGS((void));
void mch_set_shellsize __ARGS((void));
void mch_new_shellsize __ARGS((void));
int mch_call_shell __ARGS((char_u *cmd, int options));
void mch_breakcheck __ARGS((void));
int RealWaitForChar __ARGS((int fd, long msec));
int vms_wproc __ARGS((char *name, int type));
int mch_expand_wildcards __ARGS((int num_pat, char_u **pat, int *num_file, char_u ***file, int flags));
int mch_has_wildcard __ARGS((char_u *p));
char_u *mch_getenv __ARGS((char_u *lognam));
int mch_setenv __ARGS((char *var, char *value, int x));
long mch_getperm __ARGS((char_u *name));
int mch_input_isatty __ARGS((void));
int mch_expandpath __ARGS((garray_t *gap, char_u *path, int flags));
void *vms_fixfilename __ARGS((void *instring));
void *vms_remove_version __ARGS((void *fname));
