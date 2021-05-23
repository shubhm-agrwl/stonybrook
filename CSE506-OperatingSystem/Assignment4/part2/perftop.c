#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kprobes.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>

static char func_name[NAME_MAX] = "pick_next_task_fair";
module_param_string(func, func_name, NAME_MAX, S_IRUGO);
MODULE_PARM_DESC(func, "Function to kretprobe; this module will keep track of time each task spends on CPU and print the 10 most scheduled tasks");

DEFINE_HASHTABLE(my_hashtable, 14);
DEFINE_SPINLOCK(etx_spinlock);

/* per-instance private data */
struct my_data {
	struct task_struct * instance_task;
};

/* Hash Table Node */
struct kds_hashtable
{
        int val;
        unsigned long long timestamp;
        struct hlist_node hash;
};

/* Defining Red Black Tree root node */
struct rb_root root = RB_ROOT;

/* Red Black Tree Node */
struct kds_rbtree
{
        int val;
        unsigned long long total_time;
        struct rb_node rbnode;
};

/* This function is used to search and delete from hash table */
static unsigned long long search_delete_hash_table(int num, int delete){
		unsigned long long time = 0;
		struct kds_hashtable *lookup_position;
        /* Looking up in Hash Table from the number found in iterator */
        hash_for_each_possible(my_hashtable, lookup_position, hash, num)
        {
            if (num == lookup_position->val)
            {
                time = lookup_position->timestamp;
                if (delete == 1){
					hash_del(&lookup_position->hash);
                }
           	}
        }
        kfree(lookup_position);
        return time;
}

static void hash_table_remove(void)
{
	int bucket;
        struct kds_hashtable *position;
        struct hlist_node *tmp = NULL;
	int count = 0;
        hash_for_each_safe(my_hashtable, bucket, tmp, position, hash)
        {
		count++;
                hash_del(&position->hash);
                kfree(position);
        }
	pr_info("%d items removed from hashmap\n", count);
        kfree(tmp);
}

/* This function is used to insert into hash table */
static int insert_hash_table(int num, unsigned long long timestamp)
{
        struct kds_hashtable *my_hash;
        my_hash = kmalloc(sizeof(*my_hash), GFP_ATOMIC);
        my_hash->val = num;
        my_hash->timestamp = timestamp;

        search_delete_hash_table(num, 1);

        /* Inserting number to the hash table */
        hash_add(my_hashtable, &my_hash->hash, my_hash->val);

        return 0;
}

/* This function is used to insert to RB Tree */
int insert_rb_tree(struct rb_root *tree_root, struct kds_rbtree *insert_element)
{
        struct rb_node **link = &(tree_root->rb_node);
        struct rb_node *parent = NULL;
        unsigned long long time = insert_element->total_time;

        while(*link)
        {
                struct kds_rbtree *temp = container_of(*link, struct kds_rbtree, rbnode);
                parent = *link;
                if (temp->total_time > time)
                {
                        link = &(*link)->rb_left;
                }
                else if (temp->total_time < time)
                {
                        link = &(*link)->rb_right;
                }
                else
                {
                        return -1;
                }
        }

        /* After traversing where to place the element, adding the node and
        color to the red black tree */
        rb_link_node(&insert_element->rbnode, parent, link);
        rb_insert_color(&insert_element->rbnode, tree_root);

        return 0;
}

/* This function is used to fetch and remove from RB Tree */
static unsigned long long lookup_remove_from_rbtree(struct rb_root *root, int pid)
{
        struct rb_node *iternode;
	struct kds_rbtree *current_entry;
        for (iternode = rb_first(root); iternode; iternode = rb_next(iternode))
        {
                current_entry = rb_entry(iternode, struct kds_rbtree, rbnode);
                if (current_entry->val == pid){
                		unsigned long long cur_time = current_entry->total_time;
                		rb_erase(&(current_entry->rbnode),root);
                		return cur_time;
                }
        }
        return 0;
}

static void remove_from_rbtree(struct rb_root *root)
{
        struct rb_node *iternode;
	struct kds_rbtree *current_entry;
	int count = 0;
        for (iternode = rb_first(root); iternode; iternode = rb_next(iternode))
        {
		count++;
                current_entry = rb_entry(iternode, struct kds_rbtree, rbnode);
                rb_erase(&(current_entry->rbnode),root);
        }
	pr_info("%d items removed from RBTree\n", count);
}

