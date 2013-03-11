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

//Change Dir Function
//Accepts a string and changes dir.
int change_dir(char *buffer) {
	int err_stat = 0;
	char safe_buffer[2048];
	strcpy(safe_buffer, buffer);
	char* pch = strtok(safe_buffer, " \n");

	if (strncmp(pch, "cd", 2) == 0 || strncmp(pch, "chdir", 5) == 0) {
		pch = strtok(NULL, " \n");
		if (pch == NULL ) {

			errno = 0;

			err_stat = chdir(getenv("HOME"));
			if (err_stat < 0) {

				printf("%s\n", strerror(errno));
				return FALSE;

			} else {

				return TRUE;
			}
		} else if (strncmp(pch, "/", 1) == 0) {
			//Handle Absolute Path
			errno = 0;

			err_stat = chdir(pch);

			if (err_stat < 0) {
				printf("%s\n", strerror(errno));
				return FALSE;
			} else {
				return TRUE;
			}
		} else {
			errno = 0;

			char dirbuf[MAX_PATH_LEN];
			getcwd(dirbuf, sizeof(dirbuf));
			strcat(dirbuf, "/");
			strcat(dirbuf, pch);
			err_stat = chdir(dirbuf);
			if (err_stat < 0) {
				printf("%s\n", strerror(errno));
				return FALSE;
			} else {
				return TRUE;
			}

		}

	}
	return FALSE;
}

char* accept(char* line) {

	read(0, line, BUFFER_LENGTH);
	return line;
}


//Basic execute function. Ripped straigh from the old source
void execute(char ***argv, int background, char **env) {
	pid_t pid;

	int status;

	if ((pid = fork()) < 0) {
		printf("*** ERROR: forking child process failed\n");
		exit(1);
	} else if (pid == 0) {
		errno = 0;

		if (execvpe(**argv, *argv, env) < 0) {
			printf("%s\n", strerror(errno));
			exit(1);
		}
	} else {
		if (background == FALSE)
			while (wait(&status) != pid)
				;
		else
			printf("[1] %i\n", pid);
	}

	return;
}

//Execute with just a redirect out.
void execute_redirect_out(char ***argv, int background, char **env, char* path) {

	pid_t pid;

	int status;

	if ((pid = fork()) < 0) {
		printf("*** ERROR: forking child process failed\n");
		exit(1);
	} else if (pid == 0) {
		errno = 0;
		FILE *fp;
		fp = fopen(path, "w");
		dup2(fileno(fp), STDOUT_FILENO);
		fclose(fp);
		if (execvpe(**argv, *argv, env) < 0) {
			printf("%s\n", strerror(errno));
			exit(1);
		}
	} else {
		if (background == FALSE)
			while (wait(&status) != pid)
				;
		else
			printf("[1] %i\n", pid);
	}

	return;
}

//Execute with a redirect
void execute_redirect_in(char ***argv, int background, char **env, char* path) {

	pid_t pid;

	int status;

	if ((pid = fork()) < 0) {
		printf("*** ERROR: forking child process failed\n");
		exit(1);
	} else if (pid == 0) {
		errno = 0;
		FILE *fp;
		fp = fopen(path, "r");
		dup2(fileno(fp), STDIN_FILENO);
		fclose(fp);
		if (execvpe(**argv, *argv, env) < 0) {
			printf("%s\n", strerror(errno));
			exit(1);
		}
	} else {
		if (background == FALSE)
			while (wait(&status) != pid)
				;
		else
			printf("[1] %i\n", pid);
	}

	return;
}

//Execute two commands with a pipe
// Arguments
// argv1 - arguments for command 1
// background1 - 1/0 to indicate wether to run in backround
// env1 - enviroment for command 1
void execute_piped(char ***argv1, int background1, char **env1, char ***argv2,
		int background2, char **env2) {
			
	//Init. PID Containers		
	pid_t pid, pid2;
	
	//Init. Status Containers
	int status, status2;
	
	//Pipe Container
	int plumbing[2];

	//Pipe It
	pipe(plumbing);
	
	//Fork and stop if we get an error
	if ((pid = fork()) < 0) {
		printf("*** ERROR: forking child process failed\n");
		exit(1);
	} else if (pid == 0) {
		//We are in a fork(child process)
		errno = 0;
		//Close unused end of pipe
		close(plumbing[0]);
		
		//replace STDDOUT with write end of pipe
		dup2(plumbing[1], STDOUT_FILENO);

		//Execute command 1
		if (execvpe(**argv1, *argv1, env1) < 0) {
			printf("%s\n", strerror(errno));
			exit(1);
		}
	} else {
		//First proecss cannot be in  backround
		if (background1 == FALSE) {

		}

		else {
			printf("[1] %i\n", pid);
		}
	}

	//Do same as above only set STDIN to read end of pipe.
	if ((pid2 = fork()) < 0) {
		printf("*** ERROR: forking child process 2 failed\n");
		exit(1);
	} else if (pid2 == 0) {
		errno = 0;

		close(plumbing[1]);
		dup2(plumbing[0], STDIN_FILENO);
		if (execvpe(**argv2, *argv2, env2) < 0) {
			printf("%s\n", strerror(errno));
			exit(1);
		}

	} else {
		if (background2 == FALSE) {

		}

		else {
			printf("[1] %i\n", pid);
		}
	}
	
	//Close pipe completely
	close(plumbing[1]);
	close(plumbing[0]);
	
	//Wait for child2 to finish before we return the call.
	while (waitpid(pid2, &status, 0) != pid2) {
	};
	return;
}

