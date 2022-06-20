/*
Race-Averse Scheduler Scheduling Class
Implemented by WeimingZhang
Last revised on May 26th
*/

#include "sched.h"
#include <linux/slab.h>

void init_ras_rq(struct ras_rq *ras_rq, struct rq *rq)
{
  // initial the running queue, label it ras
  INIT_LIST_HEAD(&ras_rq->queue);
  ras_rq->ras_nr_running = 0;
}

#define ras_entity_is_task(ras_se) (!(ras_se)->my_q)

static inline struct task_struct *ras_task_of(struct sched_ras_entity *ras_se)
{
#ifdef CONFIG_SCHED_DEBUG
  WARN_ON_ONCE(!ras_entity_is_task(ras_se));
#endif
  // get the task_struct of the ras scheduling entity
  return container_of(ras_se, struct task_struct, ras);
}

static inline struct rq *rq_of_ras_rq(struct ras_rq *ras_rq)
{
  // get the running queue
  return ras_rq->rq;
}

static inline struct ras_rq *ras_rq_of_se(struct sched_ras_entity *ras_se)
{
  // get the ras running queue of the ras scheduling entity
  struct task_struct *tsk = ras_task_of(ras_se);
  struct rq *rq = task_rq(tsk);
  return &rq->ras;
}
/*
NO NEED FOR PRIO IMPLEMENTATION
static inline int ras_se_prio(struct sched_ras_entity *ras_se) {
  return ras_task_of(ras_se)->rt_priority;
}
*/

static inline int on_ras_rq(struct sched_ras_entity *ras_se)
{
  // whether the ras running queue exists
  return !list_empty(&ras_se->run_list);
}

static void update_curr_ras(struct rq *rq)
{
  // just as that in rt.c
  struct task_struct *curr = rq->curr;
  // struct sched_ras_entity *ras_se = &curr->ras;
  // struct ras_rq *ras_rq = ras_rq_of_se(ras_se);
  u64 delta_exec;

  if (curr->sched_class != &ras_sched_class)
    return;
  delta_exec = rq->clock_task - curr->se.exec_start;
  if (unlikely((s64)delta_exec < 0))
    delta_exec = 0;
  schedstat_set(curr->se.statistics.exec_max,
                max(curr->se.statistics.exec_max, delta_exec));
  curr->se.sum_exec_runtime += delta_exec;
  account_group_exec_runtime(curr, delta_exec);

  curr->se.exec_start = rq->clock_task;
  cpuacct_charge(curr, delta_exec);
}

static void enqueue_ras_entity(struct rq *rq, struct sched_ras_entity *ras_se,
                               bool head)
{
  struct list_head *queue = &(rq->ras.queue);
  struct task_struct *p;
  p = container_of(ras_se, struct task_struct, ras);
  p->ras.time_slice = 10;
  if (head)
    list_add(&ras_se->run_list, queue);
  else
    list_add_tail(&ras_se->run_list, queue);
  ++rq->ras.ras_nr_running;
}

static void enqueue_task_ras(struct rq *rq, struct task_struct *p, int flags)
{
  printk("Enqueue An New RAS Task!\n");
  struct sched_ras_entity *ras_se = &p->ras;
  enqueue_ras_entity(rq, ras_se, flags & ENQUEUE_HEAD);
  inc_nr_running(rq);
}

static void dequeue_ras_entity(struct rq *rq, struct sched_ras_entity *ras_se)
{
  list_del_init(&ras_se->run_list);
  rq->ras.ras_nr_running--;
}

static void dequeue_task_ras(struct rq *rq, struct task_struct *p, int flags)
{
  printk("Dequeue An RAS Task!\n");
  struct sched_ras_entity *ras_se = &p->ras;
  update_curr_ras(rq);
  dequeue_ras_entity(rq, ras_se);
  dec_nr_running(rq);
}

static void requeue_task_ras(struct rq *rq, struct task_struct *p, int head)
{
  printk("Requeue The Task!\n");
  struct sched_ras_entity *ras_se = &p->ras;
  struct ras_rq *ras_rq = &rq->ras;
  struct list_head *queue = &rq->ras.queue;
  // move the current running task to the tail of the queue
  if (head)
    list_move(&ras_se->run_list, queue);
  else
    list_move_tail(&ras_se->run_list, queue);
}

static void yield_task_ras(struct rq *rq)
{
  printk("Yield An RAS Task!\n");
  requeue_task_ras(rq, rq->curr, 0);
}

static struct task_struct *pick_next_task_ras(struct rq *rq)
{
  if (unlikely(!rq->ras.ras_nr_running))
    return NULL;
  struct ras_rq *ras_rq = &rq->ras;
  struct task_struct *p;
  struct sched_ras_entity *head;

