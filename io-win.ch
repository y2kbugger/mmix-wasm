
@x
#include <stdio.h>
#include <stdlib.h>
@y
#include <stdio.h>
#include <stdlib.h>
#include "libconfig.h"
#include "libimport.h"

#pragma warning(disable : 4996 4267)

@z

@x
    if (fwrite(buf,1,n,sfile[handle].fp)!=n) return ominus(zero_octa,size);
    fflush(sfile[handle].fp);
@y
#ifdef MMIX_PRINT
    if (sfile[handle].fp==stdout || sfile[handle].fp==stderr)
      win32_log(buf);
    else
    { if (fwrite(buf,1,n,sfile[handle].fp)!=n) return ominus(zero_octa,size);
      fflush(sfile[handle].fp);
    }
#else
    if (fwrite(buf,1,n,sfile[handle].fp)!=n) return ominus(zero_octa,size);
    fflush(sfile[handle].fp);
#endif
@z
