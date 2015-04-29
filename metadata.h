////////////////////////////////////////////////////////////////////////////////
// Metadata Prototypes
////////////////////////////////////////////////////////////////////////////////
int write_inode(const unsigned int in, struct stat st);
int read_inode(const unsigned int in, struct stat *stp);
int safs_chmod (const char *path, mode_t mode);
int safs_utimens (const char *path, const struct timespec tv[2]);
int safs_setattr (const char * path, const char *name, const char *value, size_t size, int flag);
static int safs_getattr(const char *path, struct stat *stbuf);
