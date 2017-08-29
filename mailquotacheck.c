#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#define DEBUG 0

#define FALSE 0
#define QUOTA_POR_DEFECTO 512000 /* 512 Mb */

#define SERVERNAME "arcba2emx001"

static int du_depth = 0;

int isDirectory(const char *fileName, const int followLinks,
				struct stat *statBuf)
{
	int status;
	struct stat astatBuf;

	if (statBuf == NULL) {
		/* use auto stack buffer */
		statBuf = &astatBuf;
	}

	if (followLinks)
		status = stat(fileName, statBuf);
	else
		status = lstat(fileName, statBuf);

	status = (status == 0 && S_ISDIR(statBuf->st_mode));

	return status;
}


static long du(char *filename)
{
	struct stat statbuf;
	long sum;
	int len;

	if ((lstat(filename, &statbuf)) != 0) {
		fprintf(stderr, "du: %s: %s\n", filename, strerror(errno));
		return 0;
	}

	du_depth++;
	sum = (statbuf.st_blocks >> 1);

	/* los links simbolicos no se tienen en cuenta */
	if (S_ISLNK(statbuf.st_mode)) {
		sum = 0L;
		if (du_depth == 1)
			return(sum);
//			print(sum, filename);
	}
	if (S_ISDIR(statbuf.st_mode)) {
		DIR *dir;
		struct dirent *entry;

		dir = opendir(filename);
		if (!dir) {
			du_depth--;
			return 0;
		}

		len = strlen(filename);
		if (filename[len - 1] == '/')
			filename[--len] = '\0';

		while ((entry = readdir(dir))) {
			char newfile[BUFSIZ + 1];
			char *name = entry->d_name;

			if ((strcmp(name, "..") == 0)
				|| (strcmp(name, ".") == 0)) {
				continue;
			}

			if (len + strlen(name) + 1 > BUFSIZ) {
				fprintf(stderr, "du: nombre demasiado largo\n");
				du_depth--;
				return 0;
			}
			sprintf(newfile, "%s/%s", filename, name);

			sum += du(newfile);
		}
		closedir(dir);
//		print(sum, filename);
	}
	du_depth--;
	return sum;
}


int main(int argc, char **argv)
{
	char quotaf[BUFSIZ], cantidad[10], fecha_mail[30], path[BUFSIZ], usuario[1024], msg[BUFSIZ];
	long long int quota = QUOTA_POR_DEFECTO, sum = 0;
	int ret=0, automatico=0, quotafd = 0, len = 0, i = 0;
	char c;
        time_t now;
        struct tm *l_time;
	char *result=NULL;

	result = malloc(BUFSIZ * sizeof(char));
	/**
	 * Intenta determinar el tama#o del mensaje
	 * (Pendiente)
	 *
	 */

	/**
	 *  Determina si el mail proviene de un rebote automatico
	 */
	memset(msg, 0x0, BUFSIZ);
	while (fgets(msg, sizeof(msg), stdin) != NULL) {
		if (strncasecmp(msg, "X-MV-automatic:", 15) == 0) {
			fprintf(stderr, "El mail es una respuesta automÃ¡tica interna. No se genera rebote\n");
        		automatico=1;
			break;
		}
	}

	/**
	 *  Obtiene el path al directorio personal del usuario
	 */
	char *home = getenv("HOME");

	if (DEBUG) fprintf(stderr, "%d: Home dir: %s\n", getpid(), home);
		
	/**
	 * Si puede abrir el archivo de quota, utiliza el dato almacenado allÃ.
	 * Si no puede abrirlo, toma el valor por defecto de la quota
	 */
	sprintf(quotaf, "%s/.quota", home);
        if ((quotafd = open(quotaf, O_RDONLY)) != -1) {
                while ((len = read(quotafd, &c, 1)) != 0) {
			if ((c >= '0') && (c <= '9')) {
                        	cantidad[i] = c;
				i++;
			}
                }
                cantidad[i] = '\0';
		quota = atoll(cantidad);
        	close(quotafd);
#if 0
		fprintf(stderr, "CAT TEXT: %s CANT: %lld\n", cantidad, quota);
        } else {
		fprintf(stderr, "Ha ocurrido un error cuando se intentaba conocer el tamaño de la quota del usuario. La entrega del mensaje se intentará más tarde.");
                exit(111);
#endif
	}
	if (DEBUG) fprintf(stderr, "%d: Quota: %lld\n", getpid(), quota);

	/**
	 *  Si la quota es 0, no esta¡ limitado asi que termina sin error */
	if (quota == 0) {
		ret=0;
		goto RET;
	}

//	print = print_summary;

	sum = du(home);
	if (DEBUG) fprintf(stderr, "%d: Du home: %lld\n", getpid(), sum);

	if (sum && isDirectory(home, FALSE, NULL)) {
//		print_summary(sum, home);
		/**
		 * If size > quota, write warning mail to user
		 */
		if (sum > quota) {
			if (DEBUG) fprintf(stderr, "%d: size %lld supera quota %lld\n", getpid(), sum, quota);
			time(&now);
        		// usuario=`$pwd | $cut -d "/" -f 5`
			sprintf(quotaf, "%s/Maildir/new/%ld.%d_0.%s", home, now, getpid(), SERVERNAME);
        		if ((quotafd = open(quotaf, O_CREAT | O_WRONLY | O_TRUNC, 0600)) != -1) {
				                                       time(&now);
                                l_time = localtime(&now);
                                strftime(fecha_mail, sizeof fecha_mail, "%d %b %Y %H:%M:%S %z", l_time);

       				sprintf(path, "%s", home);
        			result = strtok(path, "/");

        			while (result != NULL) {
                			sprintf(usuario, "%s", result);
                			result = strtok(NULL, "/");
        			}

				sprintf(msg, "Date: %s\nX-MV-automatic: yes\nFrom: postmaster@conduentlatam.com\nTo: %s@conduentlatam.com\nSubject: Casilla de correo excedida\n\nHan intentado enviar un mail a su direccion, pero este\nha sido rebotado porque su casilla excede la cuota permitida\nasignada por usuario. Por favor, elimine correo antiguo de su\ncasilla y borre las carpetas donde se almacena el mail eliminado.\n\n", fecha_mail, usuario);

				write(quotafd, msg, strlen(msg));
        			close(quotafd);

				if (automatico) {
        				ret=99;
					goto RET;
				} else {
					fprintf(stdout, "Usuario excedido en uso de disco de su cuenta de mail.\nLamentablemente, tu mensaje no puede ser entregado a esta cuenta.\nEl espacio en disco para este usuario esta siendo limitado.\n");
        				ret=100;
					goto RET;
				}
	       		 } else {
				fprintf(stdout, "Ha ocurrido un error cuando se intentaba conocer la cuota del usuario. La entrega del mensaje se intentara luego.\n");
        			ret=111;
				goto RET;

			}
		}
	} else {
		fprintf(stdout, "Ha ocurrido un error cuando se intentaba conocer el uso de disco del usuario. La entrega del mensaje se intentara luego.\n");
        	ret=111;
		goto RET;
	}

	ret=0;


RET:
	if (result)
		free(result);

	return(ret);


}
