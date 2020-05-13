#include <stdio.h>
#include <stdlib.h>
#include "type.h"
PROC proc[NPROC];
PROC *freeList;
PROC *readyQueue;
PROC *sleepList;
PROC *running;
PROC *zombieList;

int init();
int do_create();
int do_switch();
int do_exit();
int do_join();
void func(void *parm);
int create(void (*f)(), void *parm);
int scheduler();
void task1(void *parm);

#include "queue.c"
#include "util.c"
#include "wait.c"
#include "mutex.c"



int main()
{
	int i, pid, status;
	printf("Welcome to the MT User-Threads System\n");
	init();
	create((void *)task1, 0);
	printf("P0 switch to P1\n");
	tswitch();
	printf("All tasks ended: P0 loops\n");

	while(1);
}


void task1(void *parm) // task1: demonstrate create-join operations
{
	int pid[2];
	int i, status;
	//printf("task %d create subtasks\n", running->pid);
	printAll();
	for (i=0; i<2; i++){ // P1 creates P2, P3
		pid[i] = create(func, (void*)running->pid);
	}
	join(5, &status);    // try to join with targetPid=5
	for (i=0; i<2; i++){ // try to join with P2, P3
		pid[i] = join(pid[i], &status);
		printf("task %d joined with task %d: status = %d\n",
			running->pid, pid[i], status);
	}
}

int init()
{
	int i, j;
	PROC *p;
	for (i=0; i<NPROC; i++){
		p = &proc[i];
		p->pid = i;
		p->priority = 0;
		p->status = FREE;
		p->event = 0;
		p->joinPid = 0;
		p->joinPtr = 0;
		p->next = p+1;
	}
	proc[NPROC-1].next = 0;
	freeList = &proc[0]; // all PROCs in freeList
	readyQueue = 0;
	sleepList = 0;
	zombieList = 0;
	// create P0 as initial running task
	running = p = dequeue(&freeList);
	p->status = READY;
	p->priority = 0;
	printAll();
	printf("init complete: P0 running\n");
}


int do_create()
{
	int pid = create(func, (void *)running->pid); // parm = pid
}

int do_switch()
{
	tswitch();
}

int do_exit()
{
	texit(running->pid); // for simplicity: exit with pid value
}

int do_join()
{
	int status;
	char buf[8];

	printf("enter a pid to join with : ");
	fgets(buf, 8, stdin);
	printf("task %d try to join with task %d:", running->pid, atoi(buf));
	join(atoi(buf), &status);
}

void func(void *parm) // subtasks: enter q to exit
{
	char c;
	printf("task %d start: parm = %d\n", running->pid, (int)parm);
	while(1){
		printAll();
		printf("task %d running\n", running->pid);
		printf("enter a key [c|s|q|j]: ");
		c = getchar(); getchar(); // kill \r
		switch (c){
			case 'c' : do_create(); break;
			case 's' : do_switch(); break;
			case 'q' : do_exit(); break;
			case 'j' : do_join(); break;
		}
	}
}

int create(void (*f)(), void *parm)
{
	int i;
	PROC *p = dequeue(&freeList);
	if (!p){
		printf("create failed\n");
		return -1;
	}
	p->status = READY;
	p->priority = 1;
	p->joinPid = 0;
	p->joinPtr = 0;

	// initialize new task stack for it to resume to f(parm)
	for (i=1; i<13; i++) // zero out stack cells
		p->stack[SSIZE-i] = 0;
	p->stack[SSIZE-1] = (int)parm; // function parameter
	p->stack[SSIZE-2] = (int)do_exit; // function return address
	p->stack[SSIZE-3] = (int)f; // function entry
	p->ksp = (int)&p->stack[SSIZE-12]; // ksp -> stack top
	enqueue(&readyQueue, p);
	printAll();
	printf("task %d created a new task %d\n", running->pid, p->pid);
	return p->pid;
}


int scheduler()
{
	if (running->status == READY)
		enqueue(&readyQueue, running);
	else if (running->status == ZOMBIE)
		enqueue(&zombieList, running);
	running = dequeue(&readyQueue);
	printf("next running = %d\n", running->pid);
}
