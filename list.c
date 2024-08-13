#include "list.h"
#include "assert.h"

/*
*   Initialize List with sentinal
*/
void initList(struct List* list) {

    struct Link* sentinal = (struct Link*) malloc(sizeof(struct Link));
    assert(sentinal != NULL);

    sentinal->next = NULL;

    list->sentinal = sentinal;
}

/*
*   Push new Links to front of List
*/
void pushList(struct List* list, TYPE val) {

    struct Link* new = (struct Link*) malloc(sizeof(struct Link));
    assert(new != NULL);

    new->val = val;

    new->next = list->sentinal->next; /* old top attached to new */

    list->sentinal->next = new; /* new attached to sentinal */
}

/*
*   Pop front Link from List
*/
void popList(struct List* list) {

    struct Link* lnk = list->sentinal->next;

    if (lnk != NULL) {
        list->sentinal->next = lnk->next;
        free(lnk);
    }
}

/*
*   Remove specified Link from List
*/
void removeList(struct List* list, TYPE val) {

    struct Link* lnk = list->sentinal->next, *prev = list->sentinal;

    while (lnk != NULL) {

        if (lnk->val == val) {

            prev->next = lnk->next;
            free(lnk);
            return;
        }

        lnk = lnk->next;
        prev = prev->next;
    }
}

/*
*   Empty and free all Links from List
*/
void emptyList(struct List *list) {

    struct Link *lnk = list->sentinal->next, *prev = lnk;

    while (lnk != NULL) {

        lnk = lnk->next;
        free(prev);
        prev = lnk;
    }
}

/*
*   Empty and free entire List, including sentinal (on exit of program)
*/
void freeList(struct List *list) {

    emptyList(list);

    free(list->sentinal);
}