/* gui_motif.c */
void gui_x11_create_widgets __ARGS((void));
void gui_x11_destroy_widgets __ARGS((void));
void gui_mch_set_text_area_pos __ARGS((int x, int y, int w, int h));
void gui_x11_set_back_color __ARGS((void));
XmFontList gui_motif_create_fontlist __ARGS((XFontStruct *font));
XmFontList gui_motif_fontset2fontlist __ARGS((XFontSet *fontset));
void gui_mch_enable_menu __ARGS((int flag));
void gui_motif_set_mnemonics __ARGS((int enable));
void gui_mch_add_menu __ARGS((vimmenu_T *menu, int idx));
void gui_mch_toggle_tearoffs __ARGS((int enable));
int gui_mch_text_area_extra_height __ARGS((void));
void gui_mch_compute_menu_height __ARGS((Widget id));
void gui_mch_add_menu_item __ARGS((vimmenu_T *menu, int idx));
void gui_motif_update_mousemodel __ARGS((vimmenu_T *menu));
void gui_mch_new_menu_colors __ARGS((void));
void gui_mch_new_menu_font __ARGS((void));
void gui_mch_new_tooltip_font __ARGS((void));
void gui_mch_new_tooltip_colors __ARGS((void));
void gui_mch_destroy_menu __ARGS((vimmenu_T *menu));
void gui_mch_show_popupmenu __ARGS((vimmenu_T *menu));
void gui_mch_def_colors __ARGS((void));
void gui_mch_set_scrollbar_thumb __ARGS((scrollbar_T *sb, long val, long size, long max));
void gui_mch_set_scrollbar_pos __ARGS((scrollbar_T *sb, int x, int y, int w, int h));
void gui_mch_enable_scrollbar __ARGS((scrollbar_T *sb, int flag));
void gui_mch_create_scrollbar __ARGS((scrollbar_T *sb, int orient));
void gui_mch_destroy_scrollbar __ARGS((scrollbar_T *sb));
void gui_mch_set_scrollbar_colors __ARGS((scrollbar_T *sb));
Window gui_x11_get_wid __ARGS((void));
char_u *gui_mch_browse __ARGS((int saving, char_u *title, char_u *dflt, char_u *ext, char_u *initdir, char_u *filter));
int gui_mch_dialog __ARGS((int type, char_u *title, char_u *message, char_u *button_names, int dfltbutton, char_u *textfield));
void gui_mch_enable_footer __ARGS((int showit));
void gui_mch_set_footer __ARGS((char_u *s));
void gui_mch_show_toolbar __ARGS((int showit));
void gui_mch_reset_focus __ARGS((void));
int gui_mch_compute_toolbar_height __ARGS((void));
void gui_mch_get_toolbar_colors __ARGS((Pixel *bgp, Pixel *fgp, Pixel *bsp, Pixel *tsp, Pixel *hsp));
void gui_mch_find_dialog __ARGS((exarg_T *eap));
void gui_mch_replace_dialog __ARGS((exarg_T *eap));
/* vim: set ft=c : */
