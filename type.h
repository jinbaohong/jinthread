#define NPROC 9
#define SSIZE 1024
// PROC status
#define FREE 0
#define READY 1
#define SLEEP 2
#define BLOCK 3
#define ZOMBIE 4

typedef struct proc{
	struct proc *next;
	int ksp;
	int pid;
	int priority;
	int status;
	int event;
	int exitStatus;
	int joinPid;
	struct proc *joinPtr;
	int stack[SSIZE];
} PROC;

typedef struct mutex{
	int lock; // mutex lock state: 0 for unlocked, 1 for locked
	PROC *owner; // pointer to owner of mutex; may also use PID
	PROC *queue; // FIFO queue of BLOCKED waiting PROCs
} MUTEX;
