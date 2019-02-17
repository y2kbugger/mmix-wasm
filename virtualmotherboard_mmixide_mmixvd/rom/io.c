extern long int PageCount;
extern char __bss_start;

char *newpage(void)
{ char *p;
  p = &__bss_start + (PageCount<<13); 
  PageCount = PageCount+1;
  return p;
}
