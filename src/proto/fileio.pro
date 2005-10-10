/* fileio.c */
extern void filemess __ARGS((buf_T *buf, char_u *name, char_u *s, int attr));
extern int readfile __ARGS((char_u *fname, char_u *sfname, linenr_T from, linenr_T lines_to_skip, linenr_T lines_to_read, exarg_T *eap, int flags));
extern int prep_exarg __ARGS((exarg_T *eap, buf_T *buf));
extern int buf_write __ARGS((buf_T *buf, char_u *fname, char_u *sfname, linenr_T start, linenr_T end, exarg_T *eap, int append, int forceit, int reset_changed, int filtering));
extern char_u *shorten_fname __ARGS((char_u *full_path, char_u *dir_name));
extern void shorten_fnames __ARGS((int force));
extern void shorten_filenames __ARGS((char_u **fnames, int count));
extern char_u *modname __ARGS((char_u *fname, char_u *ext, int prepend_dot));
extern char_u *buf_modname __ARGS((int shortname, char_u *fname, char_u *ext, int prepend_dot));
extern int vim_fgets __ARGS((char_u *buf, int size, FILE *fp));
extern int tag_fgets __ARGS((char_u *buf, int size, FILE *fp));
extern int vim_rename __ARGS((char_u *from, char_u *to));
extern int check_timestamps __ARGS((int focus));
extern int buf_check_timestamp __ARGS((buf_T *buf, int focus));
extern void buf_store_time __ARGS((buf_T *buf, struct stat *st, char_u *fname));
extern void write_lnum_adjust __ARGS((linenr_T offset));
extern void vim_deltempdir __ARGS((void));
extern char_u *vim_tempname __ARGS((int extra_char));
extern void forward_slash __ARGS((char_u *fname));
extern void do_augroup __ARGS((char_u *arg, int del_group));
extern int check_ei __ARGS((void));
extern void do_autocmd __ARGS((char_u *arg, int forceit));
extern int do_doautocmd __ARGS((char_u *arg, int do_msg));
extern void ex_doautoall __ARGS((exarg_T *eap));
extern void aucmd_prepbuf __ARGS((aco_save_T *aco, buf_T *buf));
extern void aucmd_restbuf __ARGS((aco_save_T *aco));
extern int apply_autocmds __ARGS((EVENT_T event, char_u *fname, char_u *fname_io, int force, buf_T *buf));
extern int apply_autocmds_retval __ARGS((EVENT_T event, char_u *fname, char_u *fname_io, int force, buf_T *buf, int *retval));
extern int has_cursorhold __ARGS((void));
extern int has_autocmd __ARGS((EVENT_T event, char_u *sfname));
extern char_u *get_augroup_name __ARGS((expand_T *xp, int idx));
extern char_u *set_context_in_autocmd __ARGS((expand_T *xp, char_u *arg, int doautocmd));
extern char_u *get_event_name __ARGS((expand_T *xp, int idx));
extern int au_exists __ARGS((char_u *name, char_u *name_end, char_u *pattern));
extern int match_file_pat __ARGS((char_u *pattern, char_u *fname, char_u *sfname, char_u *tail, int allow_dirs));
extern int match_file_list __ARGS((char_u *list, char_u *sfname, char_u *ffname));
extern char_u *file_pat_to_reg_pat __ARGS((char_u *pat, char_u *pat_end, char *allow_dirs, int no_bslash));
/* vim: set ft=c : */
