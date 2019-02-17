/*
	memcheck.h

	Memory leaks inspector

	Provides redefinitions for free() and malloc().
	Updated version from 
	http://openbook.galileocomputing.de/c_von_a_bis_z/027_c_sicheres_programmieren_002.htm
*/

#ifndef MEM_CHECK_H
#define MEM_CHECK_H

#include <stdio.h>

#define DEBUG_FILE "Debug"

static unsigned int count_malloc = 0;
static unsigned int count_free = 0;
FILE *f;

#define malloc(size) malloc(size);printf("malloc in Zeile %d der Datei %s (%d Bytes) \n",__LINE__,__FILE__, size);count_malloc++

#define free(x) free(x);x=NULL;printf("free in Zeile %d der Datei %s\n",__LINE__,__FILE__);count_free++

// Use EXIT_SUCC instead of "return EXIT_SUCCESS"
#define EXIT_SUCC f=fopen(DEBUG_FILE, "w");fprintf(f, "Anzahl malloc : %d\n",count_malloc);fprintf(f, "Anzahl free   : %d\n",count_free);fclose(f);printf("Datei : %s erstellt\n", DEBUG_FILE);return 0

#endif