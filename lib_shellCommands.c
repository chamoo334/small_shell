#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include "lib_shellCommands.h"


/***************************************************************
 * exit_command
 * Parameters:
 * Kills all background processes and free's all_proc memory.
 * Sets shellRun to 0 to terminate main loop.
****************************************************************/
void exit_command(void){
    kill_processes(all_proc); // --> lib_linkeProcesses
    free_linked_proc(&all_proc); // --> lib_linkeProcesses
    shellRun = 0; // signal main to terminate
}

/***************************************************************
 * change_directory
 * Parameters: char **newDirectory, int numArgs
 * If numArgs == 1, changes directory to HOME. If numArgs == 2,
 * uses the second element of newDirectory as the path. Otherwise,
 * error due to too many arguments are path not found is displayed.
****************************************************************/
void change_directory(char **newDirectory, int numArgs){
    int completed = -100;

    switch(numArgs){
        // if no path given, cd home
        case 1:
            completed = chdir(getenv("HOME")); //FIX! use getwd per instructions
            break;

        // if path given, cd path
        case 2:
            completed = chdir(newDirectory[1]);
            break;

        //otherwise, too many arguments
        default:
            printf("No matching commands for number of arguments recived.\n");
            fflush(stdout);
            return;
    }

    // notify if path not found
    if (completed != 0){
        printf("Unable to find the request path.\n");
        fflush(stdout);
    }
}

/***************************************************************
 * get_status
 * Parameters:
 * Displays exit or terminating status of last foreground
 * procedure stored in last_fore_proc. [0] -> pid, [1]=1 -> exited
 * normally, [2] -> exit or signal #.
****************************************************************/
void get_status(void){

    if (last_fore_proc[1] == 1){ //exited normally with status
        printf("exited with statud %d\n", last_fore_proc[2]);
        fflush(stdout);
    } else if (last_fore_proc[1] == 2) { // terminated by signal
        printf("terminated by signal %d\n", last_fore_proc[2]);
        fflush(stdout);
    } else { // no foreground process, return 0
        printf("No foreground process: exit status 0\n");
        fflush(stdout);
    }

}

/***************************************************************
 * execute_command
 * Parameters: char **args
 * Creates a new process via fork() and executes requested
 * process via execlp. If fork or execution fails, appropriate
 * message is displayed, exit status set to 1, and created process
 * is terminated. Created process and exit status/signal saved in
 * last_fore_proc.
****************************************************************/
void execute_command(char **args, int total){
    int childStatus, childPid, i;

    pid_t child = fork(); // new process
    switch(child){

        // fork unsuccessful
        case -1:
            printf("fork() failed!\n");
            fflush(stdout);
            exit(1);

        // // fork successful, proceed with execution
        case 0:

            if (background == 1){ //set stdin & stdout for background processes
                background_handler();
            } else if (sigaction(SIGINT, &default_sig, NULL) != 0){ //otherwise, allow ctrl-c for foreground process
                    printf("Unable to alter SIGINT action for foreground child process\n");
                    fflush(stdout);
                    exit(1);
            }
            
            if (r_data.change_in==1 || r_data.change_out==1){ //redirect stdin & stdout
                redirection_handler();
            }

	        if (sigaction(SIGTSTP, &ignore_sig, NULL) != 0){ //ignore ctrl-z for all child processes
                    printf("Unable to alter SIGTSTP action for child process\n");
                    fflush(stdout);
                    exit(1);
            }

            // return exit status 1 if exec unsuccessful
            i = execvp(args[0], args);
            if(i == -1){
                printf("Unable to find the command to run.\n");
                fflush(stdout);
                exit(1);
            }

        default:
            // run as background process, add pid to linked list, and
            // reset background global.
            if(background == 1){
                add_node(&all_proc, child);
                printf("Starting background PID %d.\n", child);
                fflush(stdout);
                background = 0;
            }

            // run as foreground process
            else {
                // block SIGTSTP
                if (sigprocmask(SIG_BLOCK, &tstp_mask, NULL) != 0){
                    printf("Unable to block SIGTSTP\n");
                    fflush(stdout);
                }

                // obtain child status
                childPid = waitpid(child, &childStatus, 0);

                // unblock SIGTSTP
                if (sigprocmask(SIG_UNBLOCK, &tstp_mask, NULL) != 0){
                    printf("Unable to unblock SIGTSTP\n");
                    fflush(stdout);
                }

                // preserve child data and display signal received
                last_fore_proc[0] = child;

                if(WIFEXITED(childStatus)){
                    last_fore_proc[1] = 1;
                    last_fore_proc[2] = WEXITSTATUS(childStatus);
                } 
                else{
                    last_fore_proc[1] = 2;
                    last_fore_proc[2] = WTERMSIG(childStatus);
                    printf("\nterminated by signal %d\n", last_fore_proc[2]);
                    fflush(stdout);
                }
            }

            break;
    }

}

/***************************************************************
 * background_handler
 * Parameters: none
 * Sets STDIN and STDOUT for background processes to dev/null
****************************************************************/
void background_handler(void){
    int tempFD;

    // change input to dev/null
    tempFD = open("/dev/null", O_RDONLY);
    if (tempFD == -1){
        printf("Unable to open /dev/null\n");
        fflush(stdout); 
        exit(1); 
    }
    if (dup2(tempFD, STDIN_FILENO) == -1) { 
        printf("Unable to set STDIN to /dev/null\n");
        fflush(stdout); 
        exit(1); 
    }
    close(tempFD);

    // change output to dev/null
    tempFD = open("/dev/null", O_WRONLY | O_TRUNC);
    if (tempFD == -1){
        printf("Unable to open /dev/null\n");
        fflush(stdout); 
        exit(1); 
    }
    if (dup2(tempFD, STDOUT_FILENO) == -1) { 
        printf("Unable to set STDOUT to /dev/null\n");
        fflush(stdout); 
        exit(1); 
    }
    close(tempFD);
}

/***************************************************************
 * redirection_handler
 * Parameters: none
 * Assumes, global r_data variable contains input and output
 * file data. If boolean members are set, redirects STDIN and/or
 * STDOUT to appropriate files
****************************************************************/
void redirection_handler(void){
    int tempFD = -100;
    int redirFD;

    if(r_data.change_in == 1){ // change stdin
        tempFD = open(r_data.in_file, O_RDONLY);

        if (tempFD == -1){
        printf("Unable to open or create %s for redirection\n", r_data.in_file);
        fflush(stdout);
        exit(1);
        }

        // redirFD = dup2(tempFD, STDIN_FILENO);
        if (dup2(tempFD, STDIN_FILENO) == - 1){
            printf("Redirection to %s failed\n", r_data.in_file);
            fflush(stdout);
            exit(1);
        }

        close(tempFD);
    }


    if(r_data.change_out == 1){ //change stdout
        tempFD = open(r_data.out_file, O_WRONLY | O_TRUNC | O_CREAT, 0777);

        if (tempFD == -1){
        printf("Unable to open or create %s to redirect output\n", r_data.out_file);
        fflush(stdout);
        exit(1);
        }

        // redirFD = dup2(tempFD, STDOUT_FILENO);
        if (dup2(tempFD, STDOUT_FILENO) == - 1){
            printf("Redirection for %s failed\n", r_data.out_file);
            fflush(stdout);
            close(tempFD);
            exit(1);
        }

        close(tempFD);
    }

}