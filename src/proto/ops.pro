/* ops.c */
int get_op_type __ARGS((int char1, int char2));
int op_on_lines __ARGS((int op));
int get_op_char __ARGS((int optype));
int get_extra_op_char __ARGS((int optype));
void op_shift __ARGS((oparg_t *oap, int curs_top, int amount));
void shift_line __ARGS((int left, int round, int amount));
void op_reindent __ARGS((oparg_t *oap, int (*how)(void)));
int get_expr_register __ARGS((void));
void set_expr_line __ARGS((char_u *new_line));
int valid_yank_reg __ARGS((int regname, int writing));
int yank_register_mline __ARGS((int regname));
int do_record __ARGS((int c));
int do_execreg __ARGS((int regname, int colon, int addcr));
int insert_reg __ARGS((int regname, int literally));
int cmdline_paste __ARGS((int regname, int literally));
int op_delete __ARGS((oparg_t *oap));
int op_replace __ARGS((oparg_t *oap, int c));
void op_tilde __ARGS((oparg_t *oap));
int swapchar __ARGS((int op_type, pos_t *pos));
void op_insert __ARGS((oparg_t *oap, long count1));
int op_change __ARGS((oparg_t *oap));
void init_yank __ARGS((void));
int op_yank __ARGS((oparg_t *oap, int deleting, int mess));
void do_put __ARGS((int regname, int dir, long count, int flags));
int preprocs_left __ARGS((void));
int get_register_name __ARGS((int num));
void ex_display __ARGS((exarg_t *eap));
void dis_msg __ARGS((char_u *p, int skip_esc));
void do_do_join __ARGS((long count, int insert_space));
int do_join __ARGS((int insert_space));
void op_format __ARGS((oparg_t *oap));
int do_addsub __ARGS((int command, linenr_t Prenum1));
int read_viminfo_register __ARGS((vir_t *virp, int force));
void write_viminfo_registers __ARGS((FILE *fp));
void x11_export_final_selection __ARGS((void));
void clip_free_selection __ARGS((VimClipboard *cbd));
void clip_get_selection __ARGS((VimClipboard *cbd));
void clip_yank_selection __ARGS((int type, char_u *str, long len, VimClipboard *cbd));
int clip_convert_selection __ARGS((char_u **str, long_u *len, VimClipboard *cbd));
char_u *get_reg_contents __ARGS((int regname));
void write_reg_contents __ARGS((int name, char_u *str, int must_append));
void clear_oparg __ARGS((oparg_t *oap));
void cursor_pos_info __ARGS((void));
/* vim: set ft=c : */
