/* menu.c */
void ex_menu __ARGS((exarg_t *eap));
char_u *set_context_in_menu_cmd __ARGS((char_u *cmd, char_u *arg, int forceit));
char_u *get_menu_name __ARGS((int idx));
char_u *get_menu_names __ARGS((int idx));
char_u *menu_name_skip __ARGS((char_u *name));
int get_menu_index __ARGS((vimmenu_t *menu, int state));
int menu_is_menubar __ARGS((char_u *name));
int menu_is_popup __ARGS((char_u *name));
int menu_is_toolbar __ARGS((char_u *name));
int menu_is_separator __ARGS((char_u *name));
void gui_create_initial_menus __ARGS((vimmenu_t *menu, vimmenu_t *parent));
void gui_update_menus __ARGS((int modes));
int gui_is_menu_shortcut __ARGS((int key));
void gui_show_popupmenu __ARGS((void));
void gui_mch_toggle_tearoffs __ARGS((int enable));
void execute_menu __ARGS((exarg_t *eap));
vimmenu_t *gui_find_menu __ARGS((char_u *path_name));
void ex_menutrans __ARGS((exarg_t *eap));
