/* Exchanging buffers through a queue between threads 
   Copyright 2008 Martin Ruckert ruckertm@acm.org

   This file is part of the MMIX port for GDB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <stdio.h>
#include "buffers.h"


void init_queue(queue *q)
{ int i;
  for (i=0;i<QUEUESIZE;i++)
    q->buffer[i] = NULL;
  q-> in = q->out = 0;
#ifdef WIN32
  q->hbuffer =CreateEvent(NULL,FALSE,FALSE,NULL);
  if (!q->hbuffer) 
  { fprintf(stderr, "Unable to create event for gdb command queue\n");
    exit(1);
  }
  InitializeCriticalSection (&(q->buffer_section));
#else
  pthread_mutex_init(&(q->buffer_mutex),NULL);
  pthread_cond_init(&(q->buffer_cond),NULL);
#endif
}

#ifdef WIN32
  #define LOCK   EnterCriticalSection (&(q->buffer_section))
  #define UNLOCK LeaveCriticalSection (&(q->buffer_section))
  #define WAIT   WaitForSingleObject(q->hbuffer,INFINITE)
  #define SIGNAL SetEvent (q->hbuffer);
#else
  static void clean_up_queue_mutex(void *q)
  { pthread_mutex_unlock(&(((queue*)q)->buffer_mutex)); 
  }
  #define LOCK   pthread_mutex_lock(&(q->buffer_mutex));\
                 pthread_cleanup_push(clean_up_queue_mutex,q)
  #define UNLOCK pthread_cleanup_pop(1);
  #define WAIT   pthread_cond_wait(&(q->buffer_cond),&(q->buffer_mutex))
  #define SIGNAL pthread_cond_signal(&(q->buffer_cond))
#endif  


int queue_size(queue *q)
{ int i; 
  LOCK;
  i = (QUEUESIZE+q->in-q->out)%QUEUESIZE;
  UNLOCK;
  return i;
}

void wait_until_empty(queue *q)
{
#ifdef WIN32
  LOCK;
  while (q->in!=q->out)
  { UNLOCK;
    WAIT;
    LOCK;
  }
  UNLOCK;
#else
  LOCK;
  while (q->in!=q->out)
  { WAIT;
  }
  UNLOCK;
#endif
}


void wait_until_not_full(queue *q)
{ 
#ifdef WIN32
  LOCK;
  while (((QUEUESIZE+q->in-q->out)%QUEUESIZE)>=QUEUESIZE-1)
  { UNLOCK;
    WAIT;
    LOCK;
  }
  UNLOCK;
#else
  LOCK;
  while (((QUEUESIZE+q->in-q->out)%QUEUESIZE)>=QUEUESIZE-1)
  { WAIT;
  }
  UNLOCK;
#endif
}


void enqueue(queue *q, void *b)
{
#ifdef WIN32
 LOCK;
  while (((QUEUESIZE+q->in-q->out)%QUEUESIZE)>=QUEUESIZE-1)
  { UNLOCK;
    WAIT;
    LOCK;
  }
  q->buffer[q->in] = b;
  q->in++;
  if (q->in>=QUEUESIZE) q->in = 0;
  UNLOCK;
  SIGNAL;
#else
  LOCK;
  while (((QUEUESIZE+q->in-q->out)%QUEUESIZE)>=QUEUESIZE-1)
  { WAIT;
  }
  q->buffer[q->in] = b;
  q->in++;
  if (q->in>=QUEUESIZE) q->in = 0;
  SIGNAL;
  UNLOCK;
#endif
}

void *dequeue(queue *q)
{ void *b;
#ifdef WIN32
 LOCK;
  while (q->in==q->out)
  { int r;
	UNLOCK;
    r = WAIT;
	if (r < 0) {
	   r = GetLastError();
       fprintf(stderr,"Error while waiting for command %d\n",r);
	}
    LOCK;
  }
  b = q->buffer[q->out];
  q->out++;
  if (q->out>=QUEUESIZE) q->out = 0;
  UNLOCK;
  SIGNAL;
#else
  LOCK;
  while (q->in==q->out)
  { WAIT;
  }
  b = q->buffer[q->out];
  q->out++;
  if (q->out>=QUEUESIZE) q->out = 0;
  SIGNAL;
  UNLOCK;
#endif
  return b;
}

