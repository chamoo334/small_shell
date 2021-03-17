
#ifndef LIB_LINKEDPROCESSES_H_INCLUDED
#define LIB_LINKEDPROCESSES_H_INCLUDED


typedef struct proc_info{
  int pid;
} Proc_info;

typedef struct node {
    Proc_info *data;
    struct node *next;
} node;

typedef struct node *LinkedList;

void add_node(LinkedList *head, int childPID);
// void remove_node(LinkedList *head, int childPID);
void print_linked_proc(LinkedList current);
void kill_processes(LinkedList);
void free_linked_proc(LinkedList *head);


#endif