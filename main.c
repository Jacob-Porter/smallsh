#include "command.h"
#include "list.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>


#define INPUT_LENGTH 2048
static int status = 0;
static volatile sig_atomic_t foreground_only = 0;

/*
*   BUILT-IN: process "cd" command
*/
int _cd (struct CmdPrompt *command) {

    if (command->args[1] == NULL) {    
        char *home = getenv("HOME");
        chdir(home);
    }
    else {
        chdir(command->args[1]);
        return 1;
    }

    return 0;
}

/*
*   BUILT-IN: process "status" command
*/
void _print_status (int stat) {

    if (WIFEXITED(stat)) { // exit code

        printf("exit value %d\n", WEXITSTATUS(stat));
        return;
    } else if (WIFSIGNALED(stat)) { // signal

        printf("terminated by signal %d\n", WTERMSIG(stat));
        return;
    }

    return;
}

/*
*   Check pidList for children that have terminated and print who has
*/
void checkPIDList (struct List *list) {

    struct Link *lnk = list->sentinal->next, *prev = lnk;

    int childStatus;

    while (lnk != NULL)
    {
        pid_t out = waitpid(lnk->val, &childStatus, WNOHANG); // check process state
        if (out != 0) { // if exited

            printf("background pid %d is done: ", lnk->val);
            _print_status(childStatus);
            status = childStatus;
            
            lnk = lnk->next;
            removeList(list, prev->val);
            prev = lnk;
        } else {
            lnk = lnk->next;
            prev = lnk;
        }
    }
}

/*
*   Go through entire pidList and check if pid is still running, if so kill it
*/
void killAllProcesses (struct List *list) {

    struct Link *lnk = list->sentinal->next;

    int childStatus;

    while (lnk != NULL)
    {
        pid_t out = waitpid(lnk->val, &childStatus, WNOHANG); // check if still running/exited
        if (out != 0) {
            kill(lnk->val, 9);
            lnk = lnk->next;
        } else {
            lnk = lnk->next;
        }
    }
}

/*
*   Handler for SIGNINT
*/
static void handle_SIGINT(int signo){

    status = signo;
    exit(signo);
}

/*
*   Handler for SIGTSTP
*/
static void handle_SIGTSTP(int signo){

    // toggle
    if (foreground_only == 1) {
        char* message = "Exiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        foreground_only = 0;
    } else {
        char* message = "Entering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        foreground_only = 1;
    }
}

/*
*   Check if command should be run through BUILT-IN "programs"
*/
int commandPreCheck(struct CmdPrompt *command, struct List *pidList) {

    if (command->cmd == NULL) {
            return 1;
    } else if (command->cmd[0] == '#') {
        // do nothing
        resetCmd(command);
        return 1;
    } else if (strcmp(command->cmd, "cd") == 0) {
        // change directory
        _cd(command);
        resetCmd(command);
        return 1;
    } else if (strcmp(command->cmd, "status") == 0) {
        // get status
        _print_status(status);
        resetCmd(command);
        return 1;
    } else if (strcmp(command->cmd, "exit") == 0) {
        // close shell
        killAllProcesses(pidList);
        freeList(pidList);
        resetCmd(command);
        return EXIT_SUCCESS;
        }

    return -1;
}

/*
*   Signal setup for Parent (shell), SIGINT and SIGTSTP
*/
void parent_SignalSetup() {

    // Signal setup
    struct sigaction sigTSTP = {{0}}, sigINT = {{0}};


    // MASK
    sigset_t mask;
    if (sigfillset(&mask) == -1) { // block all signals (for when handler is running)

        printf("Setting mask failed\n");
        exit(EXIT_FAILURE);
    }

    // SIGTSTP
    sigTSTP.sa_handler = &handle_SIGTSTP;
    // Block all catchable signals while handle_SIGINT is running
    sigTSTP.sa_mask = mask;
    if (sigaction(SIGTSTP, &sigTSTP, NULL) == -1) {
        perror("Couldn't set SIGTSTP handler");
        exit(EXIT_FAILURE);
    }  

    // SIGINT
    sigINT.sa_handler = SIG_IGN;
    // Block all catchable signals while handle_SIGINT is running
    sigINT.sa_mask = mask;
    if (sigaction(SIGINT, &sigINT, NULL) == -1) {
        perror("Couldn't set SIGINT handler");
        exit(EXIT_FAILURE);
    } 
}

