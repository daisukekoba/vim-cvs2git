/* ex_cmds.c */
void do_ascii __ARGS((exarg_T *eap));
void ex_align __ARGS((exarg_T *eap));
void ex_retab __ARGS((exarg_T *eap));
int do_move __ARGS((linenr_T line1, linenr_T line2, linenr_T dest));
void ex_copy __ARGS((linenr_T line1, linenr_T line2, linenr_T n));
void do_bang __ARGS((int addr_count, exarg_T *eap, int forceit, int do_in, int do_out));
void do_shell __ARGS((char_u *cmd, int flags));
char_u *make_filter_cmd __ARGS((char_u *cmd, char_u *itmp, char_u *otmp));
void append_redir __ARGS((char_u *buf, char_u *fname));
int viminfo_error __ARGS((char *message, char_u *line));
int read_viminfo __ARGS((char_u *file, int want_info, int want_marks, int forceit));
void write_viminfo __ARGS((char_u *file, int forceit));
int viminfo_readline __ARGS((vir_T *virp));
char_u *viminfo_readstring __ARGS((vir_T *virp, int off, int convert));
void viminfo_writestring __ARGS((FILE *fd, char_u *p));
void do_fixdel __ARGS((exarg_T *eap));
void print_line_no_prefix __ARGS((linenr_T lnum, int use_number));
void print_line __ARGS((linenr_T lnum, int use_number));
void ex_file __ARGS((exarg_T *eap));
void ex_update __ARGS((exarg_T *eap));
void ex_write __ARGS((exarg_T *eap));
int do_write __ARGS((exarg_T *eap));
void ex_wnext __ARGS((exarg_T *eap));
void do_wqall __ARGS((exarg_T *eap));
int not_writing __ARGS((void));
int getfile __ARGS((int fnum, char_u *ffname, char_u *sfname, int setpm, linenr_T lnum, int forceit));
int do_ecmd __ARGS((int fnum, char_u *ffname, char_u *sfname, exarg_T *eap, linenr_T newlnum, int flags));
void ex_append __ARGS((exarg_T *eap));
void ex_change __ARGS((exarg_T *eap));
void ex_z __ARGS((exarg_T *eap));
int check_restricted __ARGS((void));
int check_secure __ARGS((void));
void do_sub __ARGS((exarg_T *eap));
void ex_global __ARGS((exarg_T *eap));
void global_exe __ARGS((char_u *cmd));
int read_viminfo_sub_string __ARGS((vir_T *virp, int force));
void write_viminfo_sub_string __ARGS((FILE *fp));
void prepare_tagpreview __ARGS((void));
void ex_help __ARGS((exarg_T *eap));
int help_heuristic __ARGS((char_u *matched_string, int offset, int wrong_case));
int find_help_tags __ARGS((char_u *arg, int *num_matches, char_u ***matches));
void fix_help_buffer __ARGS((void));
void ex_helptags __ARGS((exarg_T *eap));
void ex_sign __ARGS((exarg_T *eap));
void ex_unsign __ARGS((exarg_T *eap));
/* vim: set ft=c : */
