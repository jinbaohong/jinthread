int _has_join_this(PROC *start, PROC *this);
int _has_join_circle(PROC *start);
PROC *_get_proc(int targetPid, PROC *queue);
void printAll(void);

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
