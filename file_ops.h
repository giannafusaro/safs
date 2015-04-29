////////////////////////////////////////////////////////////////////////////////
// File operation prototypes
////////////////////////////////////////////////////////////////////////////////

static int safs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
int safs_link (const char *path1, const char *path2);
int safs_truncate (const char *path, off_t off);
int safs_ftruncate (const char *path, off_t off, struct fuse_file_info *fi);
int safs_unlink (const char *path);
int safs_unlink (const char *path);
int safs_mknod(const char *filename, mode_t mode, dev_t dev);
static int safs_open(const char *path, struct fuse_file_info *fi);
int safs_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int safs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int safs_rename(const char *oldname, const char *newname);
