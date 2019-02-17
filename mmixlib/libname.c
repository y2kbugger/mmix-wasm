#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include "libconfig.h"
#include "libtype.h"
#include "libglobals.h"
#include <setjmp.h>
#include "libname.h"
#include "libimport.h"

extern jmp_buf mmix_exit;
/* simple implementation of the identity mapping */

#define NUM_FILES 0x100
static char *filenames[NUM_FILES]={NULL};
static char file_no_bound= 0;

int filename2file(char *filename)
{ int file_no;
  file_no=0;
  while(file_no<file_no_bound)
  { if (filenames[file_no]!=NULL && strcmp(filenames[file_no],filename)==0)
      return file_no;
    file_no++;
  }
  if (file_no>=NUM_FILES)
  { MMIX_ERROR("Too many open files (%s)!\n",filename);  
    return -1;
  }
  filenames[file_no]=malloc(strlen(filename)+1);
  if (filenames[file_no]==NULL)
  { MMIX_ERROR("Out of memory for filename (%s)!\n",filename);  
    return -1;
  }
  strcpy(filenames[file_no],filename);
  if (file_no>=file_no_bound) file_no_bound= file_no+1;
  return file_no;
}

char *file2filename(int file_no)
{ 
  if (file_no<0 || file_no>=NUM_FILES) 
	   return NULL;
   else
	   return filenames[file_no];
}