/***************************************************************
 * Author: Chantel Moore
 * Class: CS 344
 * Date: 2/8/2021
 * Description: Program to mimc linux shell with built in cd,
 * exit, and status (of last foreground process). smallsh loops
 * for user input, expanding variables, parsing, and determining 
 * execution route. lib_shellCommands contains cd, exit, status, 
 * and other executions. lib_linkedProcesses contains functions 
 * for terminating tracked background child process.
 * TODO: Remove node in check_backgroundPIDs(), reduce global vars
****************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "lib_shellCommands.h"


/***************************************************************
 * Global variable definitions
****************************************************************/
int shellRun = 1; // boolean to terminate shell
LinkedList all_proc = NULL; //tracker of background pids
int last_fore_proc[3] = {-100, 0, 0}; // last foreground pid, exit/term, signal/value
int background = 0; // run as background boolean
int backgroundPermitted = 1; // background permitted boolean
redir r_data; //struct for tracking redirection
struct sigaction ignore_sig = {0}, default_sig = {0}, tstp_sig = {0}; //sigaction structs
sigset_t tstp_mask; //signal set for blocking


void get_command(char **);
void command_action(char *);
char* expand_variable(char **);
int number_integers(int);
void parse_command(char *, int*, char ***);
void check_backgroundPIDs(void);
void clean_redir_data(void);
void sigtstp_handler(int);

int main(void){

    char *command = NULL;
    r_data.change_in = 0;
    r_data.change_out = 0;

    // for alternating sigtstp
    sigemptyset(&tstp_mask);
    sigaddset(&tstp_mask, SIGTSTP);

    // default signal sigaction struct
    default_sig.sa_handler = SIG_DFL;
    sigfillset(&default_sig.sa_mask);
	default_sig.sa_flags = SA_RESTART;

    // ignore signal sigaction struct
    ignore_sig.sa_handler = SIG_IGN; 
    sigfillset(&ignore_sig.sa_mask);
	ignore_sig.sa_flags = SA_RESTART;

    // tstp signal handling sigaction struct
    tstp_sig.sa_handler = sigtstp_handler;
	sigfillset(&tstp_sig.sa_mask);
	tstp_sig.sa_flags = SA_RESTART;

    // Update SIGTSTP to tstp_sig handler
    if (sigaction(SIGTSTP, &tstp_sig, NULL) != 0){
        printf("Unable to alter SIGTSTP action for shell\n");
        fflush(stdout);
        exit(1);
    }

    // ignore SIGINT in shell
    if (sigaction(SIGINT, &ignore_sig, NULL) != 0){
        printf("Unable to alter SIGINT action for shell\n");
        fflush(stdout);
        exit(1);
    }

    // main loop to mimic shell
    do{
        // check for completed background processes
        check_backgroundPIDs();

        // obtain command
        get_command(&command);

        // ignore if blank or comment
        if ((strcmp(command, "") == 0) || (command[0] == '#')){
            continue;
        }

        // proceed with execution
        else{
            command_action(command);
        }

        free(command);
        command = NULL;

    } while(shellRun == 1);

    free(command);
    command = NULL;

    return 0;
}

/***************************************************************
 * get_command
 * Parameters: char ** returnInput
 * Obtains command to execute and updates returnInput pointer
 * for main() execution
****************************************************************/
void get_command(char **returnInput){
    char *input = NULL;
    size_t len = 0;
    ssize_t read = 0;

    printf("user@smallsh: ");
    fflush(stdout);
    read = getline(&input, &len, stdin);
    input[strlen(input)-1] = '\0'; // replace \n with \0

    *returnInput = input;
}

/***************************************************************
 * command_action
 * Parameters: char *userInput
 * Calls expand_variable, parses command, and executes command.
 * exit, cd, and status commands handled in program while other
 * commands are handled as either a foreground or background
 * process
****************************************************************/
void command_action(char *userInput){
    char *altered = expand_variable(&userInput); //expand $$ variable to  shell pid
    if (altered == NULL){
        return;
    }

    char **parsed = malloc(1 * sizeof(char *));
    int totalParsed=0, x;
    char *copyString = strdup(altered);
    char **test = NULL;
    int i;

    // parse the command and determine execution route
    parse_command(altered, &totalParsed, &parsed);

    if(strcmp(altered, "exit") == 0){exit_command();}

    if (parsed != NULL){

        if (strcmp(parsed[0], "exit") == 0){
            exit_command(); //-->lib_shellCommands
        }

        else if (strcmp(parsed[0], "cd") == 0){
            change_directory(parsed, totalParsed); //-->lib_shellCommands
        }

        else if (strcmp(parsed[0], "status") == 0){
            get_status(); //-->lib_shellCommands
        }

        else{
            execute_command(parsed, totalParsed); //-->lib_shellCommands
        }

    }
    else {
        printf("Unable to parse command.\n");
        fflush(stdout);
    }

    // free memory

    if (r_data.change_in==1 || r_data.change_out==1){ //cleanup memory for redirection
        clean_redir_data();
    }

    for(x=0;x<=totalParsed;x++){
        free(parsed[x]);
    }
       
    free(parsed);
    free(copyString);
    free(altered);
}

