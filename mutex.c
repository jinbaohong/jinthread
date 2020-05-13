typedef struct mutex{
	int lock; // mutex lock state: 0 for unlocked, 1 for locked
	PROC *owner; // pointer to owner of mutex; may also use PID
	PROC *queue; // FIFO queue of BLOCKED waiting PROCs
} MUTEX;


MUTEX *mutex_create();
void mutex_destroy(MUTEX *mp);
int mutex_lock(MUTEX *mp);
int mutex_unlock(MUTEX *mp);


MUTEX *mutex_create() // create a mutex and initialize it
{
	MUTEX *mp = (MUTEX *)calloc(sizeof(MUTEX), 1);
	return mp;
}

void mutex_destroy(MUTEX *mp)
{
	free(mp);
}

int mutex_lock(MUTEX *mp)
{
	if (!mp->lock) {
		mp->lock = 1;
		mp->owner = running;
	}
	else {
		enqueue(&mp->queue, running);
		tswitch();
	}
}

int mutex_unlock(MUTEX *mp)
{
	PROC *tmp;

	if (!mp->lock || (mp->owner != running)) {
		printf("Error: unlock error\n");
		return -1;
	}
	if (tmp = dequeue(&mp->queue)){
		mp->owner = tmp;
		enqueue(&readyQueue, tmp);
	} else {
		mp->lock = 0;
		mp->owner = NULL;
	}
	return 0;
}