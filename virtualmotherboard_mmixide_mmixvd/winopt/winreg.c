#include <windows.h>
#include "winopt.h"


#define FLAG_SIZE (sizeof(DWORD)*8)

void write_regtab(char *program)
{ HKEY p=0, vmbkey=0, s=0;
  LONG r;
  int success=0;
  if (program==NULL) return;
  r = RegOpenKeyEx(HKEY_CURRENT_USER,"Software",0,KEY_ALL_ACCESS,&s);
  if (r==ERROR_SUCCESS)
  {  DWORD n;
	  r = RegCreateKeyEx(s,"VMB",0,NULL,REG_OPTION_NON_VOLATILE,
               KEY_ALL_ACCESS,NULL,&vmbkey,&n);
     if (r==ERROR_SUCCESS)
	 {
	    r = RegCreateKeyEx(vmbkey,program,0,NULL,REG_OPTION_NON_VOLATILE,
               KEY_ALL_ACCESS,NULL,&p,&n);
        if (r==ERROR_SUCCESS)
	    { int i=0;
		  int has_flags=0,has_flagsaux=0;
		  DWORD flags=0,flagsaux=0;
		  while(regtab[i].key!=NULL)
		  { if (regtab[i].type==TYPE_DWORD)
		    { DWORD tmp=*(int*)regtab[i].value;
              RegSetValueEx(p,regtab[i].key,0,REG_DWORD,(BYTE*)&tmp,sizeof(DWORD));
		    }
		    else if (regtab[i].type==TYPE_STRING)
		    { char **tmp=(char**)regtab[i].value;
		      char empty[1]={0}, *eptr=empty;	
              if (*tmp==NULL) tmp=&eptr;
			  RegSetValueEx(p,regtab[i].key,0,REG_SZ,(BYTE*)*tmp,(DWORD)strlen(*tmp)+1);
		    } 
		    else if (0<=regtab[i].type && regtab[i].type<FLAG_SIZE)
		    { int tmp = *(int*)regtab[i].value;
			  has_flags=1;
		      flags =((flags & ~(1<<regtab[i].type)) | ((tmp)?(1<<regtab[i].type):0));
		    }
			else if (0<=regtab[i].type-FLAG_SIZE && regtab[i].type-FLAG_SIZE<FLAG_SIZE)
		    { int tmp = *(int*)regtab[i].value;
			  has_flagsaux=1;
		      flagsaux =((flagsaux & ~(1<<(regtab[i].type-FLAG_SIZE))) | 
				       ((tmp)?(1<<(regtab[i].type-FLAG_SIZE)):0));
		    }
			i++;
		  }
		  if (has_flags)	
		    RegSetValueEx(p,"flags",0,REG_DWORD,(BYTE*)&flags,sizeof(DWORD));
		  if (has_flagsaux)	
		    RegSetValueEx(p,"flagsaux",0,REG_DWORD,(BYTE*)&flagsaux,sizeof(DWORD));
          RegCloseKey(p);
		}
       RegCloseKey(vmbkey);
	 }
    RegCloseKey(s);
  }
}

void read_regtab(char *program)
{ HKEY p=0,vmbkey=0, s=0;
  LONG r;  /* get values from the registry */
  if (program==NULL) return;
  r = RegOpenKeyEx(HKEY_CURRENT_USER,"Software",0,KEY_ALL_ACCESS,&s);
  if (r==ERROR_SUCCESS)
  {  DWORD n;
	  r = RegCreateKeyEx(s,"VMB",0,NULL,REG_OPTION_NON_VOLATILE,
               KEY_ALL_ACCESS,NULL,&vmbkey,&n);
     if (r==ERROR_SUCCESS)
	 { r = RegCreateKeyEx(vmbkey,program,0,NULL,REG_OPTION_NON_VOLATILE,
               KEY_ALL_ACCESS,NULL,&p,&n);
        if (r==ERROR_SUCCESS)
	    { DWORD bc = sizeof(DWORD);
		  int i=0;
		  int has_flags=0, has_flagsaux=0;
		  DWORD flags=0,flagsaux=0;
		  r = RegQueryValueEx(p,"flags",0,NULL,(LPBYTE)&flags,&bc);
		  has_flags=(r==ERROR_SUCCESS);
		  r = RegQueryValueEx(p,"flagsaux",0,NULL,(LPBYTE)&flagsaux,&bc);
		  has_flagsaux=(r==ERROR_SUCCESS);
		  while(regtab[i].key!=NULL)
		  { if (regtab[i].type==TYPE_DWORD)
		    { DWORD tmp;
		      bc=sizeof(DWORD);
		  	  r = RegQueryValueEx(p,regtab[i].key,0,NULL,(LPBYTE)&tmp,&bc);
			  if (r==ERROR_SUCCESS)
			    *(int*)regtab[i].value=tmp;
		    }
		    else if (regtab[i].type==TYPE_STRING)
		    { 
#define		  MAXVALUE 521
			  char value[MAXVALUE];
		      bc=MAXVALUE-1;
		  	  r = RegQueryValueEx(p,regtab[i].key,0,NULL,(LPBYTE)&value,&bc);
			  if (r==ERROR_SUCCESS)
			  { value[bc]=0;
			    set_option((char**)regtab[i].value, value);
			  }
			  else if (r==ERROR_MORE_DATA)
				  win32_error(__LINE__,"Registry entry too big (>521 byte)");
		    } 
		    else if (has_flags && 0<=regtab[i].type && regtab[i].type<FLAG_SIZE)
		    { *(int*)regtab[i].value = ((flags&(1<<regtab[i].type))!=0);
		    }
		    else if (has_flagsaux && 0<=regtab[i].type-FLAG_SIZE && regtab[i].type-FLAG_SIZE<FLAG_SIZE)
		    { *(int*)regtab[i].value = ((flagsaux&(1<<(regtab[i].type-FLAG_SIZE)))!=0);
		    }
			i++;
		  }
          RegCloseKey(p);
		}
       RegCloseKey(vmbkey);
	 }
    RegCloseKey(s);
  }
}

