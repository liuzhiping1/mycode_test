#include <linux/workqueue.h>
struct workqueue_struct *wq;
/* Driver Initialization */
static int __init
mydrv_init(void)
{
/* ... */
wq = create_singlethread_workqueue("mydrv");
return 0;
}
/* Work Submission. The first argument is the work function, and
the second argument is the argument to the work function */
int
submit_work(void (*func)(void *data), void *data)
{
struct work_struct *hardwork;
hardwork = kmalloc(sizeof(struct work_struct), GFP_KERNEL);
/* Init the work structure */
INIT_WORK(hardwork, func, data);
/* Enqueue Work */
queue_work(wq, hardwork);
return 0;
}