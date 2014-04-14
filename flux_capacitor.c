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

#include "flux_capacitor.h"

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


/*
 * useful strcatloc given by wa
 */
char *strcatloc(const char *a,const char *b)
{
	size_t a_len = strlen(a);
	size_t b_len = strlen(b); 
	char *s = malloc(a_len+b_len+1);

	memcpy(s,a,a_len); 
	memcpy(s+a_len,b,b_len+1);
	return s;
}

/*
 * reemplaza el principio de la cadena old por new
 */
char *strreplace(const char *a,const char *old, const char *new)
{
	size_t a_len = strlen(a), old_len = strlen(old), new_len = strlen(new); 
	char *s = malloc(a_len-old_len+new_len+1);

	memcpy(s,new,new_len); 
	memcpy(s+new_len, (a+old_len), (a_len-old_len+1));
	return s;
}

/*
 * check_and_save - guarda una copia del archivo file
 * (si es que aun no ha sido respaldada)
 * TODO: reeemplazar return 1 por return 0/-1 :P
 */
int check_and_save(const char *file)
{
	char *new_file_ts;

	if (exist(file)) {
		mod(file,1);
		return 1;
	};

	new_file_ts=save(file);
	add(file,new_file_ts);
	free(new_file_ts);

	return 1;
};

char *save(const char *file)
{
	size_t size_c;
	time_t ts; 		// timestamp
	char *cmd;
	char *new_file, *nf;
	char *new_file_ts;
	char *date=malloc(11);  //the max lenght for ts (10) + 1 of \0
	struct stat buf;

	size_t rp_len = strlen(real_path);
	size_t vp_len = strlen(versions_place);
	size_t f_len = strlen(file);
	size_t nf_len;
	size_t d_len;
	
	
	if (exist(file)) {
		free(date);
		return NULL;
	};
	
	ts = time(NULL);
	sprintf(date, "%u", (unsigned int)ts);
	d_len = strlen(date);
	
	size_c = rp_len + 1 + vp_len + f_len + 1 + d_len;
	nf = malloc(size_c+1);    /* 1: free in (2) */
	sprintf(nf, "%s/%s%s.%s", real_path, versions_place, file, date);
	
	memset(&buf, 0, sizeof(buf));
	if (lstat(nf, &buf) == 0) {
		size_c = vp_len + f_len + 1 + d_len;
		new_file_ts = malloc(size_c+1);
		sprintf(new_file_ts, "%s%s.%s", versions_place, file, date);
		free(date);
		free(nf);  /* (2) */
		return new_file_ts;
	};
	free(nf);  /* (2) */
	
	size_c = 3 + rp_len + 21 + f_len-1 + 2 + vp_len;
	cmd = malloc(size_c+1);
	sprintf(cmd, "cd %s && cp -p --parents '%s' %s", real_path, file+1,
		versions_place);
	system(cmd);
	free(cmd);
	
	/* now, cmd shoud be: cd "real_path" && mv 'newfile' 'newfile'`date +%s`
	* "newfile" is the file saved as backup
	* this operation puts a time stamp to the new file
	* ts = time(NULL);
	* sprintf(date,"%u",ts);
	*/
	
	new_file = strcatloc(versions_place, file);
	nf_len = strlen(new_file);
	size_c = 3 + rp_len + 11 + nf_len + 3 + nf_len + 2 + d_len ; 
	cmd = malloc(size_c+1); /* *** ATENCION!, agregue el + 1 final para '\0' *** */
	sprintf(cmd, "cd %s && mv -u '%s' '%s'.%s", real_path, new_file,
		new_file, date);
	system(cmd);
	free(cmd);
	
	new_file_ts = malloc(nf_len + d_len + 2);
	sprintf(new_file_ts, "%s.%s", new_file, date);
	free(date);
	free(new_file);
	
	return new_file_ts;
};


