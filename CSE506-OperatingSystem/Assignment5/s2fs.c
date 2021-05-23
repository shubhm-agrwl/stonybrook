#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pagemap.h> 	/* PAGE_CACHE_SIZE */
#include <linux/fs.h>     	/* This is where libfs stuff is declared */
#include <asm/uaccess.h>	/* copy_to_user */

#define LFS_MAGIC 0x19920342

/*
 * We need to make an Inode whenever we make a file and directory.
 * "mode" parameter specifies whether this is a directory or file.
 */

static struct inode *s2fs_make_inode(struct super_block *sb, int mode)
{
	struct inode* inode;            
        inode = new_inode(sb);
       if (!inode) {
                return NULL;
        }
        inode->i_mode = mode;
        inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
        inode->i_ino = get_next_ino();
	return inode;
}

/*
 * Open a file. Returns 0 as we are not using this for the assignment.
 */
static int s2fs_open(struct inode *inode, struct file *filp)
{
	return 0;
}

/*
 * Read a file. Printing "Hello World"
 */

static ssize_t s2fs_read_file(struct file *filp, char *buf, size_t count, loff_t *offset)
{
	int len = strlen("Hello World!\n");
	
	if (*offset > len)
		return 0;
	if (count > len - *offset)
		count = len - *offset;
	
	if (copy_to_user(buf, "Hello World!\n" + *offset, len))
		return -EFAULT;
	*offset += count;
	return count;
}

/*
 * Write a file. Returns 0 as we are not using this for the assignment.
 */
static ssize_t s2fs_write_file(struct file *filp, const char *buf, size_t count, loff_t *offset) {
	return 0;
}


/*
 * File operations structure.
 */
static struct file_operations s2fs_fops = {
	.open	= s2fs_open,
	.read 	= s2fs_read_file,
	.write  = s2fs_write_file,
};

static struct dentry *s2fs_create_file (struct super_block *sb, struct dentry *dir, const char *file_name) {
	struct dentry *dentry;
	struct inode *inode;

/*
 * Create dentry and inode.
 */
	dentry = d_alloc_name(dir, file_name);
	if (! dentry)
		goto out;
	inode = s2fs_make_inode(sb, S_IFREG | 0777);
	if (! inode)
		goto out_dput;
	
	/* Set s2fs_fops as file operations */
	inode->i_fop = &s2fs_fops;
/*
 * Adding all into the dentry cache.
 */
	d_add(dentry, inode);
	return dentry;
  
  	out_dput:
		dput(dentry);
  	out:
		return 0;
}


/*
 * Creating a directory.
 */
static struct dentry *s2fs_create_dir (struct super_block *sb, struct dentry *parent, const char *dir_name) {
	struct dentry *dentry;
	struct inode *inode;

	dentry = d_alloc_name(parent, dir_name);
	if (! dentry)
		goto out;

	inode = s2fs_make_inode(sb, S_IFDIR | 0777);
	if (! inode)
		goto out_dput;
	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;

	d_add(dentry, inode);
	return dentry;

  out_dput:
	dput(dentry);
  out:
	return 0;
}

/*
 * Superblock operations.
 */
static struct super_operations s2fs_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

static int s2fs_fill_super (struct super_block *sb, void *data, int silent)
{
	struct inode *root;
	struct dentry *root_dentry;
	struct dentry *foo_subdir;
/*
 * Basic parameters.
 */
	sb->s_blocksize = VMACACHE_SIZE;
	sb->s_blocksize_bits = VMACACHE_SIZE;
	sb->s_magic = LFS_MAGIC;
	sb->s_op = &s2fs_s_ops;
/*
 * Make an inode to represent the root directory of the filesystem.
 */
	root = s2fs_make_inode (sb, S_IFDIR | 0777);
	inode_init_owner(root, NULL, S_IFDIR | 0777);
	if (! root)
		goto out;
	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;
	
	set_nlink(root, 2);
	root_dentry = d_make_root(root);
	if (! root_dentry)
		goto out_iput;
/*
 * Creating a sub directory "foo". Creating a file "bar" inside the 
 * "foo" sub directory if it is created successfully.
 */
	foo_subdir = s2fs_create_dir(sb, root_dentry, "foo");
	if (foo_subdir){
		s2fs_create_file(sb, foo_subdir, "bar");
	}
	sb->s_root = root_dentry;
	return 0;
	
  out_iput:
	iput(root);
  out:
	return -ENOMEM;
}


/*
 * Mounting the filesystem.
 */
static struct dentry *s2fs_get_super(struct file_system_type *fst, int flags, const char *devname, void *data) {
	return mount_nodev(fst, flags, data, s2fs_fill_super);
}

static struct file_system_type s2fs_type = {
	.owner 		= THIS_MODULE,
	.name		= "s2fs",
	.mount		= s2fs_get_super,
	.kill_sb	= kill_litter_super,
};

/*
 * Registering File System.
 */
static int __init s2fs_init(void)
{
	return register_filesystem(&s2fs_type);
}

/* 
 * Unregistering File System.
 */
static void __exit s2fs_exit(void)
{
	unregister_filesystem(&s2fs_type);
}

MODULE_LICENSE("GPL");
module_init(s2fs_init);
module_exit(s2fs_exit);
