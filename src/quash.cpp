/*
 ============================================================================
 Name        : quash.c
 Author      : Howard Grimberg
 Version     :
 Copyright   : Copyright  201 3 - The University of Kansas
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign/list_of.hpp>
#include <string>
#include <iostream>
#include <vector>
using namespace std;
using namespace boost;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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

// cmdpart contains everything needed to run a command.
class CmdPart {
    public:
    char** argv;
    int argc;
    string input_filename = "";
    string output_filename = "";
    bool is_background = false;
    // a vector of all pids associated with cmd
    vector<int> pids;
};

// holds information on individual jobs
class JobObj {
    public:
    int pid;
    string command;
    int count;
};

// emviroment to pass to execed functions
extern char **environ;
// the current number of jobs and vector holding the jobs
int job_count;
vector<JobObj*> jobs;

// Basic execute function, doesn't exec things in cwd
void execute(char **argv) {
    if (execvpe(argv[0], argv, environ) < 0)
        cerr << "ERROR: failed to exec method " << strerror(errno) << endl;
}

// interrupt function for terminating child
void sigchld_int(int signal) {
    pid_t pid;
    // loop waiting for complete death of process
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        // find the process by the pid and handle it accordingly
        for (int i = 0; i < jobs.size(); i++) {
            if (jobs[i]->pid == pid) {
                cout << endl << "[" << jobs[i]->count << "] " << jobs[i]->pid << " completed " << jobs[i]->command << endl;
                jobs.erase(jobs.begin() + i);
                job_count--;
                break;
            }
        }
    }
}

// setup a simple listner for SIGCHLD
void setup_job_listener() {
    struct sigaction a;

    a.sa_handler = sigchld_int;
    // zero out handler
    sigemptyset(&a.sa_mask);
    // auto restart signal handler
    a.sa_flags = SA_RESTART;

    
    if (sigaction( SIGCHLD, &a, NULL ) == -1) {
        cerr << "ERROR: Failed to init sigchld_int interrupt." << endl;
        exit(1);
    }
}

// Change Dir Function
// Accepts a string that is the entire command
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

// Given a string of a single command and whether it is background exec
// returns a pointer to a CmdPart object with specifics of how it executes
CmdPart* build_command(string buffer, bool background) {

    CmdPart* command = new CmdPart();
    int argc = 0;
    
    // build up a list of chunks
    vector<string> chunks;
    split(chunks, buffer, is_any_of(" "));
    
    // init our argv array. It's a little large, but doesn't hurt
    char** argv = new char*[chunks.size() + 1];
    // loop over our chunks and process io-redirection
    // not perfect, can produce undefined behaviour for poorly foormed input
    for(vector<string>::size_type i = 0; i != chunks.size(); i++) {
        if (chunks[i] == ">") {
            command->output_filename = chunks[i+1];
            break;
        }
    
        if (chunks[i] == "<") {
            command->input_filename = chunks[i+1];
            break;
        }
        
        // normal argv
        argv[argc] = new char[chunks[i].length() + 1];
        strcpy(argv[argc], chunks[i].c_str());
        argc++;
    }
    argv[argc] = NULL;
    
    //Set our command to its contents
    command->argv = argv;
    command->argc = argc;
    command->is_background = background;
    
    // return info object
    return command;
}

// splits apart a string containing pipes
vector<CmdPart*> parse_input(char* list) {
    vector<CmdPart*> parts;
    string s = string(list);
    
    // check for background execution and remove token
    bool bg = false;
    int l = s.find("&");
    if (l != string::npos) {
        bg = true;
        s.erase(l, 1);
    }
    
    // loop over our chunks that are piped
    vector<string> commands;
    split(commands, s, is_any_of("|\n"));
    
    // parse those parts into info chunks
    for(vector<string>::size_type i = 0; i < commands.size(); i++) {
        trim(commands[i]);
        // ignore empty results
        if (commands[i].length() > 0)
            parts.push_back(build_command(commands[i], bg));
    }
    
    return parts;
}

int main(int argc, char **argv, char **envp) {
	// the holder for search paths
	char search[MAX_PATHS][MAX_PATH_LEN];
	int path_len = 0;

	register struct passwd *pw;
	register uid_t uid;

	int dirchange;
    char* user;
    
    // start listening for children procs
    setup_job_listener();

	uid = geteuid();
	pw = getpwuid(uid);
	if (pw) {
		user = pw->pw_name;
	}

	// get the host name and put it in a buffer
	char hostname[64];
	hostname[0] = '\0';
	gethostname(hostname, sizeof(hostname));

	dirchange = -1;
	while (cin.good()) {
		printf("%s@%s$ ", pw->pw_name, hostname);
		fflush(stdout);
		
		// Setup our input and c_str for legacy functionality
        string tmp;
		getline(cin, tmp);
        char *buffer = new char [tmp.length()+1];
        strcpy(buffer, tmp.c_str());
        
        // build a list of commands
        vector<CmdPart*> parts = parse_input(buffer);
        
        // check for builin commands. Pretty sloppy, but it works
        vector<string> builtin = assign::list_of("cd")("set")("jobs")("kill")("exit")("quit");
        bool found = false;
        for (vector<string>::size_type i = 0; i != parts.size(); i++) {
            // search our list of builtins for our command
            if(std::find(builtin.begin(), builtin.end(), parts[i]->argv[0]) != builtin.end()) {
            	// send a kill signal to all children processes
            	if ((string)parts[i]->argv[0] == "kill")
                    exit(0);
                if ((string)parts[i]->argv[0] == "exit" || (string)parts[i]->argv[0] == "quit")
                    exit(0);
                // change directory
                if ((string)parts[i]->argv[0] == "cd")
                    change_dir(buffer);
                // list out the jobs
                if ((string)parts[i]->argv[0] == "jobs") {
                    for (int i = 0; i < jobs.size(); i++) {
                        cout << endl << "[" << jobs[i]->count << "] " << jobs[i]->pid << " -> " << jobs[i]->command << endl;
                    }
                }
                // set some enviromental variables
                if ((string)parts[i]->argv[0] == "set") {
                    // change home
                    if (strncmp(parts[i]->argv[1], "HOME=", 5) == 0) {
                        if (setenv("HOME", &parts[i]->argv[1][5], 1) == -1)
                            cerr << "ERROR: could not set home." << endl;
                    }
                    // path
                    if (strncmp(parts[i]->argv[1], "PATH=", 5) == 0) {
                        if (setenv("PATH", &parts[i]->argv[1][5], 1) == -1)
                            cerr << "ERROR: could not set path." << endl;
                    }
                }
                found = true;
            }
        }
        
        // don't do anythin else since builtins can't be piped or redirected...
        if (found)
            continue;
        
        // holder for stdin and stdout
        int stdin1 = dup(STDIN_FILENO);
        int stdout1 = dup(STDOUT_FILENO);
        
        // the placeholder for chaining
        int inputfd = NULL;
        int fd[2];
        int pid;
        int spid;
        int status;
        vector<int> pids;
        
        // loop over our commands and execute in a pipe chain
        for (vector<string>::size_type i = 0; i != parts.size(); i++) {
            // setup some convenience vars for the edge conditions
            bool last = false;
            bool first = false;
            if (i == parts.size() - 1)
                last = true;
            if (i == 0)
                first = true;
            
            // init our pipe file descriptors if it isn't the only command
            if (!(last && first))
                if (pipe(fd) < 0)
                    cerr << "ERROR: Failed to open pipe." << endl;

			// fork off our new process
            pid = fork();
            if (pid < 0) { // just in case
                cerr << "ERROR: Failed to fork new process " << parts[i]->argv[0] << "." << endl;
            } else if(pid == 0) {
                // CHILD
                // we now want to hook up our pipes properly
                
                // handle connecting to the last commands stdout if needed
                if (!first) {
                    // change stdin to last commands stdout which is set below
                    // the read end of the pipe of the last command.
                    dup2(inputfd, STDIN_FILENO);
                    close(inputfd);
                }
                
                // load up stdout into our placeholder pipe to be passed to
                // next cmd, unless this is the last in which case we leave it
                // in tact
                if (!last) {
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[1]);
                }

                if (parts[i]->is_background == true) {
                    // change child process group to not be child of our shell
                    if (setpgid(0, 0) < 0)
                        cerr << "ERROR: Failed to set pgid." << endl;
                }

                // detect our setup for file redirection and overwrite piping
                // these values were inited when we parsed the command
                if (parts[i]->input_filename.length() > 0) {
                    FILE *f = fopen(parts[i]->input_filename.c_str(), "r");
                    if (f == NULL)
                        cerr << "ERROR: Couldn't open file " << parts[i]->input_filename << " for reading." << endl;

                    // set the stdin for our process
                    dup2(fileno(f), STDIN_FILENO);
                    // close our extra file descriptor
                    fclose(f);
                }
                
                // detect our setup for file redirection and overwrite piping
                if (parts[i]->output_filename.length() > 0) {
                    FILE *f = fopen(parts[i]->output_filename.c_str(), "w");
                    if (f == NULL)
                        cerr << "ERROR: Couldn't open file " << parts[i]->output_filename << " for writing." << endl;
                    
                    // set the stdout for our process
                    dup2(fileno(f), STDOUT_FILENO);
                    // close our extra file descriptor
                    fclose(f);
                }

                // run our command
                execute(parts[i]->argv);
                exit(0);
            } else {
                if (!parts[i]->is_background) {
                    // wait if we're not a background process
                    wait(&status);
                    // sleep to help prevent stdout flush race condition
                    usleep(4000);
                // if in bacground we should save the pids for job info later
                } else
	                pids.push_back(pid);
                
            	// save the pid if it's the first in the chain
                if (first)
                    spid = pid;
            }
            
            if (!last) {
                // start the chaining operation
                close(fd[1]);
                inputfd = fd[0];
            } else {
                if (parts[i]->is_background) {
                    // fill out job information
                    JobObj *job = new JobObj();
                    job->command = buffer;
                    job->count = job_count++;
                    job->pid = spid;
                    job->pids = pids;
                    jobs.push_back(job);
                    cout << "[" << job->count << "] " << job->pid << endl;
                }
            }
        }

        // close the pipes in the parent
        close(fd[0]);
        close(fd[1]);
        
        // reset stdin and stdout
        dup2(stdin1, STDIN_FILENO);
        dup2(stdout1, STDOUT_FILENO);

		// clear our cstring just in case
		memset(buffer, '\0', sizeof(buffer));

	}
	return EXIT_SUCCESS;
}
