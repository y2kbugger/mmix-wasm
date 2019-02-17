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

#ifndef BUFFERS_H

#define BUFFERS_H

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

/* QUEUESIZE is the size of the queue plus 1 to allow for one empty place.
   it helps distinguishing between a full and an empty circular buffer */
#define QUEUESIZE 5


typedef struct queue {
  void *buffer[QUEUESIZE];
  int in; 
  int out;
#ifdef WIN32
  HANDLE hbuffer;
  CRITICAL_SECTION   buffer_section;
#else
  pthread_mutex_t buffer_mutex;
  pthread_cond_t buffer_cond;
#endif
} queue;

extern void init_queue(queue *q);
extern int queue_size(queue *q);
extern void wait_until_empty(queue *q);
extern void wait_until_not_full(queue *q);
extern void enqueue(queue *q, void *b);
extern void *dequeue(queue *q);

#endif