/*
 * cmp_and_confirm - compara la copia guardada con el archivo actual. Si son
 * diferentes mantiene la copia. Si son iguales la elimina
 * (si aún existen procesos que mantienen algún descriptor para file entonces
 * se decrementa el contador en la lista de archivos abiertos)
 * TODO: reemplazar return 1 por retunr 0/-1 
 */
int cmp_and_confirm(const char *file)
{
	char *cmd;
	int size_c;
	char *new_file;

	int n;
	n = exist(file);
	if (n > 1) {
		mod(file,-1);
		return 1;
	} else if (n == 0)
		return 1;

	new_file = query(file, &n);

	size_c = 3 + strlen(real_path) + 9 + strlen(file)-1 + 3 +
		 strlen(new_file) * 2 + 10;
	cmd = malloc(size_c+1);
	sprintf(cmd, "cd %s && cmp '%s' '%s' && rm '%s'", real_path, file+1,
		new_file,new_file);
	system(cmd);
	free(cmd);
	free(new_file);
	del(file);
	return  0;
};


/* useful functions to register - unregister pids */

int traveling_pid (int pid)
{
	struct traveler *t;
	if (traveling == NULL)
		return -1;
	else {
		t = traveling;
		while ((t->next != NULL) && (t->pid != pid))
			t = t->next;
		if (t->pid == pid)
			return t->time;
		return -1;
	}
}

int register_pid (int pid, int time)
{
	struct traveler *t;
	if (traveling_pid(pid) > -1)  /* if the pid already is traveling, then return */
		return -1;
	else {
		if (traveling == NULL) {
			traveling=malloc(sizeof(struct traveler));
			traveling->pid = pid;
			traveling->time = time;
			traveling->next = NULL;
		} else {
			t = traveling;
			while(t->next != NULL)
				t = t->next;
			t->next = malloc(sizeof(struct traveler));
			t = t->next;
			t->pid = pid;
			t->time = time;
			t->next = NULL;
		}
	}
	return 0;
}

int unregister_pid (int pid)
{
	struct traveler *t,*t2;
	if ((traveling_pid(pid)) == -1)  /* if the pid is not traveling, then return */
		return -1;
	else {
		t = traveling;
		if (traveling->pid == pid) {
			if (traveling->next != NULL)
				traveling = traveling->next;
			else
				traveling = NULL;
			free(t);
		} else {
			while ((t->next != NULL) && ((t->next)->pid != pid))
				t = t->next;
			if ((t->next)->pid == pid) {
				t2 = t->next;
				t->next = (t->next)->next;
				free(t2);
			} else /* strange situation because we already know that the pid travels */
				return -1;
		}
	}
	return 0;
}

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

int fc_getattr(const char *path)
{
	char *p2=malloc(20);
	int b;
	/*  print_pids();  */
	if (!strncmp(reg_pid,path+1,strlen(reg_pid))) {
		char *p1=malloc(20);
		int a;
		strncpy(p1,path+1+strlen(reg_pid),10);
		strcpy(p1+10,"\0");
		strcpy(p2,path+11+strlen(reg_pid));
		a = strtol(p1, (char **)NULL, 10);
		b = strtol(p2, (char **)NULL, 10);
		free(p1);
		free(p2);
		register_pid(b,a);
		return 0;
	} 
	else if (!strncmp(unreg_pid,path+1,strlen(unreg_pid))){
		//char *p1=malloc(20);
		char *p2=malloc(20);
		//int a,b;
		int b;
		//strncpy(p1,path+1+strlen(unreg_pid),10);
		//strcpy(p1+10,"\0");
		strcpy(p2,path+11+strlen(unreg_pid));
		//a = strtol(p1, (char **)NULL, 10);
		b = strtol(p2, (char **)NULL, 10);
		//free(p1);
		free(p2);
		unregister_pid(b);
		/* TODO: check if unregister_pid is successful */
		return 0;
	} 
	return -1;
};
	
/* END of systems calls functions for traveling pids */





