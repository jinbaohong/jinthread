#include <stdio.h>
#include <stdlib.h>
#include "type.h"
PROC proc[NPROC];
PROC *freeList;
PROC *readyQueue;
PROC *sleepList;
PROC *running;
PROC *zombieList;

int do_create();
int do_switch();
int do_exit();
int do_join();
void func(void *parm);
void task1(void *parm);
void tswitch(void);

#include "queue.c"
#include "util.c"
#include "base.c"
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

