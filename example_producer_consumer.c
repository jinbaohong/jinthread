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
int myexit();
void func(void *parm);
int create(void (*f)(), void *parm);
int scheduler();
int task1(void *parm);

#include "queue.c"
#include "util.c"
#include "wait.c"
#include "mutex.c"
#include "semaphore.c"
#define NBUF 4
#define N 8
int buf[NBUF], head, tail; // buffers for producer-consumer

struct jsem_t full, empty, mutex; // semaphores

void producer() // produce task code
{
	int i;
	printf("producer %d start\n", running->pid);
	for (i=0; i<N; i++){
		printf("     i=%d start\n", i);
		printf("     empty: %d", empty.value);printList(" ; ", empty.queue);
		printf("     full: %d", full.value);printList(" ; ", full.queue);
		jsem_wait(&empty);
		jsem_wait(&mutex);
		buf[head++] = i+1;
		printf("producer %d: item = %d\n", running->pid, i+1);
		head %= NBUF;
		jsem_post(&mutex);
		jsem_post(&full);
		printf("     empty: %d", empty.value);printList(" ; ", empty.queue);
		printf("     full: %d", full.value);printList(" ; ", full.queue);
		printf("     i=%d end\n", i);
	}
	printf("producer %d exit\n", running->pid);
}

void consumer()
{
	int i, c;
	printf("consumer %d start\n", running->pid);
	for (i=0; i<N; i++) {
		printf("     i=%d start\n", i);
		printf("     empty: %d", empty.value);printList(" ; ", empty.queue);
		printf("     full: %d", full.value);printList(" ; ", full.queue);
		jsem_wait(&full);
		jsem_wait(&mutex);
		c = buf[tail++];
		tail %= NBUF;
		printf("consumer %d: got item = %d\n", running->pid, c);
		jsem_post(&mutex);
		jsem_post(&empty);
		printf("     empty: %d", empty.value);printList(" ; ", empty.queue);
		printf("     full: %d", full.value);printList(" ; ", full.queue);
		printf("     i=%d end\n", i);
		printAll();
	}
	printf("consumer %d exit\n", running->pid);
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
		p->next = p+1;
	}
	proc[NPROC-1].next = 0;
	freeList = &proc[0];
	readyQueue = 0;
	sleepList = 0;
	p = running = dequeue(&freeList);
	p->status = READY;
	p->priority = 0;
	printAll();
	// initialize semaphores full, empty, mutex
	head = tail = 0;
	jsem_init(&full, 0);
	jsem_init(&empty, NBUF);
	jsem_init(&mutex, 1);
	printf("init complete\n");
}

int myexit(){ texit(0); }

int task1(void *parm)
{
	int status;
	printf("task %d creates producer-consumer tasks\n", running->pid);
	create((void *)producer, 0);
	create((void *)consumer, 0);
	join(2, &status);
	join(3, &status);

	printf("task %d exit\n", running->pid);
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
	p->stack[SSIZE-2] = (int)myexit; // function return address
	p->stack[SSIZE-3] = (int)f; // function entry
	p->ksp = (int)&p->stack[SSIZE-12]; // ksp -> stack top
	enqueue(&readyQueue, p);
	printAll();
	printf("task %d created a new task %d\n", running->pid, p->pid);
	return p->pid;
}

int main()
{
	printf("Welcome to the MT User-Level Threads System\n");
	init();
	create((void *)task1, 0);
	printf("P0 switch to P1\n");
	tswitch();
	printf("all task ended: P0 loops\n");
	while(1);
}

int scheduler()
{
	if (running->status == READY)
		enqueue(&readyQueue, running);
	else if (running->status == ZOMBIE)
		enqueue(&zombieList, running);
	running = dequeue(&readyQueue);
	printf("next running = %d\n", running->pid);
	printAll();
}
