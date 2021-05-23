#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/hashtable.h>
#include <linux/radix-tree.h>
#include <linux/xarray.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>

static char *int_str = "default value";

module_param(int_str, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(int_str, "String parameter");

/* Defining Linked list */
static LIST_HEAD(my_linkedlist);

/* Defining Red Black Tree root node */
struct rb_root root = RB_ROOT;

/* Part 2: 2.1: Creating Hash Table with 2^14 number of buckets */
DEFINE_HASHTABLE(my_hashtable, 14);

/* Defining Radix Tree */
RADIX_TREE(radix_tree, GFP_KERNEL);

/* Defining XARRAY */
DEFINE_XARRAY(kds_xarray);

/* Part 2: 6.1 Creating a bitmap to reprsent 1000 bits. */
DECLARE_BITMAP(my_bitmap, 1000);

/* Linked List Node */
struct kds_linkedlist
{
        int val;
        struct list_head list;
};

/* Red Black Tree Node */
struct kds_rbtree
{
        int val;
        struct rb_node rbnode;
};

/* Hash Table Node */
struct kds_hashtable
{
        int val;
        struct hlist_node hash;
};

/* Radix Tree Node */
struct kds_radixtree
{
        int val;
};

static int insert_linked_list(int num)
{
        struct kds_linkedlist *kds_list;
        kds_list = kmalloc(sizeof(*kds_list), GFP_KERNEL);
        kds_list->val = num;
        
        /* Adding number to the linked list pointing from tail */
        list_add_tail(&kds_list->list, &my_linkedlist);
        
        return 0;
}

int insert_rb_tree(struct rb_root *rbroot, struct kds_rbtree *insert_element)
{
        struct rb_node **link = &(rbroot->rb_node);
        struct rb_node *parent = NULL;
        int num = insert_element->val;

        while(*link)
        {
                struct kds_rbtree *temp = container_of(*link, struct kds_rbtree, rbnode);
                parent = *link;
                if (temp->val > num)
                {
                        link = &(*link)->rb_left;
                }
                else if (temp->val < num)
                {
                        link = &(*link)->rb_right;
                }
                else
                {
                        break; 
               }
        }
        
        /* After traversing where to place the element, adding the node and 
        color to the red black tree */
        rb_link_node(&insert_element->rbnode, parent, link);
        rb_insert_color(&insert_element->rbnode, rbroot);
        
        return 0;
}

static int insert_rbtree(int num)
{
        struct kds_rbtree *my_rbtree;
        my_rbtree = kmalloc(sizeof(*my_rbtree), GFP_KERNEL);
        my_rbtree->val = num;
        insert_rb_tree(&root, my_rbtree);
        return 0;
}

static int insert_hash_table(int num)
{
        struct kds_hashtable *my_hash;
        my_hash = kmalloc(sizeof(*my_hash), GFP_KERNEL);
        my_hash->val = num;
        
        /* Inserting number to the hash table */
        hash_add(my_hashtable, &my_hash->hash, my_hash->val);
        
        return 0;
}

static int insert_radix_kds(int num)
{
        struct kds_radixtree *rt_node;
        rt_node = kmalloc(sizeof(*rt_node), GFP_KERNEL);
        rt_node-> val = num;
        
        /* Inserting number to the radix tree */
        return radix_tree_insert(&radix_tree, num, rt_node); 
}

static int insert_xarray(int num)
{
        struct kds_radixtree *xarray_node;
        xarray_node = kmalloc(sizeof(*xarray_node),GFP_KERNEL);
        xarray_node->val = num;
        
        /* Inserting number to the XARRAY */
        xa_store(&kds_xarray, num, xarray_node, GFP_KERNEL);
        
        return 0;
}

static int insert_bitmap(int num)
{
        /*
        Setting bit to 1 
        */
        set_bit(num, my_bitmap);
        return 0;
}

static void linkedlist_lookup_print(void)
{
        struct list_head *position = NULL;
        struct kds_linkedlist *dataptr = NULL;
        printk(KERN_INFO "Part 2: 1.2: Linked List Values: \n");
        list_for_each(position, &my_linkedlist)
        {
                dataptr = list_entry(position, struct kds_linkedlist, list);
                printk(KERN_INFO "%d ", dataptr-> val);
        }
}

static int rb_tree_lookup(struct rb_root *tree_root, int num)
{
        struct rb_node **link = &(tree_root->rb_node);
        struct rb_node *parent = NULL;

        /* 
        Traversing through the Red Black Tree to fetch the number.
        If the number is not found and the link is NULL, return -1 
        */

        while(*link)
        {
                struct kds_rbtree *temp = container_of(*link, struct kds_rbtree, rbnode);
                parent = *link;
                if (temp->val > num)
                {
                        link = &(*link)->rb_left;
                }
                else if (temp->val < num)
                {
                        link = &(*link)->rb_right;
                }
                else
                {
                        printk(KERN_INFO "Lookup RedBlack Tree, Number Found :%d ", num);
                        return num;
                }
        }
        return -1;
}

static void rb_tree_lookup_print(void)
{
        struct rb_node *iternode;
        printk(KERN_INFO "Part 2: 2.3: Red Black Tree Lookup and Print Values: \n");

        for (iternode = rb_first(&root); iternode; iternode = rb_next(iternode))
        {
                struct kds_rbtree *temp = rb_entry(iternode, struct kds_rbtree, rbnode);
                rb_tree_lookup(&root, temp->val);
        }
}

static void hashtable_print(void)
{
        unsigned int bucket;
        struct kds_hashtable *cur_position;
        printk(KERN_INFO "Part 2: 3.3: Hash Table - Iterate and Print Values: \n");
        hash_for_each(my_hashtable, bucket, cur_position, hash)
        {
                printk(KERN_INFO "%d ", cur_position->val);
        }
}

static void hashtable_lookup_print(void)
{
        int num = 0;
        unsigned int bucket;
        struct kds_hashtable *cur_position, *lookup_position;
        printk(KERN_INFO "Part 2: 3.4: Hash Table - Lookup and Print Values: \n");
        hash_for_each(my_hashtable, bucket, cur_position, hash)
        {
                num = cur_position->val;
                
                /* Looking up in Hash Table from the number found in iterator */
                hash_for_each_possible(my_hashtable, lookup_position, hash, num)
                {
                        /* 
                        Since iterating over Hashtable, able to find all the values.
                        Need to add check otherwise
                        */
                        printk(KERN_INFO "Found: %d ", lookup_position->val);
                }
        }
}

static int xarray_lookup_print(void)
{
        struct kds_radixtree *xarray_node;
        unsigned long i = 0;
        printk(KERN_INFO "Part 2: 5.3: Xarray lookup and print");
        xa_for_each(&kds_xarray, i, xarray_node)
        {
                if (xa_load(&kds_xarray, xarray_node->val)!=NULL)
                {
                        printk(KERN_INFO "Xarray - number found : %d\n", xarray_node->val);
                }
        }
        return 0;
}

static int radix_lookup_print(void)
{
        void **slot;
        struct radix_tree_iter iter;
        struct kds_radixtree *lookup_response = kmalloc(sizeof(*lookup_response), GFP_KERNEL);
        printk(KERN_INFO "Part 2: 4.3: Radix lookup and print");
        radix_tree_for_each_slot(slot, &radix_tree, &iter, 0)
        {
        
                /* Looking up in Radix from the number in iterator */
                lookup_response = radix_tree_lookup(&radix_tree, iter.index);

                /*
                Since iteration is happening on the radix tree itself, 
                lookup_response will never fail. Add a NULL Check otherwise.
                */
                printk(KERN_INFO "%d    ", lookup_response->val);
        }
        return 0;
}

static void bitmap_print(void)
{
        unsigned long bit;
        printk(KERN_INFO "Part 2: 6.3: Printing all set bits \n");
        for_each_set_bit(bit, my_bitmap, 1000)
        {
                printk(KERN_INFO "%lu ", bit);
        }
}

static int radix_tag_odd(void)
{
        void **slot;
        struct radix_tree_iter iter;
        int tag = 1;
        printk(KERN_INFO "Part 2: 4.4: Radix tagging odd numbers");
        radix_tree_for_each_slot(slot, &radix_tree, &iter, 0)
        {
                /* Part 2: 3.4: Finding odd numbers to TAG */
                if (iter.index % 2 ==1)
                {
                        radix_tree_tag_set(&radix_tree, iter.index, tag);
                }
        }
        return 0;
}

static void xarray_tag_odd(void)
{
        struct kds_radixtree *xarray_node;
        unsigned long i = 0;
        printk(KERN_INFO "Part 2: 5.4: Tagging odd numbers in Xarray");
        xa_for_each(&kds_xarray, i, xarray_node)
        {
                /* Part 2: 4.4: Finding odd numbers to TAG */
                if (xarray_node->val % 2 == 1)
                {
                        xa_set_mark(&kds_xarray, xarray_node->val, XA_MARK_1);
                }
        }
}

static int radix_tag_lookup(void)
{
        int tag = 1;
        struct kds_radixtree *results[10];
        unsigned int count;
        int i = 0;

        /* Count has the number of values tagged */
        /* Results is an array of pointers which stores the tag values */
        count = radix_tree_gang_lookup_tag(&radix_tree, (void *)results, 0, 10, tag);
        printk(KERN_INFO "Part 2: 4.5: Number of Radix Tree Tags Results: %d\n", count);

        /* 
        Loop till the number of tagged values and printing the values
        in the result pointer gives the tagged values 
        */
        while (i<count)
        {
                printk(KERN_INFO "Value in Tagged Item Gang lookup: %d", results[i++]->val);
        }
        return 0;
}

static void xarray_tag_lookup(void)
{
        struct kds_radixtree *xarray_node;
        unsigned long i = 0;
        printk(KERN_INFO "Part 2: 5.5: Lookup Tag numbers in Xarray");
        xa_for_each_marked(&kds_xarray, i, xarray_node, XA_MARK_1)
        {
                printk(KERN_INFO "Xarray Tag: %ld\n", xarray_node->val);
        }
}

static void linked_list_remove(void)
{
        struct list_head *position;
        struct list_head *next;
        struct kds_linkedlist *data;
        list_for_each_safe(position, next, &my_linkedlist)
        {
                data = list_entry(position, struct kds_linkedlist, list);
                list_del(position);
                kfree(data);
        }
        printk(KERN_INFO "Part 2: 1.3: Removed all data and deallocated space in linked list\n");
}

static void rb_tree_remove(struct rb_root *root)
{
        struct rb_node *iternode;
        for (iternode = rb_first(root); iternode; iternode = rb_next(iternode))
        {
                struct kds_rbtree *current_entry;
                current_entry = rb_entry(iternode, struct kds_rbtree, rbnode);
                rb_erase(&(current_entry->rbnode),root);
        }
        printk(KERN_INFO "Part 2: 2.4: Removing and Deallocating space for Red Black Trees");
}

static void hash_table_remove(void)
{
        int bucket;
        struct kds_hashtable *position, *lookup_position;
        struct hlist_node *tmp = NULL;
        hash_for_each_safe(my_hashtable, bucket, tmp, position, hash)
        {
                hash_for_each_possible(my_hashtable, lookup_position, hash, &position->hash)
                {
                        hash_del(&lookup_position->hash);
                }
                kfree(position);
                kfree(lookup_position);
        }
        kfree(tmp);
        printk(KERN_INFO "Part 2: 3.5, 3.6: Removed and Deallocated Hash Table Values");
}

static int radix_remove(void)
{
        void **slot;
        struct radix_tree_iter iter;
        printk(KERN_INFO "Part 2: 4.6: Radix - Removing numbers");
        radix_tree_for_each_slot(slot, &radix_tree, &iter, 0)
        {
                radix_tree_delete(&radix_tree, iter.index);
        }
        return 0;
}

static void xarray_remove(void)
{
        struct kds_radixtree *xarray_node;
        unsigned long i = 0;
        printk(KERN_INFO "Part 2: 5.6: Erasing numbers - Xarray");
        xa_for_each(&kds_xarray, i, xarray_node)
        {
                xa_erase(&kds_xarray, xarray_node->val);
        }
}

static void bitmap_remove(void)
{
        printk(KERN_INFO "Part 2: 6.4: Clearing all the bits in bitmap..\n");
        
        /* Setting all bits to zero in the created bitmap */
        bitmap_zero(my_bitmap, 1000);
}

static int __init lkp_init(void)
{

        char *str_ref, *temp_str;
        int i=0,k=0,num = 1;
        bitmap_zero(my_bitmap, 10);
        temp_str = kmalloc(1000, GFP_KERNEL);

        str_ref = int_str;

        printk(KERN_INFO "KDS Module loaded... \n");
        printk(KERN_INFO "Input String: %s\n", int_str);
        printk(KERN_INFO "Part 1: Tokenized Numbers: \n");

        while (str_ref[i] != '\0'){
                if (str_ref[i] != ' '){
                        temp_str[k++] = str_ref[i++];
                } else {
                        temp_str[k] = '\0';
                        kstrtoint(temp_str, 0, &num);
                        printk(KERN_INFO "%d\n", num);
                        
                        /* Inserting to all the Kernel Data Structures */
                        insert_linked_list(num);
                        insert_rbtree(num);
                        insert_hash_table(num);
                        insert_radix_kds(num);
                        insert_xarray(num);
                        insert_bitmap(num);
                        
                        /*
                        If insert_response returns any value other than 0, 
                        it means that the write has failed. Can add cases for
                        error handling.
                        */
                        
                        kfree(temp_str);
                        temp_str = kmalloc(1000, GFP_KERNEL);
                        k=0;
                        i++;
                }
        }
        
        /* temp_str will contain the last number which was not inserted
        to the Kernel Data Structures. Extracting the last number, printing
        it in console and inserting to the Kernel Data Structures defined.
        */
        
        temp_str[k] = '\0';
        kstrtoint(temp_str,0,&num);
        printk(KERN_INFO "%d\n",num);
        
        printk(KERN_INFO "\n\n");
        
        insert_linked_list(num);
        printk(KERN_INFO "Part 2: 1.1: Values inserted in Linked List");
        insert_rbtree(num);
        printk(KERN_INFO "Part 2: 2.2: Values in Red Black Tree Inserted");
        insert_hash_table(num);
        printk(KERN_INFO "Part 2: 3.2: Values in Hash Table Inserted");
        insert_radix_kds(num);
        printk(KERN_INFO "Part 2: 4.2: Values in Radix Inserted");
        insert_xarray(num);
        printk(KERN_INFO "Part 2: 5.2: Values in XARRAY Inserted");
        insert_bitmap(num);
        printk(KERN_INFO "Part 2: 6.2: Values in Bitmap Inserted");
        
        kfree(temp_str);
        
        printk(KERN_INFO "\n\n");

        /* Part 2: 1: LinkedList Operations */
        linkedlist_lookup_print();
        linked_list_remove();
        
        printk(KERN_INFO "\n\n");

        /* Part 2: 2: Red Black Trees Operations */
        rb_tree_lookup_print();
        rb_tree_remove(&root);
        
        printk(KERN_INFO "\n\n");

        /* Part 2: 3: HashTable Operations */
        hashtable_print();
        hashtable_lookup_print();
        hash_table_remove();
        
        printk(KERN_INFO "\n\n");

        /* Part 2: 4: RADIX Operations */
        radix_lookup_print();
        radix_tag_odd();
        radix_tag_lookup();
        radix_remove();
        
        printk(KERN_INFO "\n\n");

        /* Part 2: 5: XARRAY Operations */
        xarray_lookup_print();
        xarray_tag_odd();
        xarray_tag_lookup();
        xarray_remove();
        
        printk(KERN_INFO "\n\n");

        /* Part 2: 6: BitMap Operations */
        bitmap_print();
        bitmap_remove();

        return 0;
}

static void __exit lkp_exit(void)
{
        printk(KERN_INFO "KDS Module Exiting.... \n");
}

module_init(lkp_init);
module_exit(lkp_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shubham Agrawal <shuagrawal@cs.stonybrook.edu>");
MODULE_DESCRIPTION("Sample kernel module");