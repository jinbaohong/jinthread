#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include "type.h"
PROC proc[NPROC];
PROC *freeList;
PROC *readyQueue;
PROC *sleepList;
PROC *running;
PROC *zombieList;
struct tqe *timerQueue;
struct tqe *timerPool;

sigset_t sigmask, oldmask;


struct tqe {
	struct tqe *next;
	PROC *proc;
	int time;
	void (*action)(PROC*);
};

int do_create();
int do_switch();
int do_exit();
int do_timer(int arg, void (action)());
void fuck(void);
void func(void *parm);
void task1(void *parm);
void tswitch(void);
int tqe_decr(void);
int tqe_add(int time, void (action)());
int printTimequeue(char *name, struct tqe *p);


#include "queue.c"
#include "util.c"
#include "base.c"
#include "wait.c"
#include "mutex.c"




void sig(PROC *proc)
{
	printf("from sig: task %d 's time has expired!\n", proc->pid);
}

int tqe_decr(void)
{
	if (!timerQueue) // No timer is counting down.
		return 0;
	if (!--timerQueue->time) { // time coun down to zero
		// Run time-up action
		timerQueue->action(timerQueue->proc);
		// Remove timerQueue's head, then add it to timerPool
		struct tqe *tmp = timerQueue;
		timerQueue = timerQueue->next;
		memset(tmp, 0, sizeof(struct tqe));
		tmp->next = timerPool;
		timerPool = tmp;
	}
	return 0;
}

int tqe_add(int time, void (action)())
{
	struct tqe *new, *tmp, *last;

	if (!(new = timerPool)){
		printf("Error: timerPool is empty\n");
		return -1;
	}
	timerPool = timerPool->next; // maintain timerPool
	// Set tqe
	new->next = NULL;
	new->proc = running;
	new->time = time;
	new->action = action;

	/* Find appropriate position for new */
	if (!timerQueue){
		timerQueue = new;
		return 0;
	}
	tmp = timerQueue;
	last = NULL;
	while ( tmp && new->time > tmp->time) {
		new->time -= tmp->time;
		last = tmp;
		tmp = tmp->next;
	}
	printAll();
	/* Install new into timerQueue */
	if (!tmp) {// new is tail of timerQueue
		last->next = new;
	}
	else if (!last) { // new is head of timerQueue
		timerQueue = new;
		new->next = tmp;
		tmp->time -= new->time;
	}
	else {
		last->next = new;
		new->next = tmp;
		tmp->time -= new->time;
	}

	return 0;
}

int printTimequeue(char *name, struct tqe *p)
{
	printf("%s = ", name);
	while(p){
		printf("[%d %d]->", p->proc ? p->proc->pid : 0, p->time);
		p = p->next;
	}
	printf("NULL\n");
}

// int _wakeup_by_pid(int pid);
// int do_pause(int arg)
// {
// 	tqe_add(arg, )
// }

int do_timer(int arg, void (action)())
{ // after arg secs, run action()
	sigprocmask(SIG_BLOCK, &sigmask, &oldmask);
	tqe_add(arg, action);
	sigprocmask(SIG_UNBLOCK, &sigmask, &oldmask);
}

int do_ps()
{
	printAll();
}

int do_exit()
{
	texit(running->pid); // for simplicity: exit with pid value
}

int menu() // command menu: to be expanded later
{
	printf("*********** menu **************\n");
	printf("* create switch exit ps timer *\n");
	printf("*******************************\n");
}



void func(void *parm) // task function
{
	int arg;
	char line[64], cmd[16];
	printf("task %d start: parm = %d\n", running->pid, (int)parm);
	while(1){
		printf("task %d running\n", running->pid);
		menu();
		printf("enter a command line: ");
		fgets(line, 64, stdin);
		line[strlen(line)-1] = 0; // kill \n at end of line
		sscanf(line, "%s %d", cmd, &arg);
		if (strcmp(cmd, "create")==0)
		create((void *)func, 0);
		else if (strcmp(cmd, "switch")==0)
		tswitch();
		else if (strcmp(cmd, "exit")==0)
		do_exit();
		else if (strcmp(cmd, "ps")==0)
		do_ps();
		else if (strcmp(cmd, "timer")==0)
		do_timer(arg, sig);
		// else if (strcmp(cmd, "pause")==0)
		// do_pause(arg);
	}
}

int main()
{
	printf("Welcome to the MT multitasking system\n");
	init();
	for (int i=1; i<5; i++) // create tasks
		create(func, 0);
	printf("P0 switch to P1\n");
	while(1){
		if (readyQueue)
			tswitch();
	}
}
