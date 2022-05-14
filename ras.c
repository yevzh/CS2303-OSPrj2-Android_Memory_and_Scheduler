/*
Race-Averse Scheduler Scheduling Class
Implemented by WeimingZhang
*/

#include "sched.h"
#include <linux/slab.h>

void init_ras_rq(struct ras_rq *ras_rq, struct rq *rq)
{
    INIT_LIST_HEAD(ras_rq->ras_run_list);
    ras_spin_lock_init(&ras_rq->ras_runtime_lock);
    ras_rq->ras_nr_running = 0;
    ras_rq->total_wcounts = 0;
    ras_rq->ras_runtime = 0;
    ras_rq->ras_time = 0;
}

#define ras_entity_is_task(ras_se) (!(ras_se)->my_q)

static inline struct task_struct *ras_task_of(struct sched_ras_entity *ras_se)
{
#ifdef CONFIG_SCHED_DEBUG
    WARN_ON_ONCE(!ras_entity_is_task(ras_se));
#endif
    return container_of(ras_se, struct task_struct, ras);
}

static inline struct rq *rq_of_ras_rq(struct ras_rq *ras_rq)
{
    return ras_rq->rq;
}

static inline struct ras_rq *ras_rq_of_se(struct sched_ras_entity *ras_se)
{
    struct task_struct *tsk = ras_task_of(ras_se);
    struct rq *rq = task_rq(tsk);
    return &rq->ras;
}

static inline int ras_se_prio(struct sched_ras_entity *ras_se)
{
    return ras_task_of(ras_se)->rt_priority;
}

static void update_curr_ras(struct rq *rq)
{
    struct task_struct *curr = rq->curr;
    struct sched_ras_entity *ras_se = &curr->ras;
    struct ras_rq *ras_rq = ras_rq_of_se(ras_se);
    u64 delta_exec;

    if (curr->sched_class != &ras_sched_class)
        return;
    delta_exec = rq->clock_task - curr->se.exec_start;
    if (unlikely((s64)delta_exec < 0))
        delta_exec = 0;
    schedstat_set(curr->se.statistics.exec_max, max(curr->se.statistics.exec_max, delta_exec));
    curr->se.sum_exec_runtime += delta_exec;
    account_group_exec_runtime(curr, delta_exec);

    curr->se.exec_start = rq->clock_task;
    cpuacct_charge(curr, delta_exec);
}

static inline void list_add_leaf_ras_rq(struct ras_rq *ras_rq)
{
    list_add_rcu(&ras_rq->leaf_ras_rq_list, &rq_of_ras_rq(ras_rq)->leaf_ras_rq_list);
}

static inline void list_del_leaf_rt_rq(struct rt_rq *rt_rq)
{
    list_del_rcu(&ras_rq->leaf_ras_rq_list);
}

static void enqueue_ras_entity(struct sched_ras_entity *ras_se, bool head)
{
    struct ras_rq *ras = ras_rq_of_se(ras_se);
    struct ras_prio_array *array = &ras_rq->active;
    struct list_head *queue = array->queue + ras_se_prio(ras_se);
    if (!ras_rq->ras_nr_running)
        list_add_leaf_ras_rq(ras_rq);
    if (head)
        list_add(&ras_se->run_list, queue);
    else
        list_add_tail(&ras_se->run_list, queue);
    __set_bit(ras_se_prio(ras_se), array->bitmap);
    WARN_ON(!ras_se_prio(ras_se));
    WARN_ON(!ras_rq->ras_nr_running);
    ras_rq->ras_nr_running++;
}

static void enqueue_task_ras(struct rq *rq, struct task_struct *p, int flags)
{
    printk(KERN_INFO "Enqueue An New RAS Task!\n");
    struct sched_ras_entity *ras_se = &p->ras;
    enqueue_ras_entity(ras_se, flags & ENQUEUE_HEAD);
    inc_nr_running(rq);
}

static void dequeue_ras_entity(struct sched_ras_entity *ras_se)
{
    struct ras_rq *ras_rq = ras_rq_of_se(ras_se);
    struct ras_prio_array *array = &ras_rq->active;
    list_del_init(&ras_se->run_list);
    if (list_empty(array->queue + ras_se_prio(ras_se)))
        __clear_bit(ras_se_prio(ras_se), array->bitmap);

    // dec_ras_tasks(ras_se,ras_rq);
    if (!ras_rq->ras_nr_running)
        list_del_leaf_ras_rq(ras_rq);
    WARN_ON(!ras_se_prio(ras_se));
    WARN_ON(!ras_rq->ras_nr_running);
    ras_rq->ras_nr_running--;
}