  head = list_first_entry(&rq->ras.queue, struct sched_ras_entity, run_list);
  p = container_of(head, struct task_struct, ras);
  if (!p)
    return NULL;
  p->se.exec_start = rq->clock_task;
  return p;
}

static void put_prev_task_ras(struct rq *rq, struct task_struct *p)
{
  // printk("Put Previous RAS Task!\n");
  update_curr_ras(rq);
  p->se.exec_start = 0;
}

// static task_fork_ras(struct task_struct *p) {
//   p->ras.time_slice = p->ras.parent->time_slice;
// }

static void set_curr_task_ras(struct rq *rq)
{
  // i do not clearly understand the function of it in the project, quq...I just
  // let it go.
  printk("Set Current RAS Task!\n");
  struct task_struct *p = rq->curr;
  p->se.exec_start = rq->clock_task;
}

static void task_tick_ras(struct rq *rq, struct task_struct *p, int queued)
{
  // THE MOST IMPORTANT!!
  struct sched_ras_entity *ras_se = &p->ras;
  struct list_head *queue = &rq->ras.queue;
  update_curr_ras(rq);
  if (p->policy != SCHED_RAS)
    return;
  printk(
      "!task_tick||cpu: %d pid: %d time_slice: %d nr_running: %d wcounts: %d\n",
      cpu_of(rq), p->pid, p->ras.time_slice, rq->ras.ras_nr_running,
      p->wcounts);

  if (--p->ras.time_slice)
    return;
  // My strategy: use an inverse function to realize wcounts of [0,INF) to race
  // probability of [0,10]
  p->ras.race_prob = 11 * p->wcounts / (p->wcounts + 2000);
  // And then use a function to get timeslice of [10,100]
  p->ras.time_slice = 100 - 9 * p->ras.race_prob;
  if (ras_se->run_list.prev != ras_se->run_list.next)
  {
    requeue_task_ras(rq, p, 0);
    resched_task(p);
    return;
  }
}
// It seems we do not need to cope with preemptions
static void check_preempt_curr_ras(struct rq *rq, struct task_struct *p)
{
  if (p->prio < rq->curr->prio)
  {
    resched_task(rq->curr);
    printk("task %d preempt %d\n", p->pid, rq->curr->pid);
    return;
  }
}

static unsigned int get_rr_interval_ras(struct rq *rq,
                                        struct task_struct *task)
{
  // This function is also somewhat strange to me.
  if (!rq || !task)
  {
    printk("INVALID RAS!\n");
    return -1;
  }
  // return the timeslice??
  return 100 - 9 * task->ras.race_prob;
}

static void prio_changed_ras(struct rq *rq, struct task_struct *p,
                             int oldprio)
{
  /*No need for prio policy*/
}

static void switched_to_ras(struct rq *rq, struct task_struct *p)
{
  printk("Switch to ras task: %d!\n", p->pid);
  if (p->on_rq && rq->curr != p)
  {
    if (rq == task_rq(p) && !rt_task(rq->curr))
      resched_task(rq->curr);
  }
}

#ifdef CONFIG_SMP
static int select_task_rq_ras(struct task_struct *p, int sd_flag, int flags)
{
}

static void set_cpus_allowed_ras(struct task_struct *p,
                                 const struct cpumask *new_mask) {}

static void rq_online_ras(struct rq *rq) {}

static void rq_offline_ras(struct rq *rq) {}

static void switched_from_ras(struct rq *rq, struct task_struct *p) {}

static void pre_schedule_ras(struct rq *rq, struct task_struct *prev) {}

static void post_schedule_ras(struct rq *rq) {}

static void task_woken_ras(struct rq *rq, struct task_struct *p) {}
#endif

const struct sched_class ras_sched_class = {
    .next = &idle_sched_class,
    .enqueue_task = enqueue_task_ras,
    .dequeue_task = dequeue_task_ras,
    .yield_task = yield_task_ras,
    .check_preempt_curr = check_preempt_curr_ras,
    .pick_next_task = pick_next_task_ras,
    .put_prev_task = put_prev_task_ras,
#ifdef CONFIG_SMP
    .select_task_rq = select_task_rq_ras,
    .set_cpus_allowed = set_cpus_allowed_ras,
    .rq_online = rq_online_ras,
    .rq_offline = rq_offline_ras,
    .pre_schedule = pre_schedule_ras,
    .task_woken = task_woken_ras,
    .switched_from = switched_from_ras,
#endif
    .set_curr_task = set_curr_task_ras,
    .task_tick = task_tick_ras,
    .get_rr_interval = get_rr_interval_ras,
    .prio_changed = prio_changed_ras,
    .switched_to = switched_to_ras,
};