/*
 * in_the_past is a pswalk() - walks across parents until to find a traveling
 * pid.
 * I use some code from pstree.c 
 */

int in_the_past(int pid)
{
	FILE *file;
	struct stat st;
	char pidbuf[10]; /* WARNING: pid should be always < 999999999 */
	char *path, comm[256];
	char readbuf[BUFSIZ+1];
	char *tmpptr;
	pid_t ppid;
	int time;
	
	if (pid < 2)
		return -1;

	while ((pid != 1) && ((time = traveling_pid(pid)) == -1)) {
		sprintf(pidbuf, "%i", pid);
		path = malloc(strlen(PROC_BASE) + strlen(pidbuf) + 7);
		sprintf (path, "%s/%s/stat", PROC_BASE, pidbuf);

		if ((file = fopen (path, "r")) != NULL) {
			free(path);
			path = malloc(strlen(PROC_BASE)+strlen(pidbuf)+2);
			sprintf (path, "%s/%s", PROC_BASE, pidbuf);
			if (stat (path, &st) < 0) {
				free(path);
				return -1;
			};
			fread(readbuf, BUFSIZ, 1, file);
			if (ferror(file) == 0) {
				memset(comm, '\0', 256);
				tmpptr = strrchr(readbuf, ')'); /* find last ) */
				/*
				 * We now have readbuf with pid and cmd, and tmpptr+2
				* with the rest
				*/
				if (sscanf(readbuf, "%*d (%15[^)]", comm) == 1) {
					if (sscanf(tmpptr+2, "%*c %d", &ppid) == 1)
						pid = ppid;
				}
			}
			(void) fclose (file);
		  }
		  free(path);
	}
	if (pid == 1)
		return -1;
	else
		return time;  
};



/*
 * file_at_time checks if f and fc have the same name(without timestamp), and if 
 * f has been living at time t.
 * for example: f=filename, fc=filename.1234567890, t=1234567890 (timestamp is .1234567890)
 * second example: f=filename.1234567890, fc=filename.1234567890, t=1234567890
 */
int file_at_time(char *f, char *fc, int t, int check_ctime)
{
	struct stat stbuf;
	memset(&stbuf, 0, sizeof(stbuf));

	if (stat(f, &stbuf) == -1) 
		return 1;

	if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
		return 1;

	if ((!strncmp(f, fc, strlen(f))) &&
	    ((strlen(fc) == (strlen(f)+11)) || (strlen(fc) == strlen(f))) &&
	    (stbuf.st_mtime <= t) &&
	    ((check_ctime == 0) || (t < stbuf.st_ctime)))
		return 0;
	else 
		return 1;
};


/*
 * dir_at_time checks if dir d existed at time t
 */
int dir_at_time(char *d, int t, int check_ctime)
{
	char *name;
	struct dirent *dp;
	DIR *dfd;
   	struct stat stbuf;
	int not_found;

	if ((dfd = opendir(d)) == NULL)
		return 1;

	not_found = 1;
	while (not_found && ((dp = readdir(dfd)) != NULL)) {
		if ((strcmp(dp->d_name, ".") == 0) ||
		    (strcmp(dp->d_name, "..") == 0) ||
		    (strncmp(versions_place,dp->d_name, strlen(versions_place)) == 0))
			continue;    /* skip self and parent */

		if (strlen(d)+strlen(dp->d_name)+2 > 256)  /* I wish 256 = file name max lenght */
			continue;
		else {
			name = malloc(strlen(d)+strlen(dp->d_name)+2);
			sprintf(name, "%s/%s", d, dp->d_name);
  			memset(&stbuf, 0, sizeof(stbuf));
			if (stat(name, &stbuf) == -1) {
				free(name);
				continue;
			}
			if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
				not_found = dir_at_time(name, t, check_ctime);
			else if ((stbuf.st_mtime <= t) &&
				 ((check_ctime == 0) || (t < stbuf.st_ctime)))
				not_found = 0;
		}
		free(name);
	};
	closedir(dfd);
		
	if (not_found == 0)
		return 0;
	else
		return 1;
};





