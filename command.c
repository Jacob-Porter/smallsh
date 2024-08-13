#include "command.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>



/*
*   Initialize and NULL set CmdPrompt
*/
void initCmd(struct CmdPrompt *cp){
 
    cp->cmd = NULL;
    cp->input_file = NULL;
    cp->output_file = NULL;
    cp->amp = 0;
    cp->arg_size = 0;
    int i;
    for (i = 0; i < MAX_ARGS; i++) {

        cp->args[i] = NULL;
    }
    
}


/*
*   Function to parse a command line into command and arguments, return 0 if no arguments; otherwise returns non-zero
*/
void parseCmd(char *input, struct CmdPrompt *cp) {
    int arg_count = 0; // indexing cp->args

    // Tokenize the input based on spaces
    char *token = strtok(input, " \n");
    while (token != NULL && arg_count < MAX_ARGS) {

        if (arg_count == 0) { // First token is the command and first argument
            cp->cmd = (char*) calloc(strlen(token)+1, sizeof(char));
            strncpy(cp->cmd, token, strlen(token));
            cp->args[arg_count] = (char*) calloc(strlen(token)+1, sizeof(char));
            strncpy(cp->args[arg_count], token, strlen(token));
            arg_count++;
        } else if (strcmp(token, "<") == 0) { // input file check

            token = strtok(NULL, " \n");
            cp->input_file = (char*) calloc(strlen(token)+1, sizeof(char));
            strncpy(cp->input_file, token, strlen(token));
        } 
        else if (strcmp(token, ">") == 0) { // output file check

            token = strtok(NULL, " \n");
            cp->output_file = (char*) calloc(strlen(token)+1, sizeof(char));
            strncpy(cp->output_file, token, strlen(token));
        } 
        else if (strcmp(token, "&") == 0) { // background process check

            cp->amp = 1;
        } 
        else { // all other tokens are arguments

            cp->args[arg_count] = (char *) calloc(strlen(token)+1, sizeof(char));  
            strncpy(cp->args[arg_count], token, strlen(token));

            cp->args[arg_count] = _expansion(cp->args[arg_count]);
            
            arg_count++;
        }
        token = strtok(NULL, " \n");
    }
    cp->arg_size = arg_count;   // set arg_count in struct 
}

/*
*   Empty and NULL set all data in CmdPrompt struct
*/
void resetCmd(struct CmdPrompt *cp) {

    if (cp != NULL){
        if (cp->cmd != NULL) {
            free(cp->cmd);
            cp->cmd = NULL;
        }
        if (cp->input_file != NULL) {
            free(cp->input_file);
            cp->input_file = NULL;
        }
        if (cp->output_file != NULL) {
            free(cp->output_file);
            cp->output_file = NULL;
        }
        int i;
        for (i = 0; i < cp->arg_size; i++) {

            free(cp->args[i]);
            cp->args[i] = NULL;
        }
        cp->amp = 0;
        cp->arg_size = 0;
    }
}

/*
*   variable expansion for "$$"
*/
char* _expansion(char *input) {

    int count = 0, tmp_count = 0, i, j;
    char *new = NULL;
    char pid_str[20];  // estimated max 20 char's for pid of process

    sprintf(pid_str, "%d", getpid()); // pid (int) -> string
    
    for (i = 0; i < strlen(input); i++) {

        if (input[i] == '$') { // found a '$' in "input"
            tmp_count += 1;
        } else {               // nonconsecutive -> reset count
            tmp_count = 0;
        }

        if (tmp_count % 2 == 0 && tmp_count > 0) { // tmp_count tracks consecutive instances of '$', so when we find "$$" increment amount of expansions need to occur (count)

            count += 1;
        }
    }

    int size = strlen(input) + (count * (strlen(pid_str)-2)); // -2 == len("$$")
    new = (char*) calloc(size + 1, sizeof(char));

    count = 0; // reset count
    int input_idx = 0; // keep track of place in original input

    for (i = 0; i < size; i++) { // for size of new string

        // add current char of input to new
        if (input_idx < strlen(input)) {
            new[i] = input[input_idx];

            if (input[input_idx] == '$') { // count consecutive '$' (again)
                count += 1;
            } else {
                count = 0;
            }

        }

        if (count % 2 == 0 && count > 0) { // when "$$" occurs (again)

            // insert pid_str
            for (j = 0; j < strlen(pid_str); j++) {

                new[i-1] = pid_str[j]; // prev element in new (first "$") = jth element of pid_str
                i+=1; // next spot
            }
            i-=2; // len("$$")
        }
        
        input_idx+=1; // next element of "input"
    }

    free(input); // free malloc'd "input"
    return new;
}