
/*
 * list.c - very stupid list. It is part of Dlorean project.
 * Rafael Ignacio Zurita, 2007
 */

#ifndef LIST_H
#define LIST_H


#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>


LIST_HEAD(listhead, entry) head;
struct listhead *headp;	             /* List head. */
struct entry {
     char *fname;           /* the file name being versioned */
     char *bfname;          /* the file name used as version saved */
     int   n;               /* n times opening */
     LIST_ENTRY(entry) entries;      /* List: open files. */
}; 


/*
 * add()
 * add an element to the list
 * the element has 2 strings and an integer:
 * 1- the file name being versioned
 * 2- the file name used as saved version
 * 3- the int n times that the file was opened
*/
int add (const char *f1, char *f2);

/*
 * exist():
 *  if f exists and n1->n==1 and c==-1 then return 1
 *  if f exists and n1->n>1 and c==-1 then n1-> -c 
 *  if f exists and c==1 then n1->n + c 
 *  if not, then return 0
 *  c should be 1 or -1
 *  it is because this function is usefull when exist un "open" or a "close"
 *  operation
 */

int exist (const char *f);

int mod (const char *f,int c);


/*
 * query()
 * query in the list: 
 * if file (*f) exists return it
 * if not, return 0
*/
char *query (const char *f, int *n);

int del (const char *f);

#endif
