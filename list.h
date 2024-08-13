#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define TYPE pid_t // used as List of process ID's (pid)

struct List
{
    struct Link* sentinal;
};

struct Link // singly linked List
{
    TYPE val;
    struct Link* next;
};

/*
*   Initialize List with sentinal
*/
void initList(struct List*);

/*
*   Push new elements to front of List
*/
void pushList(struct List*, TYPE);

/*
*   Pop front element from List
*/
void popList(struct List*);

/*
*   Remove specified element from List
*/
void removeList(struct List*, TYPE);

/*
*   Empty and free all Links from List
*/
void emptyList(struct List*);

/*
*   Empty and free entire List, including sentinal (on exit of program)
*/
void freeList(struct List*);