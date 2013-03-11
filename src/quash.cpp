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
#include <stdlib.h>
#include <string.h>
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
    friend ostream& operator<<(ostream& os, const CmdPart& dt) {
        os << "Command: " << dt.argv[0] << "\nfout: " << dt.output_filename << "\nfin: " << dt.input_filename << "\nbg: " << dt.is_background << endl;
        return os;
    }
};

// holds information on individual jobs
class JobObj {
    public:
    int pid;
    string command;
    int count;
};

extern char **environ;
int job_count;
vector<JobObj*> jobs;

//Basic execute function. Ripped straigh from the old source
void execute(char **argv) {
    if (execvpe(argv[0], argv, environ) < 0)
        cerr << "ERROR: failed to exec method " << strerror(errno) << endl;
}

void sigchld_int(int signal) {
    pid_t pid;
    // loop waiting for complete death of process
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        // find the process by the pid and handle it accordingly
        for (int i = 0; i < jobs.size(); i++) {
            if (jobs[i]->pid == pid) {
                cout << endl << "[" << jobs[i]->count << "] " << jobs[i]->pid << " completed " << jobs[i]->command << endl;
                jobs.erase(jobs.begin() + i);
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

//Given a string of a single command with arguments and an enviroment,
//returns a pointer to a Command that has been mallocaed
CmdPart* build_command(string buffer, bool background) {

    CmdPart* command = new CmdPart();
    int argc = 0;
    
    // build up a list of chunks
    vector<string> chunks;
    split(chunks, buffer, is_any_of(" "));
    
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
    
    //cout << "Command: " << command->argv[0] << "\nfout: " << command->output_filename << "\nfin: " << command->input_filename << "\nbg: " << command->is_background << endl;
    //Return the pointer to the malloced Command
    return command;
}

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
    
    for(vector<string>::size_type i = 0; i < commands.size(); i++) {
        trim(commands[i]);
        // ignore empty results
        CmdPart *temp;
        if (commands[i].length() > 0) {
            temp = build_command(commands[i], bg);
            //cout << "Command: " << temp->argv[0] << "\nfout: " << temp->output_filename << "\nfin: " << temp->input_filename << "\nbg: " << temp->is_background << endl;
            parts.push_back(temp);
        }
    }
    
    return parts;
}

int main(int argc, char **argv, char **envp) {
	// the line they enter
	char line[BUFFER_LENGTH];

	// the holder for search paths
	char search[MAX_PATHS][MAX_PATH_LEN];
	int path_len = 0;

	register struct passwd *pw;
	register uid_t uid;

	int dirchange;
    char* user;
    
    // populate our environ variable
    /*environ = envp;
    for (; *environ != 0; environ++)
    {
        char* thisEnv = *environ;
        printf("%s\n", thisEnv);
    }*/
    
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
		
		//Make sure our line/buffer is indeed empty
		memset(line, '\0', sizeof(line));
		
		//Set line to stuff from terminal
		//Also set buffer to alias line
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
                if( (string)parts[i]->argv[0] == "exit" || (string)parts[i]->argv[0] == "quit" )
                    exit( 0 );
                if ((string)parts[i]->argv[0] == "cd")
                    change_dir(buffer);
                if ((string)parts[i]->argv[0] == "jobs") {
                    for (int i = 0; i < jobs.size(); i++) {
                        cout << endl << "[" << jobs[i]->count << "] " << jobs[i]->pid << " -> " << jobs[i]->command << endl;
                    }
                }
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
                //cout << getenv ("PATH") << endl;
                //cout << get_current_dir_name() << endl;
                found = true;
            }
        }
        
        if (found)
            continue; // don't do anythin else since builtins can't be piped or redirected...
        
        // holder for stdin and stdout
        int stdin = dup(STDIN_FILENO);
        int stdout = dup(STDOUT_FILENO);
        
        // Start with input from STDIN.
        int inputfd = NULL;
        int fd[2];
        int pid;
        int spid;
        int status;
        
        // loop over our commands and execute properly
        for(vector<string>::size_type i = 0; i != parts.size(); i++) {
            // setup some convenience vars
            bool last = false;
            bool first = false;
            if (i == parts.size() - 1)
                last = true;
            if (i == 0)
                first = true;
                
            
            //cout << "Starting to execute " << parts[i]->argv[0] << endl;
            
            // Create the pipe.
            if (!(last && first)) // if it's not the only one
                if (pipe(fd) < 0)
                    cerr << "ERROR: Failed to open pipe." << endl;

            pid = fork();
            if(pid < 0) { // just in case
                cerr << "ERROR: Failed to fork new process " << parts[i]->argv[0] << "." << endl;
            } else if(pid == 0) {
                // CHILD
                // we now want to hook up our pipes properly
                
                // handle connecting to the last commands stdout if needed
                // input will always be stdin for first command
                if (!first) {
                    //cerr << "Notice: piping input from " << parts[i-1]->argv[0] << " to " << parts[i]->argv[0] << endl;
                    // Rename STDIN for this child to inputfd, which should be
                    // the read end of the pipe of the last command.
                    dup2(inputfd, STDIN_FILENO);
                    close(inputfd);
                }
                
                // load up stdout into our placeholder pipe to be passed to next unless last
                if (!last) {
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[1]);
                }

                if (parts[i]->is_background == true) {
                    // Put child in new process group.
                    if (setpgid(0, 0) < 0)
                        cerr << "ERROR: Failed to set pgid." << endl;
                }

                // detect our setup for file redirection and overwrite piping
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
                //cout << "Command: " << parts[i]->argv[0] << "\nfout: " << parts[i]->output_filename << "\nfin: " << parts[i]->input_filename << "\nbg: " << parts[i]->is_background << endl;
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
                if (first)
                    spid = pid;
                
                // Save the pid of the first child for background job reporting.
                if (!parts[i]->is_background) {
                    // Parent waits for child to finish.
                    wait(&status);
                    usleep(4000);
                }
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
                    jobs.push_back(job);
                    cout << "[" << job->count << "] " << job->pid << endl;
                }
            }
        }


        // Clean up the pipes, but check if they are valid file descriptors first.
        close(fd[0]);
        close(fd[1]);
        
        // Make sure STDIN and STDOUT go back to normal in case the pipes screwed something up.
        dup2(stdin, STDIN_FILENO);
        dup2(stdout, STDOUT_FILENO);
            
        next:
        
		//Clear buffer/line again. Just to be sure
		memset(buffer, '\0', sizeof(buffer));
		memset(line, '\0', sizeof(line));

	}
	return EXIT_SUCCESS;
}