/*
*   Signal setup for Child, SIGINT and SIGTSTP
*/
void child_SignalSetup(struct CmdPrompt *command) {

    struct sigaction ignore_action = {{0}}, _sigINT = {{0}};

    // MASK
    sigset_t mask;
    if (sigfillset(&mask) == -1) { // block all signals (for when handler is running)

        printf("Setting mask failed\n");
        exit(EXIT_FAILURE);
    }

    // ALL children ignore SIGTSTP (ctrl+Z)
    ignore_action.sa_handler = SIG_IGN;
    ignore_action.sa_mask = mask;
    if (sigaction(SIGTSTP, &ignore_action, NULL) == -1) {
        perror("Couldn't set SIGINT handler");
        exit(EXIT_FAILURE);
    }
    
    if (foreground_only == 1 || command->amp == 0) { // foreground child
        _sigINT.sa_handler = &handle_SIGINT;
        _sigINT.sa_mask = mask;
        if (sigaction(SIGINT, &_sigINT, NULL) == -1) {
            perror("Couldn't set SIGINT handler");
            exit(EXIT_FAILURE);
        }
    } else { // background child (ignore SIGINT)

        _sigINT.sa_handler = SIG_IGN;
        _sigINT.sa_mask = mask;
        if (sigaction(SIGINT, &_sigINT, NULL) == -1) {
            perror("Couldn't set SIGINT handler");
            exit(EXIT_FAILURE);
        }
    }
}

/*
*   check for input and outout files and change default file directories respectively
*/
void child_redirectionCheck(struct CmdPrompt *command) {

    if (command->input_file != NULL) {  // input file

        int targetFD = open(command->input_file, O_RDONLY);
        if (targetFD == -1) {
            printf("cannot open %s for input\n", command->input_file);
            exit(1);
        }

        // stdin swaps with inputfile
        int result = dup2(targetFD, 0);
        if (result == -1) {
            perror("dup2"); 
            exit(2); 
        }
        close(targetFD);
    }
    if (command->output_file != NULL) { // output file

        int targetFD = open(command->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (targetFD == -1) {
            perror("open()");
            exit(1);
        }

        // stdout swaps with output file
        int result = dup2(targetFD, 1);
        if (result == -1) {
            perror("dup2"); 
            exit(2); 
        }
        close(targetFD);
    }
}


int main() {
    char input[INPUT_LENGTH];

    // list of all running child processes
    struct List pidList;
    initList(&pidList);

    // command
    struct CmdPrompt command;
    initCmd(&command);

    // shells signal setup
    parent_SignalSetup();

    // start of smallsh
    printf("$ smallsh\n");

    while (1) {  

        // check if any children have exited and clean them
        checkPIDList(&pidList);

        // Print prompt and read user input
        printf(": ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            continue;  // go back to top of while loop if nothing
        }

        // Parse the command line
        parseCmd(input, &command);

        int check;
        check = commandPreCheck(&command, &pidList);
        if (check == 1) { // ran BUILT-IN "program", go back to top of while loop
            continue;
        } else if (check == EXIT_SUCCESS) { // ran "exit", close program

            return EXIT_SUCCESS;
        }

        // flush for child and shell
        fflush(stdout);

        // Fork a child process to execute the command
        pid_t pid = fork();

        // error check
        if (pid < 0) {
            perror("fork()");
            exit(EXIT_FAILURE);
        }

        switch (pid)
        {
        case 0: ;       // !! CHILD !!

            // foreground child dies on SIGINT (ctrl+C), background ignore SIGINT
            // foreground and background child ignore SIGTSTP
            child_SignalSetup(&command);
            
            // check for input/output file, redirect stdin/out respectively
            child_redirectionCheck(&command);
            
            // run command with arguments
            execvp(command.cmd, command.args);
            printf("%s: no such file or directory\n", command.cmd);  // execvp returns only if an error occurs
            exit(EXIT_FAILURE);
            break;
        default: ;     // !! PARENT !!
        
            // Parent process: wait for the child to terminate if no "&" OR foreground only mode 
            if (foreground_only == 1 || command.amp == 0) {   // FOREGROUND
                waitpid(pid, &status, 0);
                if (WIFSIGNALED(status)) { // if signaled
                    _print_status(status);
                }
            } else if (command.amp == 1) {  // BACKGROUND

                pushList(&pidList, pid);
                printf("background pid is %d\n", pid);
            }
            
            // don't wait for child to terminate if "&"
            break;
        }

        // empty command for next user input
        resetCmd(&command);
    }

    return 0;
}
