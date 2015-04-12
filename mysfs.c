#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>


/*
 * Implement an array of counters.
 */
#define NCOUNTERS 4
static atomic_t counters[NCOUNTERS];

static struct proc_dir_entry* mysfs_proc_ctrl_file;
static struct proc_dir_entry* mysfs_proc_data_file;

static int
mysfs_proc_ctrl_show(struct seq_file *m, void *v)
{
    printk(KERN_DEBUG "@ %s\n",__func__);
    seq_printf(m, "%llu\n",
        (unsigned long long) get_jiffies_64());
    return 0;
}

static int
mysfs_proc_ctrl_open(struct inode *inode, struct file *file)
{
    return single_open(file, mysfs_proc_ctrl_show, NULL);
}

static ssize_t mysfs_proc_ctrl_read (struct file *fp, char __user *ua, size_t sz, loff_t * off)
{
	printk(KERN_DEBUG "@ %s\n",__func__);
	return 0;
}

static ssize_t mysfs_proc_ctrl_write (struct file *fp, const char __user *ua, size_t sz, loff_t * off)
{
	printk(KERN_DEBUG "@ %s\n",__func__);
	return sz;
}

static const struct file_operations mysfs_proc_ctrl_fops = {
    .owner	= THIS_MODULE,
    .open	= mysfs_proc_ctrl_open,
    .read	= mysfs_proc_ctrl_read,
	.write  = mysfs_proc_ctrl_write,
    .llseek	= seq_lseek,
    .release	= single_release,
};

static int __init
mysfs_proc_ctrl_init(void)
{
    mysfs_proc_ctrl_file = proc_create("mysfs_ctrl", 0, NULL, &mysfs_proc_ctrl_fops);

    if (!mysfs_proc_ctrl_file) {
        return -ENOMEM;
    }

    return 0;
}

static void __exit
mysfs_proc_ctrl_exit(void)
{
    remove_proc_entry("mysfs_ctrl", NULL);
}

static int
mysfs_proc_data_show(struct seq_file *m, void *v)
{
    printk(KERN_DEBUG "@ %s\n",__func__);
    seq_printf(m, "%llu\n",
        (unsigned long long) get_jiffies_64());
    return 0;
}

static int
mysfs_proc_data_open(struct inode *inode, struct file *file)
{
    return single_open(file, mysfs_proc_data_show, NULL);
}

static ssize_t mysfs_proc_data_read (struct file *fp, char __user *ua, size_t sz, loff_t * off)
{
	printk(KERN_DEBUG "@ %s\n",__func__);
	return 0;
}

static ssize_t mysfs_proc_data_write (struct file *fp, const char __user *ua, size_t sz, loff_t * off)
{
	printk(KERN_DEBUG "@ %s\n",__func__);
	return sz;
}

static const struct file_operations mysfs_proc_data_fops = {
    .owner	= THIS_MODULE,
    .open	= mysfs_proc_data_open,
    .read	= mysfs_proc_data_read,
	.write  = mysfs_proc_data_write,
    .llseek	= seq_lseek,
    .release	= single_release,
};

static int __init
mysfs_proc_data_init(void)
{
    mysfs_proc_data_file = proc_create("mysfs_data", 0, NULL, &mysfs_proc_data_fops);

    if (!mysfs_proc_data_file) {
        return -ENOMEM;
    }

    return 0;
}

static void __exit
mysfs_proc_data_exit(void)
{
    remove_proc_entry("mysfs_data", NULL);
}

static int mysfs_open(struct inode *inode, struct file *filp)
{
	printk(KERN_DEBUG "@ %s inode=%ld\n", inode->i_ino);
	if (inode->i_ino > NCOUNTERS)
		return -ENODEV;  /* Should never happen.  */
	filp->private_data = counters + inode->i_ino - 1;
	return 0;
}

#define TMPSIZE 20

