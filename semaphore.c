struct jsem_t {
	int value;
	PROC *queue;
};


int jsem_init(struct jsem_t *sem, unsigned int value);
int jsem_destroy(struct jsem_t *sem);
int jsem_post(struct jsem_t *sem);
int jsem_wait(struct jsem_t *sem);


int jsem_init(struct jsem_t *sem, unsigned int value)
{
	sem->value = value;
	sem->queue = NULL;
	return 0;
}

int jsem_destroy(struct jsem_t *sem)
{
	free(sem);
}

int jsem_post(struct jsem_t *sem)
{
	PROC *tmp;

	sem->value++;
	if (tmp = dequeue(&sem->queue)){
		enqueue(&readyQueue, tmp);
	}
	return 0;
}

int jsem_wait(struct jsem_t *sem)
{
	while (1){
		if (sem->value > 0) {
			sem->value--;
			return 0;
		}
		running->status = BLOCK;
		enqueue(&sem->queue, running);
		tswitch();
	}
}
