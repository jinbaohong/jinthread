#define NPROC 9
#define SSIZE 1024
// PROC status
#define FREE 0
#define READY 1
#define SLEEP 2
#define BLOCK 3
#define PAUSE 4
#define ZOMBIE 5

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
	int time;   // time slice in ticks
	int pause;  // pause time in seconds
	int stack[SSIZE];
} PROC;


