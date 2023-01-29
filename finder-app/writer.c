#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>


int fd,error;

int main(int argc , char *argv[]){
	
	openlog(NULL, 0, LOG_USER);
	
	if(argc!=3){
		printf("Invalid number of arguments, Please enter required 2 arguments (<filename> <write string>)");
		syslog(LOG_ERR,"Invalid number of arguments");
		return 1;
	}
	
	fd=0;
	
	fd=open(argv[1], O_WRONLY | O_CREAT ,S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);

	if(fd==-1){
		syslog(LOG_ERR,"File not avaliable");
		return 1;
	}
	
	error=write(fd,argv[2],strlen(argv[2]));
	
	if(error== -1){
		syslog(LOG_ERR,"coudnt write to the given file");
		return 1;
	}
	
	if (error != strlen(argv[2])){
		syslog(LOG_ERR, "partial string is wrriten into the file");
		return 1;
	}
	
	close(fd);
	closelog();
	return 0;
;	
}
