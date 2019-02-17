/* this program is opening a tty device and 
   connects stdin and stdout to it
   it is usefull to test the VMB tty
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define MAXBUF 1024

#ifdef DEBUG
#define debug(x) x
#else
#define debug(x)
#endif

int open_tty(char *path)
/* opens and initializes the device given by path
   returns file descriptor or -1
*/
{ int ttyfd;
  struct termios tios;
  ttyfd=open(path,O_RDWR);
  if (ttyfd<0)
  { fprintf(stderr,"Unable to open device %s\n", path);
    perror(NULL);
    return -2;
  }
  if (tcgetattr(ttyfd,&tios),0)
  { perror("Unable to get attibutes");
    return -3;
  }
  cfmakeraw(&tios);
  tios.c_cc[VMIN]=1;
  tios.c_cc[VTIME]=0;
  if (tcsetattr(ttyfd,TCSANOW,&tios)<0)
  { perror("Unable to set attibutes");
    return -4;
  }
  return ttyfd;
}

int do_io(int ttyfd)
{   int ready;
    fd_set read_set;
    fd_set except_set;
    FD_ZERO(&read_set);
    FD_SET((unsigned)ttyfd, &read_set);
    FD_SET((unsigned)0, &read_set);
    FD_ZERO(&except_set);
    FD_SET((unsigned)ttyfd, &except_set);
    FD_SET((unsigned)0, &except_set);

    ready = select (ttyfd+1, &read_set, NULL, &except_set, NULL);
    if (ready<0)
      { if (errno==EINTR) return 1;
        perror("select()");
        return 0;
      }
    if (FD_ISSET(0,&except_set))
    { debug(fprintf(stderr,"Exception on STDIN\n"));
      return 0;
    }
    else if (FD_ISSET(ttyfd,&except_set))
    { debug(fprintf(stderr,"Exception on TTY\n"));
      return 0;
    }
    else if (FD_ISSET(0,&read_set))
    { int size;
      char buf[MAXBUF];
      debug(fprintf(stderr,"Reading on STDIN\n"));
      size = read(0,buf,MAXBUF);
      if (size<0)
	{ if (errno==EINTR) return 1;
          perror("Error reading STDIN");
          return 0;
	}
      else if (size==0)
	{ debug(fprintf(stderr,"EOF on STDIN\n"));
          return 0;
	}
      else
	{ int i=0;
          int ret;
          debug(fprintf(stderr,"Writing to TTY\n"));
          do {
            ret=write(ttyfd,buf+i,size-i);
            if (ret<0)
	    {  if (errno==EINTR) continue;
               perror("Error writing TTY");
               return 0;
    	     }
             else if (ret==0)
             { debug(fprintf(stderr,"EOF on TTY\n"));
               return 0;
	     }
             else
               i=i+ret;
	  } while (i<size);
          return 1;
	}
    }
    else if (FD_ISSET(ttyfd,&read_set))
    { int size;
      char buf[MAXBUF];
      debug(fprintf(stderr,"Reading on TTY\n"));
      size = read(ttyfd,buf,MAXBUF);
      if (size<0)
	{ perror("Error reading TTY");
          return 0;
	}
      else if (size==0)
	{ debug(fprintf(stderr,"EOF on TTY\n"));
          return 0;
	}
      else
	{ int i=0;
          int ret;
          debug(fprintf(stderr,"Writing to STDOUT\n"));
          do {
            ret=write(1,buf+i,size-i);
            if (ret<0)
	    {  if (errno==EINTR) continue;
               perror("Error writing STDOUT");
               return 0;
    	     }
             else if (ret==0)
	     { debug(fprintf(stderr,"EOF on STDOUT\n"));
               return 0;
	     }
             else
               i=i+ret;
	  } while (i<size);
          return 1;
	}
    }
    else
    { debug(fprintf(stderr,"select returned 0\n"));
      return 0;
    }
}

int main(int argc, char *argv[])
{ int ttyfd;

  if (argc!=2)
  { fprintf(stderr,"Usage: talk device\n"
         	   "example:  talk /dev/ttyVMB\n");
    return -1;
  }
  
  ttyfd = open_tty(argv[1]);
  if (ttyfd<0) return ttyfd;
 
  while (do_io(ttyfd)) continue;

  close(ttyfd);

  return 0;
}


