#include "libmmixal.h"

extern void create_symtab(void);
extern void symtab_reset(void);
extern void symtab_mark(char *symbol);
extern int symtab_measureitem(LPMEASUREITEMSTRUCT mi);
extern int symtab_drawitem(LPDRAWITEMSTRUCT di);
extern sym_node *find_symbol(char *symbol, int file_no);
extern void update_symtab(void);
extern INT_PTR CALLBACK OptionSymtabDialogProc( HWND hDlg, UINT message, WPARAM wparam, LPARAM lparam );
extern int symtab_locals;
extern int symtab_registers;
extern int symtab_small;
extern HWND	  hSymbolTable;
extern int SymbolContextMenuHandler(int x, int y);