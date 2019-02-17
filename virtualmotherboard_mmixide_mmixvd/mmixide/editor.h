extern HIMAGELIST hFileMarkers;

extern int edit_file_no; /* the file currently in the editor */

extern void init_edit(HINSTANCE hInstance);
extern int ed_setfocus(void);
extern void ed_new(void);
extern int ed_save(void);
extern int ed_save_as(void);
extern int ed_save_changes(int cancel);
extern int ed_save_all(int cancel);
extern int ed_close(void);
extern int ed_close_all(int cancel);

extern void ed_zoom(int fontsize);
extern void ed_set_ascii(void);
extern void ed_set_utf8(void);
extern void set_whitespace(int ws);

extern void  ed_show_line(int line_no);
extern void ed_mark_error(int file_no, int line_no);
extern void ed_refresh_breaks(void);

extern int ed_operation(unsigned int op);
extern void *ed_create_document(void);
extern void ed_release_document(int file_no);
extern void ed_get_document(void);
extern int ed_open(void);

extern int ed_show_tab(int file_no);
extern int ed_add_tab(int file_no);
extern void ed_remove_tab(int file_no);

extern void ed_toggle_break(int bit);
extern void ed_set_break(int file_no, int line_no, unsigned char bits);
extern char * ed_get_instruction(void);

extern void clear_profile(void);
//extern void create_edit(void); /* create the editor pane */