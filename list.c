/*
 * list.c - very stupid list. It is part of Dlorean project.
 * Rafael Ignacio Zurita, 2007
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

#include "list.h"


/*
 * add()
 * add an element to the list
 * the element has 2 strings and an integer:
 * 1- the file name being versioned
 * 2- the file name used as saved version
 * 3- the int n times that the file was opened
*/
int add (const char *f1, char *f2){
    size_t f1len=(strlen(f1)+1);
    size_t f2len=(strlen(f2)+1);
    void *res;
    struct entry *n1;

    n1 = malloc(sizeof(struct entry));      /* Insert at the head. */
    n1->fname=malloc(f1len);
    n1->bfname=malloc(f2len);
    n1->n = 1;
    res=memcpy(n1->fname,f1,f1len);
    res=memcpy(n1->bfname,f2,f2len);

    
    LIST_INSERT_HEAD(&head, n1, entries);
    printf("dato insertado=_%s_:_%s_\n",n1->fname,n1->bfname);

    return 1;
}


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

int exist (const char *f){
    struct entry *n1;

    for (n1 = head.lh_first; ((n1 != NULL)&&(strcmp(n1->fname,f))); n1 = n1->entries.le_next);
    if ( (n1!=NULL)&&(!strcmp(n1->fname,f)) )
	    return n1->n;

    return 0;
};



int mod (const char *f,int c){
    struct entry *n1;

    for (n1 = head.lh_first; n1 != NULL; n1 = n1->entries.le_next)
	     if (!strcmp(n1->fname,f)) {
		     n1->n = n1->n + c;
		     return 1;
	     };
    printf("[CASO EXTRANIO]\n");
    return 0;
};


/*
 * query()
 * query in the list: 
 * if file (*f) exists return it
 * if not, return 0
*/
char *query (const char *f, int *n){
    struct entry *n1;
    char *s;
printf("[file to compare:%s]\n",f);
    for (n1 = head.lh_first; n1 != NULL; n1 = n1->entries.le_next){
printf("[file to compare2:%s]\n",n1->fname);
	     if (!strcmp(n1->fname,f)) {
		     printf("[elemento1:%s]\n",n1->fname);
		     printf("[elemento2:%s]\n",n1->bfname);
		     s = malloc(strlen(n1->bfname)+1);
		     memcpy(s,n1->bfname,strlen(n1->bfname)+1);
		     *n=n1->n;
		     return s;
	     };
    };
    *n=0;
    return 0;
};


int del (const char *f){
    struct entry *n1;

    for (n1 = head.lh_first; ((n1 != NULL)&&(strcmp(n1->fname,f))); n1 = n1->entries.le_next);
    if (!strcmp(n1->fname,f)) {
	LIST_REMOVE(n1, entries);
	free(n1->fname);
	free(n1->bfname);
	free(n1);
	return 1;
    };
	
    return 0;
};

