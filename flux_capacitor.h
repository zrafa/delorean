#ifndef FLUX_CAPACITOR_H
#define FLUX_CAPACITOR_H

/*
 * flux_capacitor.c - saves and retrieves file versions in the file system,
 *                    registers/unregisters proccess pids in the traveling pids
 *                    table and other useful functions.
 *
 * It is part of Dlorean project
 * Rafael Ignacio Zurita, 2006
 */

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

#include "list.h"

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <fuse.h>


/* Modify real_path if you want to use another directory as mount path */
//static const char *real_path = "/mnt/fuse/real";
char *real_path;

//static const char *mount_point_path = "/mnt/fuse/mount_point";
char *mount_point_path;
static const char *versions_place = ".versions+place";

static const char *reg_pid = ".register";
static const char *unreg_pid = ".unregister";

char *save(const char *file);

struct traveler {
	int pid;
	int time;
	struct traveler *next;
} *traveling;


/*
 * useful strcatloc given by wa
 */
char *strcatloc(const char *a,const char *b);

/*
 * reemplaza el principio de la cadena old por new
 */
char *strreplace(const char *a,const char *old, const char *new);

/*
 * check_and_save - guarda una copia del archivo file
 * (si es que aun no ha sido respaldada)
 * TODO: reeemplazar return 1 por return 0/-1 :P
 */
int check_and_save(const char *file);

char *save(const char *file);


/*
 * cmp_and_confirm - compara la copia guardada con el archivo actual. Si son
 * diferentes mantiene la copia. Si son iguales la elimina
 * (si aún existen procesos que mantienen algún descriptor para file entonces
 * se decrementa el contador en la lista de archivos abiertos)
 * TODO: reemplazar return 1 por retunr 0/-1 
 */
int cmp_and_confirm(const char *file);


/* useful functions to register - unregister pids */
int traveling_pid (int pid);

int register_pid (int pid, int time);

int unregister_pid (int pid);

/*
 * Por ahora no es necesaria print_pids()
 * void print_pids (void)
 * {
 * 	struct traveler *t;
 * 	if (traveling == NULL)
 * 		return -1;
 * 	else {
 * 		t = traveling;
 * 		while(t->next!=NULL){
 * 			printf("pid=%i....time%i\n",t->pid,t->time);
 * 			t = t->next;
 * 		}
 * 		printf("pid=%i....time%i\n",t->pid,t->time);
 * 		return 0;
 * 	}
 * }
 */

/* END of useful functions for register - unregister pid */


/* 
 * systems calls functions for traveling pids 
 * these are called when travel.c wants to register a pid
 */

int fc_getattr(const char *path);
	
/* END of systems calls functions for traveling pids */


/*
 * in_the_past is a pswalk() - walks across parents until to find a traveling
 * pid.
 * I use some code from pstree.c 
 */

#define PROC_BASE    "/proc"
int in_the_past(int pid);

/*
 * file_at_time checks if f and fc have the same name(without timestamp), and if 
 * f has been living at time t.
 * for example: f=filename, fc=filename.1234567890, t=1234567890 (timestamp is .1234567890)
 * second example: f=filename.1234567890, fc=filename.1234567890, t=1234567890
 */
int file_at_time(char *f, char *fc, int t, int check_ctime);


/*
 * dir_at_time checks if dir d existed at time t
 */
int dir_at_time(char *d, int t, int check_ctime);


/* 
 * get_fn(f,t): Devuelve el nombre del archivo real para el tiempo t (solo para pids viajando)
 * f = file. t = time
 * get_fn devuelve el nombre del archivo que el pid que esta viajando debería ver.
 * Por ej.: si el proceso que viaja al pasado (al pasado "t") desea realizar un open(/dir/file)
 * get_fn("/dir/file", t) deberia devolver el nombre del archivo respaldado que 
 * contiene la información que existió en ese archivo para el tiempo "t".
 * El tiempo t es el tiempo en que el pid está viajando.
 */

char *get_fn(const char *f, int t);


/*
 * check if dir2 lives in the same FS than dir1
 */
int samefs(char *dir1, char *dir2);


/*
 * this du estimates the .versions+place size.
 * and that size should be used in statfs at least.
 */

int du(char *dir);

int not_included (int n, struct dirent **namelist, char *file);

//int fc_getdir(int ts, const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
int fc_getdir(int ts, const char *path, void *h, fuse_fill_dir_t filler);
 
#endif
