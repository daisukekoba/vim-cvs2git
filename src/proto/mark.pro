/* mark.c */
int setmark __ARGS((int c));
void setpcmark __ARGS((void));
void checkpcmark __ARGS((void));
pos_t *movemark __ARGS((int count));
pos_t *getmark_coladd __ARGS((int c, int changefile));
pos_t *getmark __ARGS((int c, int changefile));
pos_t *getnextmark __ARGS((pos_t *startpos, int dir, int beginline));
void fmarks_check_names __ARGS((buf_t *buf));
int check_mark __ARGS((pos_t *pos));
void clrallmarks __ARGS((buf_t *buf));
char_u *fm_getname __ARGS((fmark_t *fmark, int lead_len));
void do_marks __ARGS((exarg_t *eap));
void ex_jumps __ARGS((exarg_t *eap));
void mark_adjust __ARGS((linenr_t line1, linenr_t line2, long amount, long amount_after));
void copy_jumplist __ARGS((win_t *from, win_t *to));
void free_jumplist __ARGS((win_t *wp));
void set_last_cursor __ARGS((win_t *win));
int read_viminfo_filemark __ARGS((char_u *line, FILE *fp, int force));
void write_viminfo_filemarks __ARGS((FILE *fp));
int removable __ARGS((char_u *name));
int write_viminfo_marks __ARGS((FILE *fp_out));
void copy_viminfo_marks __ARGS((char_u *line, FILE *fp_in, FILE *fp_out, int count, int eof));
/* vim: set ft=c : */
