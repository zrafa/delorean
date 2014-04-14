/*
 *  dlorean.c : Un versioning filesystem orientado al usuario
 *
 *  Copyright (c) 2009 - Rafael Ignacio Zurita <rizurita@yahoo.com>
 *
 *
 *  Leer el archivo README para conocer como compilar y utilizar
 *
 *  This program can be distributed under the terms of the GNU GPL.
 *  See the file COPYING.
 */ 

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include "flux_capacitor.h"


static int dlorean_getattr(const char *path, struct stat *stbuf)
{
	char *path_t;
	int res = 0;
	int time;
	
	if (!fc_getattr(path)){
		printf("devuelvo 0!..\n");
		res = lstat("/dev/null",stbuf);
		return 0;
	};
	
	//  **** PARA NO PERMITIR EL INGRESO AL DIRECTORIO de versiones !!**
	if (!strncmp(versions_place,path+1,strlen(versions_place)))
		    return -2;
	
	if (fuse_get_context()->pid < 1) 
		    return -2;

	if ((time=in_the_past(fuse_get_context()->pid)) > 0) 
		path_t=get_fn(path, time);
	else
		path_t = strcatloc(real_path, path);
	
	// el siguiente memset.. puede ser reemplazado por stbuf[..]=0?
	memset(stbuf, 0, sizeof(struct stat));
	
	// utilizo lstat para poder obtener tambien el stat de symbolic links
	res = lstat(path_t, stbuf);
	
	free(path_t);
	
	// if res<0 then hubo un error con stat. Devuelvo -2 (no such file or dir)
	if (res < 0)
		res = -2;
	return res;
}

static int dlorean_statfs(const char *path, struct statvfs *stbuf)
{
	char *path_t;
	int res = 0;
	
	path_t = strcatloc(real_path, path);
	// el siguiente memset.. puede ser reemplazado por stbuf[..]=0?
	memset(stbuf, 0, sizeof(struct statvfs));
	res = statvfs(path_t, stbuf);
	free(path_t);
	
	return res;
	}
	
static int dlorean_readlink(const char *path, char *buf, size_t size)
{
	char *path_t;
	int res = 0;
	
	path_t = strcatloc(real_path, path);
	res = readlink(path_t, buf, size-1);
	buf[res] = 0;
	free(path_t);
	
	// controlo si en buf existe el camino real_path
	if ( (path_t = strstr(buf, real_path)) && (strlen(path_t)==strlen(buf)) ) {
		path_t = strreplace (buf, real_path, mount_point_path);
		memcpy(buf, path_t, strlen(path_t));
		buf[strlen(path_t)] = 0;
	};
	
	// res = a la cantidad de de caracteres ubicados en el buffer
	// o res = -1 si hubo un error.
	// dlorean_readlink debería devolver 0 si todo ha ido bien.
	if (res > -1)
		res = 0;
	return res;
}

static int dlorean_mknod(const char *path, mode_t mode, dev_t dev)
{
	char *path_t;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	path_t = strcatloc(real_path, path);
	res = mknod(path_t, mode, dev);
	free(path_t);
	
	return res;
}

static int dlorean_utime(const char *path, struct utimbuf *buf)
{
	char *path_t;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	path_t = strcatloc(real_path, path);
	res = utime(path_t, buf);
	free(path_t);
	
	return res;
}

static int dlorean_mkdir(const char *path, mode_t mode)
{
	char *path_t;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	path_t = strcatloc(real_path, path);
	res = mkdir(path_t, mode);
	free(path_t);
	
	return res;
}
	
static int dlorean_unlink(const char *path)
{
	char *path_t;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	char *f;
	f = save(path);    /* in flux_capacitor.c */
	free(f);
	
	path_t = strcatloc(real_path, path);
	res = unlink(path_t);
	free(path_t);
	
	return res;
}

static int dlorean_rmdir(const char *path)
{
	char *path_t;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	path_t = strcatloc(real_path, path);
	res = rmdir(path_t);
	free(path_t);
	
	return res;
}
	
