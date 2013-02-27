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

#define _PROGRAM_NAME "whoami"
#define TRUE (!FALSE)
#define FALSE (0)
#define BUFFER_LENGTH 2048


char line[BUFFER_LENGTH];

char* accept(void) {

	read(0, line, BUFFER_LENGTH);

	return line;
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

	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, sizeof(hostname));

	printf("%s@%s:", user, hostname);

//buffer = accept();

	while (buffer) {
		printf(buffer);
		buffer = accept();
	}

//	while (TRUE) {
//		printf("Enter Something:");
//		p = gets(line);
//		if (p == NULL ) {
//
//		} else {
//			printf(p);
//		}
//
//	}
//	char* pPath;
//	  pPath = getenv ("PATH");
//	  if (pPath!=NULL)
//	    printf ("The current path is: %s",pPath);
//
//	  char* token;
//	  while ((token = strsep(&pPath, ":")) != NULL)
//	   {
//		 ;
//	     printf("%s\n", token);
//	   }
	return EXIT_SUCCESS;
}
