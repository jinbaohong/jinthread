#include <stdio.h>
#include "type.h"
PROC proc[NPROC];
PROC *freeList;
PROC *readyQueue;
PROC *sleepList;
PROC *running;
PROC *zombieList;
#include "queue.c"


int init();
int texit(int value);
int do_create();
int do_switch();
int do_exit();
int do_join();

void func(void *parm);
int create(void (*f)(), void *parm);
int scheduler();
int tsleep(int event);
int twakeup(int event);
int join(int targetPid, int *status);
int _has_join_this(PROC *start, PROC *this);
int _has_join_circle(PROC *start);
PROC *_get_proc(int targetPid, PROC *queue);
void task1(void *parm);
void printAll(void);

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

int texit(int status)
{
	PROC *tmp;
	int any;

	// Does anyone wait for running?
	any = 0;
	tmp = sleepList;
	while (tmp) {
		if (tmp->joinPid == running->pid) {
			any = 1;
			break;
		}
		tmp = tmp->next;
	}

	printf("task %d in texit status=%d\n", running->pid, status);
	if (!any) { // No one's waiting ---> exit as free
		running->status = FREE;
		running->priority = 0;
		enqueue(&freeList, running);
		printf("task %d exit normally.\n", running->pid);
	} else { // At least one is waiting ---> exit as zombie
		// This is weird, cause we didn't move this task out of readyQueue.
		running->exitStatus = status;
		running->status = ZOMBIE;
		// enqueue(&zombieList, running);
		printf("task %d becomes ZOMBIE.\n", running->pid);
		twakeup(running->pid);
	}
	printAll();
	tswitch();
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

int tsleep(int event)
{
	running->event = event;
	running->status = SLEEP;
	// rm from readyQueue?
	enqueue(&sleepList, running);
	tswitch();
}

int twakeup(int event)
{
	PROC *tmp, *last;
	int cnt;

	cnt = 0;
	last = NULL;
	tmp = sleepList;
	while(tmp){
		if (tmp->event == event){
			tmp->status = READY;
			printf("task %d wake up task %d\n", running->pid, tmp->pid);

			if (tmp == sleepList){
				enqueue(&readyQueue, dequeue(&sleepList));
				tmp = sleepList;
			} else {
				last->next = tmp->next;
				enqueue(&readyQueue, tmp);
				tmp = last->next;
			}
			cnt++;
		} else {
			last = tmp;
			tmp = tmp->next;
		}
	}
	printAll();
	return cnt; // return number of waken up threads.
}

int join(int targetPid, int *status)
{
	PROC *tmp, *i;
	// Test if targetPid exist
	printf("Test if targetPid exist...\n");
	if (!(tmp = _get_proc(targetPid, zombieList)))
		if (!(tmp = _get_proc(targetPid, readyQueue)))
			if (!(tmp = _get_proc(targetPid, sleepList))) {
				printf("Error: ENOPID no pid %d\n", targetPid);
				return -1; //ENOPID (or maybe the ZOMBIE was catched by others)
			}
	// Test if it has cycle
	printf("Test if it has cycle...\n");
	if (_has_join_this(tmp, running)){
		printf("Error: EDEADLOCK\n");
		return -1;
	}
	// is it ZOMBIE now? (although this is less likely?)
	printf("is it ZOMBIE now?\n");
	if (tmp->status == ZOMBIE) {
		// Clean ZOMBIE
		*status = tmp->exitStatus;
		tmp->status = FREE;
		tmp->priority = 0;
		enqueue(&freeList, tmp);
		if (tmp == zombieList)
			dequeue(&zombieList);
		else {
			i = zombieList;
			while (i->next != tmp)
				i = i->next;
			i->next = tmp->next;
		}

		return tmp->pid;
	}
	// it is not ZOMBIE now, so running have to wait it.
	printf("it is not ZOMBIE now...\n");
	running->joinPtr = tmp;
	running->joinPid = targetPid;
	printf("It's not zombie now, pid %d gotta sleep to wait %d\n", running->pid, targetPid);
	tsleep(targetPid);
	// if wakeup, tmp must be ZOMBIE or catched by others
	if (!(tmp = _get_proc(targetPid, zombieList))){
		printf("pid %d : shit! pid %d is not in zombieList\n", running->pid, targetPid);
		printAll();
		return 0;
	}

	printf("pid %d got the zombie %d\n", running->pid, targetPid);
	running->joinPtr = NULL;
	running->joinPid = 0;
	// clean ZOMBIE
	*status = tmp->exitStatus;
	tmp->status = FREE;
	tmp->priority = 0;
	enqueue(&freeList, tmp);
	if (tmp == zombieList)
		dequeue(&zombieList);
	else {
		i = zombieList;
		while (i->next != tmp)
			i = i->next;
		i->next = tmp->next;
	}
	
	return tmp->pid;

}

int _has_join_this(PROC *start, PROC *this)
{ // start -> ... -> this ? 1 : 0
	PROC *tmp = start;

	while (tmp = tmp->joinPtr)
		if (tmp == this)
			return 1;
	return 0;
}

int _has_join_circle(PROC *start)
{
	PROC *tmp = start;

	while (tmp = tmp->joinPtr)
		if (tmp == start)
			return 1;
	return 0;
}

PROC *_get_proc(int targetPid, PROC *queue)
{
	PROC *tmp;

	tmp = queue;
	while (tmp){
		if (tmp->pid == targetPid)
			return tmp;
		tmp = tmp->next;
	}
	return NULL;
}

void printAll(void)
{
	printf("-----------Debug-info--------\n");
	printList("    readyQueue", readyQueue);
	printList("    freeList", freeList);
	printList("    running", running);
	printList("    sleepList", sleepList);
	printList("    zombieList", zombieList);
	printf("-----------------------------\n");
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
