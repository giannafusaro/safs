////////////////////////////////////////////////////////////////////////////////
//////// myfs declarations
////////////////////////////////////////////////////////////////////////////////

int read_inode(const unsigned int in, struct stat *stp);
int write_inode(const unsigned int in, struct stat st);

int myfs_link (const char *path1, const char *path2);
int myfs_unlink (const char *path);

int myfs_chmod (const char *path, mode_t mode);
int myfs_chown (const char *path, uid_t uid, gid_t gid);

int myfs_utimens (const char *path, const struct timespec tv[2]);
int myfs_utime (const char *path, struct utimbuf *tb);

int myfs_truncate (const char *path, off_t off);
int myfs_ftruncate (const char *path, off_t off, struct fuse_file_info *fi);

int myfs_setattr (const char *path, const char *name, const char *value, size_t size, int flag);
static int myfs_getattr(const char *path, struct stat *stbuf);
static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int myfs_mknod (const char *filename, mode_t mode, dev_t dev);
int myfs_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

static int myfs_open(const char *path, struct fuse_file_info *fi);
static int myfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