/* This function is used to insert to RB Tree */
static int insert_rbtree(int num, unsigned long long time)
{
        struct kds_rbtree *my_rbtree;
        my_rbtree = kmalloc(sizeof(*my_rbtree), GFP_ATOMIC);
        my_rbtree->val = num;
        my_rbtree->total_time = time;
        insert_rb_tree(&root, my_rbtree);
        return 0;
}

/* This function is used to print the top 10 most scheduled tasks */
static void rb_tree_print(struct seq_file *m)
{
        struct rb_node *iternode;
        struct kds_rbtree *temp;
	char str[1000];
	char buf[256];
	int count = 1;
	strcpy(str,"");
        for (iternode = rb_last(&root); iternode; iternode = rb_prev(iternode))
        {
			if (count > 10) {
				break;
			}
            temp = rb_entry(iternode, struct kds_rbtree, rbnode);
	    snprintf(buf, sizeof buf, "PID - %d | Total TSC - %llu\n", temp->val, temp->total_time);
	    strcat(str, buf);
            count++;
            memset(buf, 0, 256);
        }
		seq_printf(m, str);
		memset(str, 0, 1000);
}

/* entry handler */
static int entry_pick_next_fair(struct kretprobe_instance *ri, struct pt_regs *regs){

	struct my_data *data;
	struct task_struct * prev_task;

	/* Skip kernel threads */
	if (!current->mm)
		return 1;

	data = (struct my_data *)ri->data;

	/* Extracting previous task value from the SI Register */
	prev_task = (struct task_struct*)(regs->si);

	/* Handling case when task struct is not available in the register */
        if (prev_task == NULL){
                return 1;
        }

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
	struct my_data *data;
	struct task_struct * prev_task;
	struct task_struct * next_task;
	unsigned long long delta;
	unsigned long long now;
	unsigned long long time;
	unsigned long long prev_total_time;

	/* Skip kernel threads */
        if (!current->mm)
        	return 1;

	data = (struct my_data *)ri->data;
	/* Extracting Next task value from the AX Register */
	next_task = (struct task_struct*)(regs->ax);
	/* Handling case when task struct is not available in the register */
	if (next_task == NULL){
		return 1;
	}

	/* Reading Previous Task Value from the data structure */
	prev_task = data->instance_task;

	/* Writing Next Task PID to the system logs for debugging */
	pr_info("Next Task PID: %u\n", next_task->pid);

	/* Checking whether the previous task is not the same as next task */
	if (prev_task != next_task){

		spin_lock(&etx_spinlock);

		/* Getting current time */
		now = rdtsc();

		/* inserting the current time for the PID in the hash table */
		insert_hash_table((int)next_task->pid, now);

		/* Getting time of previous PID from the hash table */
		time = search_delete_hash_table((int)prev_task->pid, 0);
		if (time > 0){

			/* Getting time difference, i.e time for task */
			delta = now - time;
			
			/* Getting previous total time from the RB Tree */
			prev_total_time = lookup_remove_from_rbtree(&root, (int)prev_task->pid);
			
			/* Inserting the new total value for the PID to the RB Tree */
			insert_rbtree((int)prev_task->pid, delta + prev_total_time);
		}
		spin_unlock(&etx_spinlock);
	}

	return 0;
}
NOKPROBE_SYMBOL(ret_pick_next_fair);

/* Displaying the atomic counter values */
static int perftop_show(struct seq_file *m, void *v) {
	rb_tree_print(m);
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
	.handler		= ret_pick_next_fair,
	.entry_handler		= entry_pick_next_fair,
	.data_size		= sizeof(struct my_data),
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
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
	pr_info("Deallocating HashMap and RBTree: \n");
	hash_table_remove();
	remove_from_rbtree(&root);

	unregister_kretprobe(&my_kretprobe);
	pr_info("kretprobe at %p unregistered\n", my_kretprobe.kp.addr);

	/* nmissed > 0 suggests that maxactive was set too low. */
	pr_info("Missed probing %d instances of %s\n",
		my_kretprobe.nmissed, my_kretprobe.kp.symbol_name);
}

MODULE_LICENSE("GPL");
module_init(perftop_init);
module_exit(perftop_exit);
