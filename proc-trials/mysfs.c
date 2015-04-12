//////////////////////////////////////////
///// header files
//////////////////////////////////////////
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include "mysfs.h"

//////////////////////////////////////////
///// variable definitions
//////////////////////////////////////////
#define NCOUNTERS 4
#define TMPSIZE 20
static atomic_t counters[NCOUNTERS];

//////////////////////////////////////////
///// structures
//////////////////////////////////////////

// mysfs_type: owner, name, mount, kill_sb
static struct file_system_type mysfs_type = {
  .owner     = THIS_MODULE,
  .name    = "mysfs",
  .mount    = mysfs_mount,
  .kill_sb  = kill_litter_super,
};

// mysfs_file_ops: open, read, write
static struct file_operations mysfs_file_ops = {
  .open  = mysfs_open,
  .read   = mysfs_read_file,
  .write  = mysfs_write_file,
};

// mysfs_proc_ctrl_fops: owner, open, read, write, llseek, release
static const struct file_operations mysfs_proc_ctrl_fops = {
  .owner  = THIS_MODULE,
  .open  = mysfs_proc_ctrl_open,
  .read  = mysfs_proc_ctrl_read,
  .write  = mysfs_proc_ctrl_write,
  .llseek  = seq_lseek,
  .release  = single_release,
};

// mysfs_proc_data_fops: owner, open, read, write, llseek, release
static const struct file_operations mysfs_proc_data_fops = {
  .owner  = THIS_MODULE,
  .open  = mysfs_proc_data_open,
  .read  = mysfs_proc_data_read,
  .write  = mysfs_proc_data_write,
  .llseek  = seq_lseek,
  .release  = single_release,
};

// OurFiles: 4 files each with name, ops, mode
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


//////////////////////////////////////////
///// proc_ctrl utility methods
//////////////////////////////////////////
// init, open, read, write, show, exit

// mysfs_proc_ctrl_init
static int __init mysfs_proc_ctrl_init(void) {
  mysfs_proc_ctrl_file = proc_create("mysfs_ctrl", 0, NULL, &mysfs_proc_ctrl_fops);

  if (!mysfs_proc_ctrl_file) {
    return -ENOMEM;
  }
  return 0;
}

// mysfs_proc_ctrl_open
static int mysfs_proc_ctrl_open(struct inode *inode, struct file *file) {
  return single_open(file, mysfs_proc_ctrl_show, NULL);
}

// mysfs_proc_ctrl_read
static ssize_t mysfs_proc_ctrl_read (struct file *fp, char __user *ua, size_t sz, loff_t * off) {
  printk(KERN_DEBUG "@ %s\n",__func__);
  return 0;
}

// mysfs_proc_ctrl_write
static ssize_t mysfs_proc_ctrl_write (struct file *fp, const char __user *ua, size_t sz, loff_t * off) {
  printk(KERN_DEBUG "@ %s\n",__func__);
  return sz;
}

// mysfs_proc_ctrl_show
static int mysfs_proc_ctrl_show(struct seq_file *m, void *v) {
  printk(KERN_DEBUG "@ %s\n",__func__);
  seq_printf(m, "%llu\n",
  (unsigned long long) get_jiffies_64());
  return 0;
}

// mysfs_proc_ctrl_exit
static void __exit mysfs_proc_ctrl_exit(void) {
  remove_proc_entry("mysfs_ctrl", NULL);
}

//////////////////////////////////////////
///// proc_data utility methods
//////////////////////////////////////////
// init, open, read, write, show, exit

// mysfs_proc_data_init
static int __init mysfs_proc_data_init(void){
  mysfs_proc_data_file = proc_create("mysfs_data", 0, NULL, &mysfs_proc_data_fops);

  if (!mysfs_proc_data_file) {
    return -ENOMEM;
  }
  return 0;
}

// mysfs_proc_ctrl_open
static int mysfs_proc_data_open(struct inode *inode, struct file *file) {
  return single_open(file, mysfs_proc_data_show, NULL);
}

// mysfs_proc_ctrl_read
static ssize_t mysfs_proc_data_read (struct file *fp, char __user *ua, size_t sz, loff_t * off) {
  printk(KERN_DEBUG "@ %s\n",__func__);
  return 0;
}

// mysfs_proc_ctrl_write
static ssize_t mysfs_proc_data_write (struct file *fp, const char __user *ua, size_t sz, loff_t * off) {
  printk(KERN_DEBUG "@ %s\n",__func__);
  return sz;
}

// mysfs_proc_ctrl_show
static int mysfs_proc_data_show(struct seq_file *m, void *v) {
  printk(KERN_DEBUG "@ %s\n",__func__);
  seq_printf(m, "%llu\n", (unsigned long long) get_jiffies_64());
  return 0;
}

// mysfs_proc_data_exit
static void __exit mysfs_proc_data_exit(void) {
  remove_proc_entry("mysfs_data", NULL);
}

//////////////////////////////////////////
///// mysfs methods
//////////////////////////////////////////
// mount, fill_super, init, open, read, write, exit

// mysfs_mount
static struct dentry *mysfs_mount(struct file_system_type *fst, int flags, const char *devname, void *data) {
  printk(KERN_DEBUG "@ %s\n",__func__);
  return mount_nodev(fst, flags, data, mysfs_fill_super);
}

// mysfs_fill_super
static int mysfs_fill_super (struct super_block *sb, void *data, int silent) {
  return simple_fill_super(sb, 0x01020304, OurFiles);
}

// mysfs_init
static int __init mysfs_init(void) {
  int i;
  printk(KERN_DEBUG "mysfs_init start");
  for (i = 0; i < NCOUNTERS; i++)
  atomic_set(counters + i, 0);

  mysfs_proc_ctrl_init();
  mysfs_proc_data_init();

  return register_filesystem(&mysfs_type);
}

// mysfs_open
static int mysfs_open(struct inode *inode, struct file *filp) {
  printk(KERN_DEBUG "@ %lu inode=%ld\n", inode->i_ino);
  if (inode->i_ino > NCOUNTERS)
    return -ENODEV;  /* Should never happen.  */
  filp->private_data = counters + inode->i_ino - 1;
  return 0;
}

// mysfs_read
static ssize_t mysfs_read_file(struct file *filp, char *buf, size_t count, loff_t *offset) {
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

//mysfs_write_file
static ssize_t mysfs_write_file(struct file *filp, const char *buf, size_t count, loff_t *offset) {
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

// mysfs_exit
static void __exit mysfs_exit(void) {
  mysfs_proc_ctrl_exit();
  mysfs_proc_data_exit();
  unregister_filesystem(&mysfs_type);
}


//////////////////////////////////////////
///// call init and exit module
//////////////////////////////////////////

// initialize fs module
module_init(mysfs_init);

FILE *fila;
const char *buff;
size_t c;
loff_t *o;
static ssize_t mysfs_write_file(struct file *filp, const char *buf, size_t count, loff_t *offset)

// exit mysfs module
module_exit(mysfs_exit);