void execute_piped_out_redir(char ***argv1, int background1, char **env1,
		char ***argv2, int background2, char **env2, char* path) {
	pid_t pid, pid2;
	int status, status2;
	int plumbing[2];

	pipe(plumbing);

	if ((pid = fork()) < 0) {
		printf("*** ERROR: forking child process failed\n");
		exit(1);
	} else if (pid == 0) {
		errno = 0;
		close(plumbing[0]);
		dup2(plumbing[1], STDOUT_FILENO);

		if (execvpe(**argv1, *argv1, env1) < 0) {
			printf("%s\n", strerror(errno));
			exit(1);
		}
	} else {
		if (background1 == FALSE) {

		}

		else {
			printf("[1] %i\n", pid);
		}
	}

	if ((pid2 = fork()) < 0) {
		printf("*** ERROR: forking child process 2 failed\n");
		exit(1);
	} else if (pid2 == 0) {
		errno = 0;

		close(plumbing[1]);
		dup2(plumbing[0], STDIN_FILENO);
		FILE *fp;
		fp = fopen(path, "w");
		dup2(fileno(fp), STDOUT_FILENO);
		fclose(fp);
		if (execvpe(**argv2, *argv2, env2) < 0) {
			printf("%s\n", strerror(errno));
			exit(1);
		}

	} else {
		if (background2 == FALSE) {

		}

		else {
			printf("[1] %i\n", pid);
		}
	}
	close(plumbing[1]);
	close(plumbing[0]);

	while (waitpid(pid2, &status, 0) != pid2) {
	};
	return;
}

//What kind of commands are available. 
//This is a relic of my shattered dream to write my own parser.
typedef enum {
	REG, PIPE, REDIR_IN, REDIR_OUT
} cmd_type;


//Command contains everything needed to run a command.
//This is a relic of my shattered dream to write my own parser.
typedef struct {
	char** argv;
	int argc;
	char **envv;
	cmd_type com_type;
	int is_background;
	char target[MAX_PATH_LEN];
} Command;


//Given a string of a single command with arguments and an enviroment,
//returns a pointer to a Command that has been mallocaed
Command* build_regular_command(char* buffer, char **env) {
	
	//Temporary stuff
	char safe_buffer[256];
	char* pch;
	char** argv = (char**) malloc(sizeof(char*) * MAX_ARGV);
	char** argv2;
	int argc = 0;
	int background;
	char* command_buffer;

	Command* command;

	command = (Command*) malloc(sizeof(Command));
	
	
	//Copy buffer to safe_buffer to avoid messing with other stuff
	strcpy(safe_buffer, buffer);
	
	//Tokenize on spaces
	pch = strtok(safe_buffer, " \n");
	while (pch != NULL ) {
		//Malloc a new string
		command_buffer = (char*) malloc((strlen(pch) * sizeof(char)) + 1);
		
		//Zero it out
		memset(command_buffer, '\0', (strlen(pch) * sizeof(char)) + 1);
		
		//Copy the contents of our token into command_buffer
		strcpy(command_buffer, pch);
		
		//Set the argv array to the command_buffer pointer freshly malloced
		argv[argc] = command_buffer;
		
		//Set command_buffer pointer to null for next iteration
		command_buffer = NULL;
		
		//Tokenize again
		pch = strtok(NULL, " \n");
		argc++;
	}

	// Run detection for background operation
	if (argc != 0) {
		if (strcmp(argv[argc - 1], "&") == 0) {
			background = TRUE;
			argv[argc - 1][0] = '\0';
		} else {
			background = FALSE;
		}
	}
	
	//Trims the emoty slots of the argv array to avoid causing headaches 
	//when running commands with different amounts of arguments
	//I lost 3 hours until I realized this.
	argv2 = (char**) malloc(sizeof(char*) * (argc + 1));
	int i = 0;
	for (i = argc; i < MAX_ARGV - 1; i++) {

		argv[argc] = '\0';
	}
	
	//Set our command to its contents
	command->argc = argc;
	command->is_background = background;
	command->argv = argv;
	command->com_type = REG;
	command->envv = env;
	
	//Return the pointer to the malloced Command
	return command;
}

