#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>

int main(int argc , char *argv[]){
	

	openlog(NULL, 0, LOG_USER);
	
	if(argc != 3){
		printf("Invalid number of arguments, Please enter required 2 arguments (<filename> <write string>).\n\r");
		syslog(LOG_ERR,"Invalid number of arguments.");
		return 1;
	}
	
	
	int fd=open(argv[1], O_WRONLY | O_CREAT ,S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);

	if(fd == -1){
		syslog(LOG_ERR,"The file %s is not avaliable or created.", argv[1]);
		return 1;
	}
	
	int error = write(fd,argv[2],strlen(argv[2]));
	
	if(error == -1){
		syslog(LOG_ERR,"Coudnt write %s to the given file %s.", argv[2], argv[1]);
		return 1;
	}
	
	if (error != strlen(argv[2])){
		syslog(LOG_ERR, "partial string is written into the file. written bytes=%d, total bytes=%ld.",error,strlen(argv[2]));
		return 1;
	}
	
	syslog(LOG_DEBUG, "Wrote %s to file %s.", argv[2], argv[1]);
	
	close(fd);
	
	closelog();
	
	return 0;	
}