/* 
 * get_fn(f,t): Devuelve el nombre del archivo real para el tiempo t (solo para pids viajando)
 * f = file. t = time
 * get_fn devuelve el nombre del archivo que el pid que esta viajando debería ver.
 * Por ej.: si el proceso que viaja al pasado (al pasado "t") desea realizar un open(/dir/file)
 * get_fn("/dir/file", t) deberia devolver el nombre del archivo respaldado que 
 * contiene la información que existió en ese archivo para el tiempo "t".
 * El tiempo t es el tiempo en que el pid está viajando.
 */

char *get_fn(const char *f, int t)
{
	char *dn, *fn, *dirc, *basec, *rdn, *rf;
	/*
	 * dn : dir name
	 * fn : file name
	 * dirc, basec: f's temporary copies
	 * rdn : real dir name
	 * rf : return file
	 */
	struct stat buf;
	int size_c;
	char *path_t;
	size_t rdn_len;
	size_t rp_len = strlen(real_path);
	size_t vp_len = strlen(versions_place);
	size_t f_len = strlen(f);

	path_t = strcatloc(real_path, f);

	if (strcmp(f, "/") == 0)
		return path_t;

	/* 
	 * if f es un directorio tal vez debería hacer un chequeo de si el proceso ve 
	 * ese directorio y luego devolver la información de cual directorio ver
	 */
	
   	struct stat stbuf;
  	memset(&stbuf, 0, sizeof(stbuf));
	/* if (stat(path_t, &stbuf) == 0)
		if (((stbuf.st_mode & S_IFMT) == S_IFDIR) && 
		    (!dir_at_time(path_t, t, 0))) */
	if ((stat(path_t, &stbuf) == 0) &&
	    (((stbuf.st_mode & S_IFMT) == S_IFDIR) && 
	    (!dir_at_time(path_t, t, 0))))
			return path_t;
	free(path_t);

	size_c = rp_len + 1 + vp_len + f_len;
	path_t = malloc(size_c+1);    /* 1: free in (2) */
	sprintf(path_t,"%s/%s%s",real_path,versions_place,f);
  	memset(&stbuf, 0, sizeof(stbuf));
	if (stat(path_t, &stbuf) == 0)
		if (((stbuf.st_mode & S_IFMT) == S_IFDIR) && 
		    (!dir_at_time(path_t, t, 1)))
			return path_t;
	free(path_t);

	path_t = strcatloc(real_path, f);
	memset(&buf, 0, sizeof(buf));
	if ((lstat(path_t, &buf)==0) && (buf.st_mtime <= t))
		return path_t;
	else {
		//printf("ENTRO AL NO REAL PATH CON=%s\n",path_t);
		free(path_t);
		dirc = strdup(f);
		basec = strdup(f);
		dn = dirname(dirc);
		fn = basename(basec);
	
		size_c = rp_len + 1 + vp_len + strlen(dn);
		rdn = malloc(size_c+1);    /* 1: free in (2) */
		sprintf(rdn, "%s/%s%s", real_path, versions_place,dn);
		rdn_len = size_c + 1;
	
		char *name;
		struct dirent *dp=malloc(sizeof(struct dirent));
		DIR *dfd;
		int not_found;
		size_t dp_len;
	
		if ((dfd = opendir(rdn)) == NULL) {
			free(rdn);
			free(dirc);
			free(basec);
			free(dp);
			free(dn);
			free(fn);
			return NULL;
		};

		not_found=1;
		while (not_found && ((dp = readdir(dfd)) != NULL)) {
			dp_len = strlen(dp->d_name);
			if ((strcmp(dp->d_name, ".") == 0) ||
			    (strcmp(dp->d_name, "..") == 0))
				continue;    /* skip self and parent */
			if (rdn_len+dp_len+2 > 256) /* sizeof(name)) */
				continue;
			else {
				name=malloc(rdn_len+dp_len+2);
				sprintf(name, "%s/%s", rdn, dp->d_name);
  				memset(&stbuf, 0, sizeof(stbuf));
				if (stat(name, &stbuf) == -1) {
					free(name);
					continue;
				}
				if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
					free(name);
					continue;
				}
				if ((!strncmp(dp->d_name, fn, strlen(fn))) &&
				    (dp_len == (strlen(fn)+11)) &&
				    (stbuf.st_mtime <= t) &&
				    (t < stbuf.st_ctime))
					not_found = 0;
			}
			free(name);
		}

		rf = NULL;
		if (!not_found) {
			size_c = rdn_len + 1 + dp_len;
			rf = malloc(size_c+1);    /* 1: free in (2) */
			sprintf(rf,"%s/%s", rdn, dp->d_name);
	
		}
		closedir(dfd);
		free(dirc);
		free(basec);
		free(rdn);
		return rf;
	};
};