/***************************************************************
 * expand_variable
 * Parameters: char **string
 * Cycles through string and replaces every occurence of $$ with
 * the shell's process ID.
****************************************************************/
char* expand_variable(char **string){
    int i=0, pidLength = number_integers(getpid());
    int cLength = strlen(*string);
    char *copy = (char *)malloc((cLength+1)*sizeof(char));
    strcpy(copy, *string);
    char *tempCopy = NULL;

    // cycle through string to replace $$ with pid
    while (copy[i+1] != '\0'){
        
        // $$ found and needs updating
        if(copy[i]=='$' && copy[i+1]=='$'){

            // update characters for formatted printing
            copy[i] = '%';
            copy[i+1] = 'd';
            
            // calculate new string length and allocate memory
            cLength = cLength + pidLength + 2;
            char *tempCopy = (char *)malloc(cLength*sizeof(char));

            if (tempCopy == NULL){ // exit if memory not allocated
                printf("Unable to allocate memory to replace $$.\n");
                fflush(stdout);

                free(copy);
                copy = NULL;
                return NULL;
            }

            // string print formatted and update copy
            tempCopy[0] = '\0';
            sprintf(tempCopy, copy, getpid());
            free(copy);
            copy = tempCopy;

            i += 2; 
        } else{ i += 1; }

    }

    // if last character is &, update background boolean
    if (copy[strlen(copy)-1] == '&'){

        char *temp = strndup(copy, strlen(copy)-1);
        free(copy);
        copy = strdup(temp);
        free(temp);
        temp = NULL;

        if (backgroundPermitted == 1){ //verify not in foreground only mode
            background = 1; // set to run as background process
        }
    }
    
    return copy;
}

/***************************************************************
 * number_integers
 * Parameters: int a
 * Returns the length of integer a
****************************************************************/
int number_integers(int a){
    int total = 0;

    while(a > 0){
        a /= 10;
        ++total;
    }

    return total;
}

/***************************************************************
 * parse_command
 * Parameters: char *string, int *total (expected value = 0)
 * Tokenizes string using space as a delimiter and increments
 * total for each string. Saves every substring in an array and 
 * returns array.
****************************************************************/
void parse_command(char *string, int *total, char ***returnArgs){
    int x;
    char *remaining;
    char *test = NULL;

    // tokenize string
    char *token = strtok_r(string, " ", &remaining);

    while(token){

        // check for input redirection
        if(strcmp(token, "<") == 0){
            r_data.change_in = 1;
            token = strtok_r(NULL, " ", &remaining);
            r_data.in_file = strdup(token);
            token = strtok_r(NULL, " ", &remaining);
        }

        // check for output redirection
        else if(strcmp(token, ">") == 0){
            r_data.change_out = 1;
            token = strtok_r(NULL, " ", &remaining);
            r_data.out_file = strdup(token);
            token = strtok_r(NULL, " ", &remaining);
        }

        // otherwise save command and continue
        else {
            (*returnArgs)[*total] = strdup(token);
            *total += 1;

            // continue parsing and allocate more memory
            token = strtok_r(NULL, " ", &remaining);
            *returnArgs = realloc(*returnArgs, (*total + 1) * sizeof(char *));

            if (*returnArgs== NULL){ //print memory error and return null
                printf("Unable to allocate enough memory for command.\n");
                fflush(stdout);
                return;
            }

            (*returnArgs)[*total] = NULL;
        }

    }

}

/***************************************************************
 * check_backgroundPIDs
 * Parameters: None
 * Finds child processes with changed status and displays
 * information if any found.
****************************************************************/
void check_backgroundPIDs(void){
    int childPID, childStatus;

    if(all_proc == NULL){ return; }

    do{
        // find any child process with changed status
        childPID = waitpid(-1, &childStatus, WNOHANG);

        // display exit or signal termination data
        if (childPID > 0){
            printf("Background PID %d terminated ", childPID);
            fflush(stdout);

            if(WIFEXITED(childStatus)){
                printf("with exit value %d.\n", WEXITSTATUS(childStatus));
                fflush(stdout);
            } 
            else{
                printf("by signal %d.\n", WTERMSIG(childStatus));
                fflush(stdout);
            }
            // remove node
        }
    } while(childPID > 0);
}

/***************************************************************
 * clean_redir_data
 * Parameters: None
 * Clears memory and resets booleans of r_data
****************************************************************/
void clean_redir_data(void){
    if (r_data.change_in == 1){ //clean input redirection
        r_data.change_in = 0;
        free(r_data.in_file);
        r_data.in_file = NULL;
    }
    
    if (r_data.change_out == 1){ //clean output redirection
        r_data.change_out = 0;
        free(r_data.out_file);
        r_data.out_file = NULL;
    }
}

/***************************************************************
 * sigtstp_handler
 * Parameters: int sig_num
 * SIGTSTP triggers foreground-only mode on/off by updating
 * backgroundPermitted boolean.
****************************************************************/
void sigtstp_handler(int sig_num){
    if (backgroundPermitted == 1){
        char *message = "\nEntering foreground-only mode\n";
        write(STDOUT_FILENO, message, 31);
        backgroundPermitted = 0;
    } else {
        char *message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        backgroundPermitted = 1;
    }
}
