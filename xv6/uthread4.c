#include "types.h"
#include "stat.h"
#include "user.h"

/* Possible states of a thread; */
#define FREE        0x0
#define RUNNING     0x1
#define RUNNABLE    0x2
#define WAIT        0x3

#define STACK_SIZE  8192
#define MAX_THREAD  10

typedef struct thread thread_t, *thread_p;
typedef struct mutex mutex_t, *mutex_p;

struct thread {
  int        sp;                /* saved stack pointer */
  char stack[STACK_SIZE];       /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE, WAIT */
  int        tid;    /* thread id */
  int        ptid;  /* parent thread id */
};

static thread_t all_thread[MAX_THREAD];
thread_p  current_thread;
thread_p  next_thread;
extern void thread_switch(void);
static void thread_schedule(void);
static void mythread(void);

void 
thread_init(void)
{

  // main() is thread 0, which will make the first invocation to
  // thread_schedule().  it needs a stack so that the first thread_switch() can
  // save thread 0's state.  thread_schedule() won't run the main thread ever
  // again, because its state is set to RUNNING, and thread_schedule() selects
  // a RUNNABLE thread.
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
  current_thread->tid = 0;
  current_thread->ptid = 0;
  uthread_init((int)thread_schedule);
}

int 
thread_create(void (*func)())
{
  thread_p t;

  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == FREE) break;
  }
  t->sp = (int) (t->stack + STACK_SIZE);   // set sp to the top of the stack
  t->sp -= 4;                              // space for return address
  /* 
    set tid and ptid
  */
  * (int *) (t->sp) = (int)func;           // push return address on stack
  t->sp -= 32;                             // space for registers that thread_switch expects

  t->tid = t - all_thread;
  t->ptid = current_thread->tid;

  t->state = RUNNABLE;

  return t->tid;
}

static void 
thread_join(int tid)
{
  thread_p t = &all_thread[tid];

  if (t->state == FREE) return;

  while (t->state != FREE) {
    current_thread->state = WAIT;
    thread_schedule();
  }
  
  printf(1, "thread_join: thread %d exited\n", tid);
}

static void 
child_thread(void)
{
  int i;
  printf(1, "child thread running\n");
  for (i = 0; i < 100; i++) {
    printf(1, "[tid : %d] child thread 0x%x\n", current_thread->tid, (int) current_thread);
  }
  printf(1, "child thread: exit\n");
  current_thread->state = FREE;

// wake parent thread here
for (thread_p t = all_thread; t < all_thread + MAX_THREAD; t++) {
  if (t->state == WAIT && t->tid == current_thread->ptid) {
    t->state = RUNNABLE;
  }
}

// 스케줄러 호출 전에 체크
int any_runnable = 0;
for (thread_p t = all_thread; t < all_thread + MAX_THREAD; t++) {
  if (t->state == RUNNABLE) {
    any_runnable = 1;
    break;
  }
}
if (any_runnable) {
  thread_schedule();
}
}

static void 
mythread(void)
{
  int i;
  int tid[5];
  //
  printf(1, "my thread running\n");

  for (i = 0; i < 5; i++) {
    tid[i]=thread_create(child_thread);
  }
  
  for (i = 0; i < 5; i++) {
    thread_join(tid[i]);
  }
  
  printf(1, "my thread: exit\n");
  current_thread->state = FREE;
  thread_schedule();
}

static void thread_schedule(void)
{
  thread_p t;

  // 부모 쓰레드 깨우기
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == WAIT) {
      int waiting_tid = -1;
      // 해당 부모 쓰레드가 기다리는 자식이 아직 살아있는지 확인
      for (thread_p c = all_thread; c < all_thread + MAX_THREAD; c++) {
        if (c->ptid == t->tid && c->state != FREE) {
          waiting_tid = c->tid; // 아직 살아있는 자식 있음
          break;
        }
      }

      // 자식이 전부 종료되었으면 부모를 깨운다
      if (waiting_tid == -1) {
        printf(1, "[scheduler] Waking parent tid=%d\n", t->tid);
        t->state = RUNNABLE;
      }
    }
  }

  // 🔁 기존 스케줄링 로직
  next_thread = 0;
  for (t = all_thread; t < all_thread + MAX_THREAD; t++) {
    if (t->state == RUNNABLE && t != current_thread) {
      next_thread = t;
      break;
    }
  }

  if (t >= all_thread + MAX_THREAD &&
      (current_thread->state == RUNNABLE || current_thread->state == RUNNING)) {
    if (current_thread != &all_thread[0]) {
      next_thread = current_thread;
    }
  }

  if (next_thread == 0) {
    uthread_init(0);
    printf(2, "thread_schedule: no runnable threads\n");
    exit();
  }

  if (current_thread != next_thread) {
    next_thread->state = RUNNING;

    if (current_thread != &all_thread[0] &&
        current_thread->state != FREE &&
        current_thread->state != WAIT)
      current_thread->state = RUNNABLE;

    thread_switch();
  } else {
    next_thread = 0;
  }
}

int 
main(int argc, char *argv[]) 
{
  int tid;
  thread_init();
  tid = thread_create(mythread);
  thread_join(tid);
  return 0;
}