/*
 * check if dir2 lives in the same FS than dir1
 */
int samefs(char *dir1, char *dir2)
{
	struct statfs stbuf1, stbuf2;
	memset(&stbuf1, 0, sizeof(struct statfs));
	memset(&stbuf2, 0, sizeof(struct statfs));

	if (statfs(dir1, &stbuf1) == -1){
		printf("OUTCH! problemas con dir1\n");
		return -1;
	};
	if (statfs(dir2, &stbuf2) == -1){
		return -1;
		printf("OUTCH! problemas con dir2\n");
	};
	printf("STBUFFERS=%i,%i,%i,%i.\n", stbuf1.f_fsid.__val[0],stbuf2.f_fsid.__val[0],stbuf1.f_fsid.__val[1],stbuf2.f_fsid.__val[1]);
	if ((stbuf1.f_fsid.__val[0] == stbuf2.f_fsid.__val[0]) &&
	    (stbuf1.f_fsid.__val[1] == stbuf2.f_fsid.__val[1]))
		return 0;
	else
		return -1;
}



/*
 * this du estimates the .versions+place size.
 * and that size should be used in statfs at least.
 */

int du(char *dir)
{
	int s = 0;
	DIR *d;
	char name[256];
	struct dirent *dp=malloc(sizeof(struct dirent));
	struct stat stbuf;

	if ((d = opendir(dir)) == NULL) 
			return 0;

	while (((dp = readdir(d)) != NULL)) {
		if ((strcmp(dp->d_name, ".") == 0) ||
		    (strcmp(dp->d_name, "..") == 0))
			continue;    /* skip self and parent */
		if (strlen(dir)+strlen(dp->d_name)+2 > sizeof(name))
			continue;
		else {
			sprintf(name, "%s/%s", dir, dp->d_name);

 			memset(&stbuf, 0, sizeof(stbuf));
			if (stat(name, &stbuf) == -1) {
					continue;
			}
			if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
				s+=du(name);
			else
				s+=stbuf.st_size;
				
		}
	}
	closedir(d);
	return s;
};

int not_included (int n, struct dirent **namelist, char *file)
{
	int not_found = 1;
	while ((n--) && (not_found)) {
		if (namelist[n] != NULL)
			printf("INCLUIDO!!!!!!!!!!!=%s,%s.\n",file,namelist[n]->d_name);
		//if ((namelist[n] != NULL) && (!strncmp(file, namelist[n]->d_name, strlen(file))) 
		//&& ((strlen(file)+11) == strlen(namelist[n]->d_name))){
		if ((namelist[n] != NULL) && (!strcmp(file, namelist[n]->d_name))){
			not_found = 0;
		}
	}
	printf("NOT FOUUUUUND=%i\n",not_found);
	return not_found;
}