static int dlorean_symlink(const char *oldpath, const char *newpath)
{
	char *path_n, *path_o;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	path_n = strcatloc(real_path, newpath);
	
	// controlo si en buf existe el camino real_path
	if ( (path_o = strstr(oldpath, mount_point_path)) && (strlen(path_o)==strlen(oldpath)) ) {
		path_o = strreplace (oldpath, mount_point_path, real_path);
		res = symlink(path_o, path_n);
		free(path_o);
	} else 
		res = symlink(oldpath, path_n);
	
	free(path_n);
	
	return res;
}

static int dlorean_rename(const char *oldpath, const char *newpath)
{
	char *path_o, *path_n;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	char *f;
	f = save(oldpath);    /* in flux_capacitor.c */
	free(f);
	
	path_o = strcatloc(real_path, oldpath);
	path_n = strcatloc(real_path, newpath);
	res = rename(path_o, path_n);
	free(path_n);
	free(path_o);
	
	return res;
}
	
static int dlorean_link(const char *oldpath, const char *newpath)
{
	char *path_o, *path_n;
	int res = 0;

	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	path_o = strcatloc(real_path, oldpath);
	path_n = strcatloc(real_path, newpath);
	res = link(path_o, path_n);
	free(path_n);
	free(path_o);
	
	return res;
}

static int dlorean_chmod(const char *path, mode_t mode)
{
	char *path_t;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	path_t = strcatloc(real_path, path);
	res = chmod(path_t, mode);
	free(path_t);
	
	return res;
}
	
static int dlorean_chown(const char *path, uid_t owner, gid_t group)
{
	char *path_t;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	path_t = strcatloc(real_path, path);
	res = chown(path_t, owner, group);
	free(path_t);
	
	return res;
}

static int dlorean_truncate(const char *path, off_t newsize)
{
	char *path_t;
	int res = 0;
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;
	
	char *f;
	f = save(path);    /* in flux_capacitor.c */
	free(f);
	
	path_t = strcatloc(real_path, path);
	res = truncate(path_t, newsize);
	free(path_t);
	
	return res;
}

static int dlorean_opendir(const char *path, struct fuse_file_info *fi)
{
/*
	int size_c, dir_ok;
	char *path_t;
	time_t ts;
	DIR *n;

	(void) fi;
	
	if ((ts=in_the_past(fuse_get_context()->pid)) > 0) {
		size_c = strlen(real_path) + 1 + strlen(versions_place) + strlen(path);
		path_t = malloc(size_c+1);
		printf(path_t,"%s/%s%s",real_path,versions_place,path);
		if ((dir_ok = dir_at_time(path_t, ts, 1)) != 0)
			return -1;
	}
	else 
		path_t = strcatloc(real_path, path);
	n = opendir(path_t);
	free(path_t);
	if (n == NULL)
		return -1;
	else */
		return 0;

}

//static int dlorean_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
static int dlorean_readdir(const char *path, void *h, fuse_fill_dir_t filler,
			  off_t offset, struct fuse_file_info *fi)
{
	struct dirent **namelist;
	int n;
	char *path_t;
	time_t ts;

	(void) offset;
	(void) fi;

	
	ts = in_the_past(fuse_get_context()->pid);
	if (ts > 0) 
		return fc_getdir(ts, path, h, filler);
	else {
		path_t = strcatloc(real_path, path);
		n = scandir(path_t, &namelist, 0, alphasort);
		free(path_t);
	
		if (n < 0) {
			printf("1111: no pude hacer un scandir de:%s__\n",path_t);
			perror("scandir"); /* perhaps should be return 1 */
		} else {
			while (n--) {
				/* 
				 * si namelist[n]->d_name no es verions+place entonces
				 * coloco a namelist[n]->d_name en la estructura h
				 */
				if (strncmp(versions_place,namelist[n]->d_name,strlen(versions_place))){
					struct stat st;
					memset(&st, 0, sizeof(st));
					st.st_ino = namelist[n]->d_ino;
					// st.st_mode..
		    			//filler(h, namelist[n]->d_name, namelist[n]->d_type, namelist[n]->d_ino);
		    			filler(h, namelist[n]->d_name, &st, 0);
				};
		    		free(namelist[n]);
			}
		}
	};
	return 0;
}
static int dlorean_releasedir(const char *path, struct fuse_file_info *fi)
{
		return 0;
}

