
int init();
int create(void (*f)(), void *parm);
int scheduler();

void thandler(int sig)
{
	static int ticks = 0; // timer tick
	static int secs;
	if (!(++ticks % 100)){
		secs++;
		tqe_decr();
		// printf("%d\n", secs);
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
	readyQueue = NULL;
	sleepList = NULL;
	zombieList = NULL;
	timerQueue = NULL;

	// Init timerPool
	struct tqe *v, *u;
	timerPool = NULL;
	u = timerPool;
	for (i=0; i<NPROC; i++){
		v = calloc(sizeof(struct tqe), 1);
		if (u)
			u->next = v;
		else
			timerPool = v;
		u = v;
	}

	// Timer mechanism
	signal(SIGALRM, thandler); // install SIGALRM catcher
	struct itimerval t; // configure timer
	t.it_value.tv_sec = 0;
	t.it_value.tv_usec = 10000; // start in 10 msec
	t.it_interval.tv_sec = 0;
	t.it_interval.tv_usec = 10000; // period = 10 msec
	setitimer(ITIMER_REAL, &t, NULL); // start REAL mode timer

	// Protect tqe list
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGALRM);
	
	// create P0 as initial running task
	running = p = dequeue(&freeList);
	p->status = READY;
	p->priority = 0;
	printAll();
	printf("init complete: P0 running\n");
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
