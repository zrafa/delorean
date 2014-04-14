/*
 * travel.c - registers/unregisters pids in dlorean traveling pids table,
 *            and executes a new program.
 * It is part of Dlorean project
 * Rafael Ignacio Zurita, 2007
 */

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#include "flux_capacitor.h"

int
main(int argc, char *argv[])
{
	pid_t cpid, w;
	int status,i,err;
	time_t ts,tshelp;
	struct tm tmstruct;
	char *chartime=malloc(20);
	char *fn=malloc(16);  /* fn: temp file name. E.g. /tmp/1235678901 */
	char *fnreg=malloc(60); /* fnreg: file name for pid registration */
	char *fnreg2=malloc(60); /* fnreg2: file name for unregistration pid */
	struct stat buf;

	if (!(argc == 4)){
		printf("\nUse:\n\t# travel yyyy-mm-dd hh:mm:ss command\n\n");
		exit(1);
	};

	char *command=malloc(strlen(argv[3])+1);;
	strcat(command,argv[3]);
	printf("Ejecutando %s ...\n",command);

	memcpy(chartime,argv[1],strlen(argv[1]));
	memcpy(chartime+10,argv[2],strlen(argv[2])+1);
	strptime(chartime, "%Y-%m-%d%H:%M:%S", &tmstruct);

	tmstruct.tm_isdst = -1;
	ts = mktime(&tmstruct);           

	tshelp = time(NULL);
	sprintf(fn,"/tmp/%u",tshelp);

	cpid = fork();

	/*
	 * llamar al sistema de archivos con un stat de .traveling(segundos
	 * desde 1970)(pid del proceso a viajar)
	 */
	sprintf(fnreg,"%s%u%i",reg_pid,ts,cpid);
	sprintf(fnreg2,"%s%u%i",unreg_pid,ts,cpid);

	if (cpid == -1) { perror("fork"); exit(EXIT_FAILURE); }

	if (cpid > 0) {		       /* Code executed by parent */
	        /* 
		 * Intento registrar en el sistema de archivos
		 * al proceso hijo que va a comenzar a viajar 
		 */
		memset(&buf,0,sizeof(buf));
		err = stat(fnreg, &buf);
		if (err==-1) {
			printf("Error al registrar proceso viajante. Abortando ..\n");
			kill(cpid,SIGKILL);
			exit(err);
		}

		err = mknod(fn, S_IFREG, 0);  /* dev=0 ignored */
		if (err==-1) {
			printf("Error al crear archivo de coordinacion. Abortando ..\n");
			kill(cpid,SIGKILL);
			exit(err);
		}
		/* Espero a que finalice la ejecucion de mi proceso hijo */
		do {
			w = waitpid(cpid, &status, WUNTRACED | WCONTINUED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status)); 

		/* aca debo desregistrar en el sistema de archivos al proceso hijo */
		memset(&buf,0,sizeof(buf));
		err = stat(fnreg2, &buf);
		if (err==-1) {
			printf("Error al desregistrar proceso viajante. Abortando ..\n");
			kill(cpid,SIGKILL);
			exit(err);
		} 

		printf("Ya ha terminado la ejecucion de %s\n",command);
		err = unlink(fn);
		exit(EXIT_SUCCESS);
	}
	else { 	       /* Code executed by child */
	       
		/* espero a que mi padre registre mi PID en el sistema de archivos */
		do {
			memset(&buf,0,sizeof(buf));
			err = stat(fn, &buf);
		} while (err < 0);
		execve(command,NULL,NULL);
	}
}