static int dlorean_release(const char *path, struct fuse_file_info *fi)
{
	int n;

	int time = in_the_past(fuse_get_context()->pid);
	if (time < 0) 
		cmp_and_confirm(path);    /* in flux_capacitor.c */
	
	n = close (fi->fh);
	if (n == -1) {
		perror("close");
		return -EACCES;
	} 
	return 0;
}

static int dlorean_open(const char *path, struct fuse_file_info *fi)
{
	int n;
	char *path_t;
	
	samefs(real_path, mount_point_path);
	/* The line below doesn't work :(. Perhaps i should check only "readonly"
	* flag in an open operation
	*/
	//if ((fi->flags == O_WRONLY) || (fi->flags == O_RDWR))
	/*
	* if ((fi->flags > O_RDONLY))
	*      save_a_copy(path);
	*/
	/* When i open some files (if they already exist) the flags
	* are 32768 (with "more" for example) or > 32768.
	* Is read only 0 only?
	*/
	
	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0)
		path_t=get_fn(path, time);
	else {
		check_and_save(path);
		path_t = strcatloc(real_path, path);
	};
	
	n = open (path_t, fi->flags);
	free(path_t);
	if (n == -1) {
		perror("open");
		return -EACCES;
	} 
	fi->fh = n;
	
	return 0;
}

static int dlorean_write(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	off_t lseek_r;
	size_t len;

	int time = in_the_past(fuse_get_context()->pid);
	if (time > 0) 
		return -1;

	lseek_r = lseek (fi->fh, offset, SEEK_SET);
	// aqui tal vez deberia controlar si lseek fue exitoso o no
	len = write (fi->fh, buf, size);
	if (len == -1) {
		perror("write");
		return -1;
	}
	
	return len;
}

static int dlorean_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	off_t lseek_r;
	size_t len;
	
	lseek_r = lseek (fi->fh, offset, SEEK_SET);
	// aqui tal vez deberia controlar si lseek fue exitoso o no
	len = read (fi->fh, buf, size);
	if (len == -1) {
		perror("read");
		return -1;
	}
	
	return len;
}

static struct fuse_operations dlorean_oper = {
	.getattr	= dlorean_getattr,
	.statfs		= dlorean_statfs,
	.readlink	= dlorean_readlink,
	.opendir	= dlorean_opendir,
	.readdir	= dlorean_readdir,
	.releasedir	= dlorean_releasedir,
	.mknod		= dlorean_mknod,
	.utime		= dlorean_utime,
	.mkdir		= dlorean_mkdir,
	.rmdir		= dlorean_rmdir,
	.unlink		= dlorean_unlink,
	.symlink	= dlorean_symlink,
	.rename		= dlorean_rename,
	.link		= dlorean_link,
	.chmod		= dlorean_chmod,
	.chown		= dlorean_chown,
	.truncate	= dlorean_truncate,
	.release	= dlorean_release,
	.open		= dlorean_open,
	.read		= dlorean_read,
	.write		= dlorean_write,
};

int main(int argc, char *argv[])
{
	char *path;
	int size_c, n;

	real_path = malloc(strlen(argv[1]) + 1);
	strcpy(real_path, argv[1]);
	mount_point_path = malloc(strlen(argv[2]) + 1);
	strcpy(mount_point_path, argv[2]);
	traveling = NULL;

	size_c = strlen(real_path) + 1 + strlen(versions_place) + 5;
	path = malloc(size_c+1);
	sprintf(path,"%s/%s/test",real_path,versions_place);

	n = mknod (path, S_IFREG, S_IFREG);
	if (n == -1) {
		fprintf(stderr, "\nERROR: you can't write on %s/%s/\n"
			"The File System must use that directory, so fix the problem and re-try.\n\n",real_path,versions_place);
		_exit(-1);
	} 
	n = unlink(path);
	free(path);

	fuse_main((argc-1), (argv+1), &dlorean_oper, NULL);
	return 0;
}

