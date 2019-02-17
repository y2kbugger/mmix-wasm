extern void new_memory_view(int i);
extern int close_memory_view(int i);
extern int uncheck_memory_view(HWND hWnd);
extern void new_register_view(int i);
extern int close_register_view(int i);
extern int uncheck_register_view(HWND hWnd);
extern void memory_update(void);
extern void regstack_update(void);
extern void debug_init(void);

extern void set_goto_addr(int segment, uint64_t goto_addr);

extern void set_register_inspectors(void);
extern unsigned int show_special_registers;
extern INT_PTR CALLBACK OptionSpecialDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam );
extern INT_PTR CALLBACK OptionDebugDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam );
extern void set_debug_windows(void);
extern int is_inspector(HWND h);
extern void change_mem_font(void);
extern int break_at_Main;
extern int break_after;

#define REG_LOCAL 0
#define REG_GLOBAL 1
#define REG_SPECIAL 2
#define REG_STACK 3


extern int show_debug_local;
extern int show_debug_global;
extern int show_debug_special;
extern int show_debug_regstack;
extern int show_debug_text;
extern int show_debug_data;
extern int show_debug_pool;
extern int show_debug_stack;
extern int show_debug_neg;
extern int show_trace;

extern int missing_app;

#ifdef VMB
extern int auto_connect;
#endif