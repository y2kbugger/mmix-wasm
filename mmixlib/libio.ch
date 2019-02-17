
@x
@<Preprocessor macros@>@;
@<Type definitions@>@;
@<External subroutines@>@;
@<Global variables@>@;
@<Subroutines@>@;
@y
@<Preprocessor macros@>@;
#ifdef WIN32
#pragma warning(disable : 4996 4267)
#endif
#include "libconfig.h"
@<Type definitions@>@;
@<External subroutines@>@;
@<Global variables@>@;
#include "libimport.h"
@<Subroutines@>@;
@z

@x
    if (fwrite(buf,1,n,sfile[handle].fp)!=n) return ominus(zero_octa,size);
    fflush(sfile[handle].fp);
@y
#ifdef MMIX_PRINT
    if (sfile[handle].fp==stdout || sfile[handle].fp==stderr)
      win32_log(buf);
    else
#endif
    { if (fwrite(buf,1,n,sfile[handle].fp)!=n) return ominus(zero_octa,size);
      fflush(sfile[handle].fp);
    }
@z

@x
    if (fwrite(buf,1,n,sfile[handle].fp)!=n) return neg_one;
@y
#ifdef MMIX_PRINT
    if (sfile[handle].fp==stdout || sfile[handle].fp==stderr)
      win32_log(buf);
    else
#endif
    if (fwrite(buf,1,n,sfile[handle].fp)!=n) return neg_one;
@z

@x
      fflush(sfile[handle].fp);
@y
#ifdef MMIX_PRINT
    if (sfile[handle].fp!=stdout && sfile[handle].fp!=stderr)
#endif
      fflush(sfile[handle].fp);
@z

@x
    if (fwrite(buf,1,n,sfile[handle].fp)!=n) return neg_one;
@y
#ifdef MMIX_PRINT
    if (sfile[handle].fp==stdout || sfile[handle].fp==stderr)
      win32_log(buf);
    else
#endif
    if (fwrite(buf,1,n,sfile[handle].fp)!=n) return neg_one;
@z

@x
      fflush(sfile[handle].fp);
@y
#ifdef MMIX_PRINT
    if (sfile[handle].fp!=stdout && sfile[handle].fp!=stderr)
#endif
      fflush(sfile[handle].fp);
@z
