#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kprobes.h>

static char func_name[NAME_MAX] = "pick_next_task_fair";
module_param_string(func, func_name, NAME_MAX, S_IRUGO);
MODULE_PARM_DESC(func, "Function to kretprobe; this module will report the number of context switches for the function");

/* per-instance private data */
struct my_data {
  struct task_struct * instance_task;
};

/* atomic counters initialized to zero accessed by all instances */
static atomic_t pre_count = ATOMIC_INIT(0);
static atomic_t post_count = ATOMIC_INIT(0);
static atomic_t context_switch_count = ATOMIC_INIT(0);

/* entry handler */
static int entry_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs)
{
  struct task_struct * prev_task;
  struct my_data *data = (struct my_data *)ri->data;

  /* Skip kernel threads */
  if (!current->mm)
    return 1;

  /* Extracting previous task value from the SI Register */
  prev_task = (struct task_struct*)(regs->si);

  /* Handling case when task struct is not available in the register */
  if (prev_task == NULL){
    return 1;
  }

    /* Incrementing Pre Count Value */
        atomic_add(1, &pre_count);

  /* Assigning Previous task struct value to the data structure */
  data->instance_task = prev_task;

  /* Writing Previous Task PID to the system logs for debugging */
        pr_info("Previous Task PID: %u\n",prev_task->pid);

  return 0;
}
NOKPROBE_SYMBOL(entry_pick_next_fair);

/* Return Handler */
static int ret_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs)
{
  struct task_struct * next_task;
  struct my_data *data = (struct my_data *)ri->data;
  /* Reading Previous Task Value from the data structure */
        struct task_struct * prev_task = data->instance_task;

  /* Incrementing Post Count Value */
  atomic_add(1, &post_count);

  /* Extracting Next task value from the AX Register */
  next_task = (struct task_struct*)(regs->ax);
  
  /* Handling case when task struct is not available in the register */
  if (next_task == NULL){
    return 1;
  }

  /* Writing Next Task PID to the system logs for debugging */
  pr_info("Next Task PID: %u\n", next_task->pid);

  /* Checking whether the previous task is not the same as next task */
  if (prev_task != next_task){
    /* Incrementing the context switch count if the above condition
     * is validated
     */
    atomic_add(1, &context_switch_count);
  }

  return 0;
}
NOKPROBE_SYMBOL(ret_pick_next_fair);

/* Displaying the atomic counter values */
static int perftop_show(struct seq_file *m, void *v) {
    seq_printf(m, "Hello World | Pre Count: %d | Post Count: %d | Context Switch Count: %d\n", atomic_read(&pre_count), atomic_read(&post_count), atomic_read(&context_switch_count));
    return 0;
}

static int perftop_open(struct inode *inode, struct  file *file) {
  return single_open(file, perftop_show, NULL);
}

static const struct proc_ops perftop_fops = {
  .proc_open = perftop_open,
  .proc_read = seq_read,
  .proc_lseek = seq_lseek,
  .proc_release = single_release,
};

static struct kretprobe my_kretprobe = {
  .handler    = ret_pick_next_fair,
  .entry_handler    = entry_pick_next_fair,
  .data_size    = sizeof(struct my_data),
  /* Probe up to 20 instances concurrently. */
  .maxactive    = 20,
};

static int __init perftop_init(void) {
    int ret;
  proc_create("perftop", 0, NULL, &perftop_fops);

    my_kretprobe.kp.symbol_name = func_name;

  ret = register_kretprobe(&my_kretprobe);
  if (ret < 0) {
    pr_err("register_kretprobe failed, returned %d\n", ret);
    return -1;
  }
  pr_info("Planted return probe at %s: %p\n",
      my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);
  return 0;
}

static void __exit perftop_exit(void) {
  remove_proc_entry("perftop", NULL);

  unregister_kretprobe(&my_kretprobe);
  pr_info("kretprobe at %p unregistered\n", my_kretprobe.kp.addr);

  /* nmissed > 0 suggests that maxactive was set too low. */
  pr_info("Missed probing %d instances of %s\n",
    my_kretprobe.nmissed, my_kretprobe.kp.symbol_name);
}

MODULE_LICENSE("GPL");
module_init(perftop_init);
module_exit(perftop_exit);
