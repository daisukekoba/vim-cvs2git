/* workshop.c */
void workshop_init __ARGS((void));
void workshop_postinit __ARGS((void));
void ex_wsverb __ARGS((exarg_T *eap));
char *workshop_get_editor_name __ARGS((void));
char *workshop_get_editor_version __ARGS((void));
void workshop_load_file __ARGS((char *filename, int line, char *frameid));
void workshop_reload_file __ARGS((char *filename, int line));
void workshop_show_file __ARGS((char *filename));
void workshop_goto_line __ARGS((char *filename, int lineno));
void workshop_front_file __ARGS((char *filename));
void workshop_save_file __ARGS((char *filename));
void workshop_save_files __ARGS((void));
void workshop_quit __ARGS((void));
void workshop_minimize __ARGS((void));
void workshop_maximize __ARGS((void));
void workshop_add_mark_type __ARGS((int index, char *colorspec, char *sign));
void workshop_set_mark __ARGS((char *filename, int lineno, int markId, int idx));
void workshop_change_mark_type __ARGS((char *filename, int markId, int idx));
void workshop_goto_mark __ARGS((char *filename, int markId, char *message));
void workshop_delete_mark __ARGS((char *filename, int markId));
int workshop_get_mark_lineno __ARGS((char *filename, int markId));
void workshop_moved_marks __ARGS((char *filename));
int workshop_get_font_height __ARGS((void));
void workshop_footer_message __ARGS((char *message, int severity));
void workshop_menu_begin __ARGS((char *label));
void workshop_submenu_begin __ARGS((char *label));
void workshop_submenu_end __ARGS((void));
void workshop_menu_item __ARGS((char *label, char *verb, char *accelerator, char *acceleratorText, char *name, char *filepos, char *sensitive));
void workshop_menu_end __ARGS((void));
void workshop_toolbar_begin __ARGS((void));
void workshop_toolbar_end __ARGS((void));
void workshop_toolbar_button __ARGS((char *label, char *verb, char *senseVerb, char *filepos, char *help, char *sense, char *file, char *left));
void workshop_frame_sensitivities __ARGS((VerbSense *vs));
void workshop_set_option __ARGS((char *option, char *value));
void workshop_balloon_mode __ARGS((Boolean on));
void workshop_balloon_delay __ARGS((int delay));
void workshop_show_balloon_tip __ARGS((char *tip));
void workshop_hotkeys __ARGS((Boolean on));
int workshop_get_positions __ARGS((void *clientData, char **filename, int *curLine, int *curCol, int *selStartLine, int *selStartCol, int *selEndLine, int *selEndCol, int *selLength, char **selection));
char *workshop_test_getcurrentfile __ARGS((void));
int workshop_test_getcursorrow __ARGS((void));
int workshop_test_getcursorcol __ARGS((void));
char *workshop_test_getcursorrowtext __ARGS((void));
char *workshop_test_getselectedtext __ARGS((void));
void workshop_save_sensitivity __ARGS((char *filename));
void findYourself __ARGS((char *argv0));
/* vim: set ft=c : */
