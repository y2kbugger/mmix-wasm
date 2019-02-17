#include <windows.h>
#include <stdio.h>
#include "winopt.h"
#include "winmain.h"
#include "edit.h"
#include "assembler.h"
#include "debug.h"
#include "sources.h"
#include "libmmixal.h"
#include "symtab.h"
#include "runoptions.h"
#include "findreplace.h"
#include "indent.h"
#define STATIC_BUILD
#include "../scintilla/include/scilexer.h"

regtable regtab= {
	{"xpos",&xpos,TYPE_DWORD},
	{"ypos",&ypos,TYPE_DWORD},
	{"width",&width,TYPE_DWORD},
	{"height",&height,TYPE_DWORD},
	/* editor options */
	{"tabwidth",&tabwidth,TYPE_DWORD},
	{KEY_FLAGS ,&autosave,0},
	{KEY_FLAGS ,&show_line_no,1},
	{KEY_FLAGS ,&show_profile,2},
	{KEY_FLAGS ,&syntax_highlighting,3},
	{KEY_FLAGS ,&show_whitespace,4},
	{"fontsize",&fontsize,TYPE_DWORD},
	{"codepage",&codepage,TYPE_DWORD},

	{"opcolor",&syntax_color[SCE_MMIXAL_OPCODE_VALID],TYPE_DWORD},
	{"errcolor",&syntax_color[SCE_MMIXAL_OPCODE_UNKNOWN],TYPE_DWORD},
	{"regcolor",&syntax_color[SCE_MMIXAL_REGISTER],TYPE_DWORD},
	{"symcolor",&syntax_color[SCE_MMIXAL_SYMBOL],TYPE_DWORD},
	{"commentcolor",&syntax_color[SCE_MMIXAL_COMMENT],TYPE_DWORD},

	/* run options */
	{"stdin",&stdin_file,TYPE_STRING},
	{"args",&run_args,TYPE_STRING},

    /* assembler options */
	{"boption",&buf_size,TYPE_DWORD},
	{KEY_FLAGS ,&expanding,5},
	{KEY_FLAGS ,&l_option,6},
	{KEY_FLAGS ,&auto_assemble,7},

	/*debugger options */
	{KEY_FLAGS,&break_at_Main,8},
	{KEY_FLAGS,&break_after,9},
	{KEY_FLAGS,&show_debug_local,10},
	{KEY_FLAGS,&show_debug_global,11},
	{KEY_FLAGS,&show_debug_special,12},
	{KEY_FLAGS,&show_debug_regstack,13},
	{KEY_FLAGS,&show_debug_text,14},
	{KEY_FLAGS,&show_debug_data,15},
	{KEY_FLAGS,&show_debug_pool,16},
	{KEY_FLAGS,&show_debug_neg,17},
	{KEY_FLAGS,&show_trace,18},
	{"traceex",&tracing_exceptions,TYPE_DWORD},
	{"showspecial",&show_special_registers,TYPE_DWORD},
#ifdef VMB
	{KEY_FLAGS,&show_operating_system,19},
#endif
	{KEY_FLAGS,&stack_tracing,20},
#ifdef VMB
	{KEY_FLAGS,&auto_connect,21},
#endif
    /* symbol table */
	{KEY_FLAGS,&symtab_locals,22},
	{KEY_FLAGS,&symtab_registers,23},
	{KEY_FLAGS,&symtab_small,24},

	{KEY_FLAGS,&missing_app,25},
	{KEY_FLAGS,&load_single_file,26},
	{KEY_FLAGS,&find_escape,27},
	{KEY_FLAGS ,&auto_close_errors,28},
	{"traceexceptions",&tracing_exceptions,TYPE_DWORD},
	{KEY_FLAGS ,&warn_as_error,29},
	{KEY_FLAGS ,&use_tab_indent,30},
	{KEY_FLAGS ,&use_crlf,31},
	{KEY_FLAGS ,&check_X_BIT,32},
	{KEY_FLAGS ,&showing_stats,33},

	{"maxlabel",&max_label_indent,TYPE_DWORD},
	{"maxop",&max_opcode_indent,TYPE_DWORD},
	{"maxarg",&max_arg_indent,TYPE_DWORD},

	{NULL,NULL,0}};