static void dequeue_task_ras(struct rq *rq, struct task_struct *p, int flags)
{
    printk(KERN_INFO "Dequeue An RAS Task!\n");
    struct sched_ras_entity *ras_se = &p->ras;
    update_curr_ras(rq);
    dequeue_ras_entity(ras_se);
    dec_nr_running(rq);
}

static void requeue_task_ras(struct rq *rq, struct task_struct *p, int head)
{
    printk(KERN_INFO "Requeue The Task!\n");
    struct sched_ras_entity *ras_se = &p->ras;
    struct ras_rq *ras_rq = &rq->ras;
    struct ras_prio_array *array = &ras_rq->active;
    struct list_head *queue = array->queue + ras_se_prio(ras_se);
    if (head)
        list_move(&ras_se->run_list, queue);
    else
        list_move_tail(&ras_se->run_list, queue);
}

static void yield_task_ras(struct rq *rq)
{
    printk(KERN_INFO "Yield An RAS Task!\n");
    requeue_ras_entity(rq, rq->curr, 0);
}

static struct task_struct *pick_next_task_ras(struct rq *rq)
{
    if (unlikely(!rq->ras.ras_nr_running))
        return NULL;
    struct ras_rq *ras_rq = &rq->ras;
    struct ras_prio_array *array = &ras_rq->active;
    struct sched_ras_entity *next = NULL;
    struct list_head *queue;
    int idx;

    idx = sched_find_first_bit(array->bitmap);
    BUG_ON(idx >= MAX_RAS_PRIO);
    queue = array->queue + idx;
    next = list_entry(queue->next, struct sched_ras_entity, run_list);
    struct task_struct *p;
    p = ras_task_of(next);
    if (!p)
        return NULL;
    p->se.exec_start = rq->clock_task;
    return p;
}

static void put_prev_task_ras(struct rq *rq, struct task_struct *p)
{
    printk(KERN_INFO "Put Previous RAS Task!\n");
    update_curr_ras(rq);
    p->se.exec_start = 0;
}

static void set_curr_task_ras(struct rq *rq)
{
    printk(KERN_INFO "Set Current RAS Task!\n");
    struct task_struct *p = rq->curr;
    p->se.exec_start = rq->clock_task;
}

static void task_tick_ras(struct rq *rq, struct task_struct *p, it queued)
{
    struct sched_ras_entity *ras_se = &p->ras;
    update_curr_ras(rq);
    if (p->policy != SCHED_RAS)
        return;
    printk(KERN_INFO "!task_tick||cpu: %d pid: %d time_slice: %d\n", cpu_of(rq), p->pid, p->ras.time_slice);

    if (--p->ras.time_slice)
        return;
    p->ras.time_slice = 100 / (p->wcounts + 10);
    if (ras_se->run_list.prev != ras_se->run_list.next)
    {
        requeue_task_ras(rq, p, 0);
        resched_task(p);
        return;
    }
}

static void check_preempt_curr_ras(struct rq *rq, struct task_struct *p)
{
    if (p->prio < rq->curr->prio)
    {
        resched_task(rq->curr);
        printk(KERN_INFO "task %d preempt %d\n", p->pid, rq->curr->pid);
        return;
    }
}

static unsigned int get_rr_interval_ras(struct rq *rq, struct task_struct *task)
{
    if (!rq || !task)
    {
        printk(KERN_INFO "rq or task is NULL  | get_rr_interval_ras\n");
        return -1;
    }
    unsigned int time_slice;
    time_slice = get_timeslice(rq, task);
    if (time_slice > RAS_TIMESLICE_MAX || time_slice < RAS_TIMESLICE_MIN)
    {
        printk(KERN_INFO "time slice out of range| get_rr_interval_ras");
        return -1;
    }
    printk(KERN_INFO "get_rr_interval_ras of task %d is: %u\n", task->pid, time_slice);
    return time_slice;
}

static void prio_changed_ras(struct rq *rq, struct task_struct *p, int oldprio)
{
    /*No need for prio policy*/
}

static void switched_to_ras(struct rq *rq, struct task_struct *p)
{
    /*No need*/
}

#ifdef CONFIG_SMP
static int select_task_rq_ras(struct task_struct *p, int sd_flag, int flags)
{
}

static void set_cpus_allowed_ras(struct task_struct *p, const struct cpumask *new_mask)
{
}

static void rq_online_ras(struct rq *rq)
{
}

static void rq_offline_ras(struct rq *rq)
{
}

static void switched_from_ras(struct rq *rq, struct task_struct *p)
{
}

static void pre_schedule_ras(struct rq *rq, struct task_struct *prev)
{
}

static void post_schedule_ras(struct rq *rq)
{
}

static void task_woken_ras(struct rq *rq, struct task_struct *p)
{
}
#endif

const struct sched_class ras_sched_class = {
    .next = &fair_sched_class,
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
}