#ifndef _mysfs_h
#define _mysfs_h

//////////////////////////////////////////
///// headers
//////////////////////////////////////////

//struct declarations
static struct proc_dir_entry* mysfs_proc_ctrl_file;
static struct proc_dir_entry* mysfs_proc_data_file;
static struct dentry *mysfs_mount(struct file_system_type *fst, int flags, const char *devname, void *data);
static struct file_system_type mysfs_type;
static struct file_operations mysfs_file_ops;
static const struct file_operations mysfs_proc_ctrl_fops;
static const struct file_operations mysfs_proc_data_fops;
struct tree_descr OurFiles[];

// mysfs_proc_ctrl methods
static int __init mysfs_proc_ctrl_init(void);
static int mysfs_proc_ctrl_open(struct inode *inode, struct file *file);
static ssize_t mysfs_proc_ctrl_read (struct file *fp, char __user *ua, size_t sz, loff_t * off);
static ssize_t mysfs_proc_ctrl_write (struct file *fp, const char __user *ua, size_t sz, loff_t * off);
static int mysfs_proc_ctrl_show(struct seq_file *m, void *v);
static void __exit mysfs_proc_ctrl_exit(void);

// mysfs_proc_data methods
static int __init mysfs_proc_data_init(void);
static int mysfs_proc_data_open(struct inode *inode, struct file *file);
static ssize_t mysfs_proc_data_read (struct file *fp, char __user *ua, size_t sz, loff_t * off);
static ssize_t mysfs_proc_data_write (struct file *fp, const char __user *ua, size_t sz, loff_t * off);
static int mysfs_proc_data_show(struct seq_file *m, void *v);
static void __exit mysfs_proc_data_exit(void);

// msfs methods
static int mysfs_fill_super (struct super_block *sb, void *data, int silent);
static int __init mysfs_init(void);
static int mysfs_open(struct inode *inode, struct file *filp);
static ssize_t mysfs_read_file(struct file *filp, char *buf, size_t count, loff_t *offset);
static ssize_t mysfs_write_file(struct file *filp, const char *buf, size_t count, loff_t *offset);
static void __exit mysfs_exit(void);

#endif