static ssize_t mysfs_read_file(struct file *filp, char *buf,
		size_t count, loff_t *offset)
{
	int v, len;
	char tmp[TMPSIZE];
	atomic_t *counter = (atomic_t *) filp->private_data;
	printk(KERN_DEBUG "READ\n");

/*
 * Encode the value, and figure out how much of it we can pass back.
 */
	v = atomic_read(counter);
	if (*offset > 0)
		v -= 1;  /* the value returned when offset was zero */
	else
		atomic_inc(counter);
	len = snprintf(tmp, TMPSIZE, "%d\n", v);
	if (*offset > len)
		return 0;
	if (count > len - *offset)
		count = len - *offset;
/*
 * Copy it back, increment the offset, and we're done.
 */
	if (copy_to_user(buf, tmp + *offset, count))
		return -EFAULT;
	*offset += count;
	return count;
}

static ssize_t mysfs_write_file(struct file *filp, const char *buf, size_t count, loff_t *offset)
{
	char tmp[TMPSIZE];
	atomic_t *counter = (atomic_t *) filp->private_data;

	printk(KERN_DEBUG "READ\n");

/*
 * Only write from the beginning.
 */
	if (*offset != 0)
		return -EINVAL;
/*
 * Read the value from the user.
 */
	if (count >= TMPSIZE)
		return -EINVAL;
	memset(tmp, 0, TMPSIZE);
	if (copy_from_user(tmp, buf, count))
		return -EFAULT;
/*
 * Store it in the counter and we are done.
 */
	atomic_set(counter, simple_strtol(tmp, NULL, 10));
	return count;
}


/*
 * Now we can put together our file operations structure.
 */
static struct file_operations mysfs_file_ops = {
	.open	= mysfs_open,
	.write  = mysfs_write_file,
	.read 	= mysfs_read_file,
};


/*
 * OK, create the files that we export.
 */
struct tree_descr OurFiles[] = {
	{ NULL, NULL, 0 },  /* Skipped */
	{ .name = "counter0",
	  .ops = &mysfs_file_ops,
	  .mode = S_IWUSR|S_IRUGO },
	{ .name = "counter1",
	  .ops = &mysfs_file_ops,
	  .mode = S_IWUSR|S_IRUGO },
	{ .name = "counter2",
	  .ops = &mysfs_file_ops,
	  .mode = S_IWUSR|S_IRUGO },
	{ .name = "counter3",
	  .ops = &mysfs_file_ops,
	  .mode = S_IWUSR|S_IRUGO },
	{ "", NULL, 0 }
};



/*
 * Superblock stuff.  This is all boilerplate to give the vfs something
 * that looks like a filesystem to work with.
 */

/*
 * "Fill" a superblock with mundane stuff.
 */
static int mysfs_fill_super (struct super_block *sb, void *data, int silent)
{
	return simple_fill_super(sb, 0x01020304, OurFiles);
}


/*
  * Stuff to pass in when registering the filesystem.
  */
static struct dentry *mysfs_mount(struct file_system_type *fst,
 		int flags, const char *devname, void *data)
 {
	return mount_nodev(fst, flags, data, mysfs_fill_super);
 }

 static struct file_system_type mysfs_type = {
 	.owner 		= THIS_MODULE,
 	.name		= "mysfs",
	.mount		= mysfs_mount,
 	.kill_sb	= kill_litter_super,
 };


static int __init mysfs_init(void)
{
	int i;

	for (i = 0; i < NCOUNTERS; i++)
		atomic_set(counters + i, 0);

	mysfs_proc_ctrl_init();
	mysfs_proc_data_init();

	return register_filesystem(&mysfs_type);
}

static void __exit mysfs_exit(void)
{
	mysfs_proc_ctrl_exit();
	mysfs_proc_data_exit();
	unregister_filesystem(&mysfs_type);
}


module_init(mysfs_init);
module_exit(mysfs_exit);
