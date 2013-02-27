/*
 ============================================================================
 Name        : quash.c
 Author      : Howard Grimberg
 Version     :
 Copyright   : Copyright  201 3 - The University of Kansas
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>

#define _PROGRAM_NAME "whoami"
#define TRUE (!FALSE)
#define FALSE (0)
#define BUFFER_LENGTH 2048
#define MAX_PATHS 24
#define MAX_PATH_LEN 1024
#define MAX_ARGV 12
#define MAX_ARG_LEN 128

// the line they enter
char line[BUFFER_LENGTH];

// the holder for search paths
char search[MAX_PATHS][MAX_PATH_LEN];
int path_len = 0;



char* accept(void) {
	read(0, line, BUFFER_LENGTH);
	return line;
}

void execute(char **argv) {
    pid_t  pid;
    int    status;
    if ((pid = fork()) < 0) {
         printf("*** ERROR: forking child process failed\n");
         exit(1);
    }
    else if (pid == 0) {
        if (execvp(*argv, argv) < 0) {
            printf("*** ERROR: exec failed\n");
            exit(1);
        }
    }
    else {
        while (wait(&status) != pid)
            ;
    }
}


int main(void) {

	register struct passwd *pw;
	register uid_t uid;
	char *user;

	char *buffer;
	uid = geteuid();
	pw = getpwuid(uid);
	if (pw) {
		user = pw->pw_name;
	}

        // get the host name and put it in a buffer
	char hostname[64];
	hostname[0] = '\0';
	gethostname(hostname, sizeof(hostname));
	
	// retrieve our path and put it into an array of search paths
	// not really needed if we're using execvp... oops
	char* path;
	path = getenv("PATH");
	char* pch = strtok(path, ":");
        while (pch != NULL) {
            strcpy(search[path_len], pch);
            pch = strtok(NULL, ":");
            path_len++;
        }

	while (TRUE) {
        printf("%s@%s$ ", pw->pw_name, hostname);
        fflush(stdout);
       	buffer = accept();
               	
		// make our argv and argc
		char argv[MAX_ARGV][MAX_ARG_LEN];
        int argc = 0;
        char* pch = strtok(buffer, " \n");
        while (pch != NULL) {
            strcpy(argv[argc], pch);
            pch = strtok(NULL, " \n");
            argc++;
        }
        // now try and execute our command
        execute(argv);
	}
	return EXIT_SUCCESS;
}