//int fc_getdir(int ts, const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
int fc_getdir(int ts, const char *path, void *h, fuse_fill_dir_t filler)
{
	struct dirent **namelist;
	struct dirent **namelist2;
	int n, ns;
	char *path_t;
	int size_c, f_ok;
	char *nf;
	struct stat st;
	int ddir_flag = 0;	/* flag for defaults dirs : . and .. */

	size_c = strlen(real_path) + 1 + strlen(versions_place) + strlen(path);
	nf = malloc(size_c+1);
	sprintf(nf,"%s/%s%s",real_path,versions_place,path);
	n = scandir(nf, &namelist2, 0, alphasort);
	ns = n;
	free(nf);
	if (n < 0)
		perror("scandir");  /* perhaps should be return 1 */
	else {
		while (n--) {
			size_c = strlen(real_path) + 1 + strlen(versions_place) + strlen(path) + 1 + strlen(namelist2[n]->d_name);
			nf = malloc(size_c+1);
			sprintf(nf,"%s/%s%s/%s",real_path,versions_place,path,namelist2[n]->d_name);
			memset(&st, 0, sizeof(st));
			st.st_ino = namelist2[n]->d_ino;
			//st.st_mode ...
			if (strncmp(versions_place, namelist2[n]->d_name, strlen(versions_place)) == 0) {
				free(nf);
				//free(namelist[n]);
				continue;
			//} else if ((strcmp(namelist2[n]->d_name, ".") == 0) ||
			} else if ((strcmp(namelist2[n]->d_name, "..") == 0)) {
				
					filler(h, namelist2[n]->d_name, &st, 0);
					ddir_flag = 1;		/* to avoid duplicate */
			}
			else if ((f_ok=dir_at_time(nf, ts, 1)) == 0)
	
				//filler(h, namelist2[n]->d_name, namelist2[n]->d_type, namelist2[n]->d_ino);
				filler(h, namelist2[n]->d_name, &st, 0);
			else if ((f_ok=file_at_time(nf, nf, ts, 1)) == 0) {
				free(nf);
				size_c = strlen(namelist2[n]->d_name)-11;
				nf = malloc(size_c+1);
				memset(nf,'\0',size_c+1);
				strncat (nf, namelist2[n]->d_name, size_c);
				//filler(h, nf, namelist2[n]->d_type, namelist2[n]->d_ino);
				filler(h, nf, &st, 0);
			} else {
				free(namelist2[n]);
				namelist2[n]=NULL;
			};
			free(nf);
			//free(namelist[n]);
		}
	}
	
	path_t = strcatloc(real_path, path);
	n = scandir(path_t, &namelist, 0, alphasort);
	free(path_t);
	
	if (n < 0)
		perror("scandir");  /* perhaps should be return 1 */
	else {
		while(n--) {
			size_c = strlen(real_path) + strlen(path) + 1 + strlen(namelist[n]->d_name);
			nf = malloc(size_c+1);
			sprintf(nf,"%s%s/%s",real_path,path,namelist[n]->d_name);

			memset(&st, 0, sizeof(st));
			st.st_ino = namelist[n]->d_ino;
			//st.st_mode ...
			if (strncmp(versions_place, namelist[n]->d_name, strlen(versions_place)) == 0) {
				free(nf);
				free(namelist[n]);
				continue;
			//} else if ((strcmp(namelist[n]->d_name, ".") == 0) ||
			} else if (strcmp(namelist[n]->d_name, "..") == 0) {
					if (!ddir_flag)  filler(h, namelist[n]->d_name, &st, 0);
			} else if ((printf("llamo a dir_at_time5=%s\n",nf)) && ((f_ok=dir_at_time(nf, ts, 0)) == 0)) {
				if (not_included(ns, namelist2, namelist[n]->d_name)) 
				filler(h, namelist[n]->d_name, &st, 0);
			}
			else if ((f_ok=file_at_time(nf, nf, ts, 0)) == 0)
				filler(h, namelist[n]->d_name, &st, 0);
			free(nf);
			free(namelist[n]);
		}
		while (ns--)
			if (namelist2[ns] != NULL)
				free(namelist2[ns]);

	}
	
	return 0;
};
