#ifndef _INDENT_H_
#define _INDENT_H_

extern int max_label_indent;
extern int max_opcode_indent;
extern int max_arg_indent;
extern int use_tab_indent;; /* whether to use tabs or spaces */
extern int use_crlf; /* Use CR LF to end a line */


void indent(int file_no);

#endif