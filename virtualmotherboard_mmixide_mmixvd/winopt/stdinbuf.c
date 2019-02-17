#include <windows.h>
#include "winopt.h"

char stdin_buf[256]={0}; /* standard input to the simulated program */
char *stdin_buf_start=stdin_buf; /* current position in that buffer */
char *stdin_buf_end=stdin_buf; /* current end of that buffer */
