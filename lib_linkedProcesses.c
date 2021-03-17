
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<signal.h>
#include "lib_linkedProcesses.h"


/***************************************************************
 * add_node
 * Parameters: LinkedList *head, int childPID
 * Creates a node with ChildPID as data and adds to end of
 * linked list
****************************************************************/
void add_node(LinkedList *head, int childPID){
    // create new node with childPID 
    Proc_info *new_add = malloc(sizeof(Proc_info));
    if ( new_add == NULL){
        printf("Unable to allocate memory to add to linked list\n");
        fflush(stdout);
        return;
    }
    new_add->pid = childPID;

    // insert as first if list empty
    if (*head == NULL){
        LinkedList buildList = malloc(sizeof(node));
        buildList->data = new_add;
        buildList->next = NULL;
        *head = buildList;

    } else { // otherwise, find the end of the list and update next pointer
        LinkedList last = *head;
        while (last->next != NULL) {
            last = last->next;
        }

        node *newNode = malloc(sizeof(node));
        newNode->data = new_add;
        newNode->next = NULL;
        last->next = newNode;
    }
}

/***************************************************************
 * print_linked_proc
 * Parameters: LinkedList current
 * Cycles through list and prints pids. 
****************************************************************/
void print_linked_proc(LinkedList current){

    if (current == NULL) {
        printf("Linked list is empty.\n");
        fflush(stdout);
        return;
    }

    printf("The linked list includes:\n");
    fflush(stdout);
    while (current != NULL) {
        printf("%d -> ", current->data->pid);
        fflush(stdout);
        current = current->next;
    }
    printf("NULL\n");
    fflush(stdout);

}

/***************************************************************
 * kill_processes
 * Parameters: LinkedList current
 * Cycle through linked list of pids. If pid is active, terminates
 * with SIGKILL. Displays terminated data info for pid
****************************************************************/
void kill_processes(LinkedList current){
    int child_pid;

    if (current == NULL) {
        return;
    }

    while (current != NULL) {
        child_pid = kill(current->data->pid, SIGKILL);
        current = current->next;
    }
}

/***************************************************************
 * free_linked_proc
 * Parameters: LinkedList *head
 * Cycles through list and free's memory for data and next pointers
****************************************************************/
void free_linked_proc(LinkedList *head){

    if (*head == NULL){
        return;
    }

    LinkedList temp;
    while (*head != NULL) {
        temp = (*head)->next;
        free((*head)->data);
        free(*head);
        (*head) = temp;
    }
}
