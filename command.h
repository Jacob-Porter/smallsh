#include <stdlib.h>

#define MAX_ARGS 512
#define MAX_CMD_LEN 100

struct CmdPrompt
{
    /* data */
    char *cmd;
    char *args[MAX_ARGS];
    char *input_file;
    char *output_file;
    int amp;
    int arg_size;
};

/*
*   Initialize and NULL set CmdPrompt
*/
void initCmd(struct CmdPrompt*);

/*
*   Function to parse a command line into command and arguments, return 0 if no arguments; otherwise returns non-zero
*/
void parseCmd(char*, struct CmdPrompt*);

/*
*   Empty and NULL set all data in CmdPrompt struct
*/
void resetCmd(struct CmdPrompt*);

/*
*   variable expansion for "$$"
*/
char* _expansion(char *);
