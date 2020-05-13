int tsleep(int event);
int twakeup(int event);
int join(int targetPid, int *status);
int texit(int value);


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
	// printf("Test if targetPid exist...\n");
	if (!(tmp = _get_proc(targetPid, zombieList)))
		if (!(tmp = _get_proc(targetPid, readyQueue)))
			if (!(tmp = _get_proc(targetPid, sleepList))) {
				printf("Error: ENOPID no pid %d\n", targetPid);
				return -1; //ENOPID (or maybe the ZOMBIE was catched by others)
			}
	// Test if it has cycle
	// printf("Test if it has cycle...\n");
	if (_has_join_this(tmp, running)){
		printf("Error: EDEADLOCK\n");
		return -1;
	}
	// is it ZOMBIE now? (although this is less likely?)
	// printf("is it ZOMBIE now?\n");
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
	// printf("it is not ZOMBIE now...\n");
	running->joinPtr = tmp;
	running->joinPid = targetPid;
	// printf("It's not zombie now, pid %d gotta sleep to wait %d\n", running->pid, targetPid);
	tsleep(targetPid);
	// if wakeup, tmp must be ZOMBIE or catched by others
	if (!(tmp = _get_proc(targetPid, zombieList))){
		printf("pid %d : shit! pid %d is not in zombieList\n", running->pid, targetPid);
		printAll();
		return 0;
	}

	// printf("pid %d got the zombie %d\n", running->pid, targetPid);
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
