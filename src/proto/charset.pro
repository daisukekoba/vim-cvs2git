/* charset.c */
int init_chartab __ARGS((void));
int buf_init_chartab __ARGS((buf_t *buf, int global));
void trans_characters __ARGS((char_u *buf, int bufsize));
char_u *transstr __ARGS((char_u *s));
void str_tolower __ARGS((char_u *p));
char_u *transchar __ARGS((int c));
void transchar_nonprint __ARGS((char_u *buf, int c));
void transchar_hex __ARGS((char_u *buf, int c));
int byte2cells __ARGS((int b));
int char2cells __ARGS((int c));
int ptr2cells __ARGS((char_u *p));
int vim_strsize __ARGS((char_u *s));
int chartabsize __ARGS((char_u *p, colnr_t col));
int linetabsize __ARGS((char_u *s));
int win_linetabsize __ARGS((win_t *wp, char_u *p, colnr_t len));
int vim_isIDc __ARGS((int c));
int vim_iswordc __ARGS((int c));
int vim_iswordp __ARGS((char_u *p));
int vim_iswordc_buf __ARGS((char_u *p, buf_t *buf));
int vim_isfilec __ARGS((int c));
int vim_isprintc __ARGS((int c));
int vim_isprintc_strict __ARGS((int c));
int lbr_chartabsize __ARGS((unsigned char *s, colnr_t col));
int lbr_chartabsize_adv __ARGS((char_u **s, colnr_t col));
int win_lbr_chartabsize __ARGS((win_t *wp, unsigned char *s, colnr_t col, int *headp));
int in_win_border __ARGS((win_t *wp, colnr_t vcol));
void getvcol __ARGS((win_t *wp, pos_t *pos, colnr_t *start, colnr_t *cursor, colnr_t *end));
colnr_t getvcol_nolist __ARGS((pos_t *posp));
void getvvcol __ARGS((win_t *wp, pos_t *pos, colnr_t *start, colnr_t *cursor, colnr_t *end));
void getvcols __ARGS((win_t *wp, pos_t *pos1, pos_t *pos2, colnr_t *left, colnr_t *right));
char_u *skipwhite __ARGS((char_u *p));
char_u *skipdigits __ARGS((char_u *p));
int vim_isdigit __ARGS((int c));
char_u *skiptowhite __ARGS((char_u *p));
char_u *skiptowhite_esc __ARGS((char_u *p));
long getdigits __ARGS((char_u **pp));
int vim_isblankline __ARGS((char_u *lbuf));
void vim_str2nr __ARGS((char_u *start, int *hexp, int *len, int dooct, int dohex, long *nptr, unsigned long *unptr));
int hex2nr __ARGS((int c));
int hexhex2nr __ARGS((char_u *p));
int rem_backslash __ARGS((char_u *str));
void backslash_halve __ARGS((char_u *p));
char_u *backslash_halve_save __ARGS((char_u *p));
/* vim: set ft=c : */
