static struct _mydrv_wq {
struct list_head mydrv_worklist; /* Work List */
spinlock_t lock; /* Protect the list */
wait_queue_head_t todo; /* Synchronize submitter and worker */
} mydrv_wq;

struct _mydrv_work {
struct list_head mydrv_workitem; /* The work chain */
void (*worker_func)(void *); /* Work to perform */
void *worker_data; /* Argument to worker_func */
/* ... */ /* Other fields */
} mydrv_work;

static int __init
mydrv_init(void)
{
/* Initialize the lock to protect against
concurrent list access */
spin_lock_init(&mydrv_wq.lock);
/* Initialize the wait queue for communication
between the submitter and the worker */
init_waitqueue_head(&mydrv_wq.todo);
/* Initialize the list head */
INIT_LIST_HEAD(&mydrv_wq.mydrv_worklist);
/* Start the worker thread. See Listing 3.4 */
kernel_thread(mydrv_worker, NULL,
CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD);
return 0;
}

int
submit_work(void (*func)(void *data), void *data)
{
struct _mydrv_work *mydrv_work;
/* Allocate the work structure */
mydrv_work = kmalloc(sizeof(struct _mydrv_work), GFP_ATOMIC);
if (!mydrv_work) return -1;
/* Populate the work structure */
mydrv_work->worker_func = func; /* Work function */
mydrv_work->worker_data = data; /* Argument to pass */
spin_lock(&mydrv_wq.lock); /* Protect the list */
/* Add your work to the tail of the list */
list_add_tail(&mydrv_work->mydrv_workitem,
&mydrv_wq.mydrv_worklist);
/* Wake up the worker thread */
wake_up(&mydrv_wq.todo);
spin_unlock(&mydrv_wq.lock);
return 0;
}

static int
mydrv_worker(void *unused)
{
DECLARE_WAITQUEUE(wait, current);
void (*worker_func)(void *);
void *worker_data;
struct _mydrv_work *mydrv_work;
set_current_state(TASK_INTERRUPTIBLE);
/* Spin until asked to die */
while (!asked_to_die()) {
add_wait_queue(&mydrv_wq.todo, &wait);
if (list_empty(&mydrv_wq.mydrv_worklist)) {
schedule();
/* Woken up by the submitter */
} else {
set_current_state(TASK_RUNNING);
}
remove_wait_queue(&mydrv_wq.todo, &wait);
/* Protect concurrent access to the list */
spin_lock(&mydrv_wq.lock);
/* Traverse the list and plough through the work functions
present in each node */
while (!list_empty(&mydrv_wq.mydrv_worklist)) {
/* Get the first entry in the list */
mydrv_work = list_entry(mydrv_wq.mydrv_worklist.next,
struct _mydrv_work, mydrv_workitem);
worker_func = mydrv_work->worker_func;
worker_data = mydrv_work->worker_data;
/* This node has been processed. Throw it out of the list */
list_del(mydrv_wq.mydrv_worklist.next);
kfree(mydrv_work); /* Free the node */
/* Execute the work function in this node */
spin_unlock(&mydrv_wq.lock); /* Release lock */
worker_func(worker_data);
spin_lock(&mydrv_wq.lock); /* Re-acquire lock */
}
spin_unlock(&mydrv_wq.lock);
set_current_state(TASK_INTERRUPTIBLE);
}
set_current_state(TASK_RUNNING);
return 0;
}