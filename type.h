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