int main(int argc, char **argv, char **envp) {
// the line they enter
	char line[BUFFER_LENGTH];

// the holder for search paths
	char search[MAX_PATHS][MAX_PATH_LEN];
	int path_len = 0;

	register struct passwd *pw;
	register uid_t uid;
	char **env;
	env = envp;

	int dirchange;

	char* pipeptr;
	char* inptr;
	char* outptr;
	char* user;
	Command* basic;
	Command* firstC;
	Command* SecondC;
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
	/*char* path;
	 path = getenv("PATH");
	 char* pch = strtok(path, ":");
	 while (pch != NULL) {
	 strcpy(search[path_len], pch);
	 pch = strtok(NULL, ":");
	 path_len++;
	 }*/
	dirchange = -1;
	while (TRUE) {
		printf("%s@%s$ ", pw->pw_name, hostname);
		fflush(stdout);
		
		//Make sure our line/buffer is indeed empty
		memset(line, '\0', sizeof(line));
		
		//Set line to stuff from terminal
		//Also set buffer to alias line
		buffer = accept(line);
		
		//Check for various things in the line
		//to determine what combination of commands to run
		pipeptr = strpbrk(buffer, "|");
		inptr = strpbrk(buffer, "<");
		outptr = strpbrk(buffer, ">");

		//Handle exit command
		if (strncmp(buffer, "exit", 4) == 0
				|| strncmp(buffer, "quit", 4) == 0) {
			return EXIT_SUCCESS;
		}
		
		//Handle CD command
		if (strncmp(buffer, "cd", 2) == 0) {
			change_dir(buffer);
		} else {
			//If we have an inbound redirect, nothing else can happen.
			if (inptr != NULL ) {
				char* pch = strtok(buffer, "<\n");
				char first[256];
				strcpy(first, pch);
				char second[256];
				pch = strtok(NULL, "<\n");
				strcpy(second, pch);

				basic = build_regular_command(first, env);
				execute_redirect_in(&(basic->argv), basic->is_background,
						basic->envv, second);

				memset(first, '\0', sizeof(char) * 256);
				memset(second, '\0', sizeof(char) * 256);
				memset(buffer, '\0', sizeof(buffer));
				free(basic);
				basic = NULL;
			} else {
				//Otherwise, figure out what combo we have and run it.
				if (pipeptr != NULL && outptr != NULL ) {
					//Redirect out AND pipe
					
					//Tokenize once on pipe ( } )
					char* pch = strtok(buffer, "|\n");
					char first[256];
					strcpy(first, pch);
					char second[256];
					pch = strtok(NULL, "|\n");
					strcpy(second, pch);

					char third[256];
					
					//Tokenzie twice on >
					//Being sure to copy stuff to buffers
					//to avoid tampering with the pointer
					pch = strtok(second, ">\n");
					strcpy(third, pch);
					char fourth[256];
					pch = strtok(NULL, ">\n");
					strcpy(fourth, pch);
					
					
					//Build our commands
					firstC = build_regular_command(first, env);

					SecondC = build_regular_command(third, env);
					
					
					//Execute everything
					execute_piped_out_redir(&(firstC->argv),
							firstC->is_background, firstC->envv,
							&(SecondC->argv), SecondC->is_background,
							SecondC->envv, fourth);
							
					//Free and clear stuff
					free(firstC);
					free(SecondC);
					firstC = NULL;
					SecondC = NULL;
					memset(first, '\0', sizeof(char) * 256);
					memset(second, '\0', sizeof(char) * 256);
					memset(third, '\0', sizeof(char) * 256);
					memset(fourth, '\0', sizeof(char) * 256);
					memset(buffer, '\0', sizeof(buffer));
				} else if (pipeptr != NULL && outptr == NULL ) {
					//Just a pipe

					char* pch = strtok(buffer, "|\n");
					char first[256];
					strcpy(first, pch);
					char second[256];
					pch = strtok(NULL, "|\n");
					strcpy(second, pch);
					firstC = build_regular_command(first, env);

					SecondC = build_regular_command(second, env);

					execute_piped(&(firstC->argv), firstC->is_background,
							firstC->envv, &(SecondC->argv),
							SecondC->is_background, SecondC->envv);
					free(firstC);
					free(SecondC);
					firstC = NULL;
					SecondC = NULL;
					memset(first, '\0', sizeof(char) * 256);
					memset(second, '\0', sizeof(char) * 256);
					memset(buffer, '\0', sizeof(buffer));

				} else if (pipeptr == NULL && outptr != NULL ) {

					char* pch = strtok(buffer, ">\n");
					char first[256];
					strcpy(first, pch);
					char second[256];
					pch = strtok(NULL, ">\n");
					strcpy(second, pch);

					basic = build_regular_command(first, env);
					execute_redirect_out(&(basic->argv), basic->is_background,
							basic->envv, second);

					memset(first, '\0', sizeof(char) * 256);
					memset(second, '\0', sizeof(char) * 256);
					memset(buffer, '\0', sizeof(buffer));
					free(basic);
					basic = NULL;

					//Just an outbound redirect
				} else if (pipeptr == NULL && outptr == NULL ) {
					basic = build_regular_command(buffer, env);

					execute(&(basic->argv), basic->is_background, basic->envv);
					memset(basic, 0, sizeof(*basic));
					memset(buffer, '\0', sizeof(buffer));
					free(basic);
					basic = NULL;

				}
			}
		}
		
		//Clear buffer/line again. Just to be sure
		memset(buffer, '\0', sizeof(buffer));
		memset(line, '\0', sizeof(line));

	}
	return EXIT_SUCCESS;
}
