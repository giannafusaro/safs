// gcc -Wall safs.c `pkg-config fuse --cflags --libs` -o safs

////////////////////////////////////////////////////////////////////////////////
// safs.c
////////////////////////////////////////////////////////////////////////////////
// Here we:
//   - include c our params, file and metadata operations
//   - create a persisted inodes file for storing file info
//   - create a persisted directory in /tmp/safs
//   - create a root path for all files in safs to save to
//   - call fuse main to create a socket pair, load the kernel module,
//   - and mount the filesystem to the mount point specified by user
////////////////////////////////////////////////////////////////////////////////

#include "params.h"
#include "metadata.c"
#include "file_ops.c"

////////////////////////////////////////////////////////////////////////////////
// Initialization
////////////////////////////////////////////////////////////////////////////////
static int safs_init() {

  // Create /tmp/safs
  if (mkdir(safs_path, 0777) < 0 && errno != EEXIST) {
    printf("Could not create or access %s \n", safs_path);
    return -errno;
  }

  // Create /tmp/safs/Inodes~
  if (open(inodes_path, O_CREAT|O_RDWR, 0666) < 0) {
    printf("Could not create %s \n", directory_path);
    return -errno;
  }

  // Create /tmp/safs/Directory~
  if (open(directory_path, O_CREAT|O_RDWR, 0666) < 0) {
    printf("Could not create %s \n", directory_path);
    return -errno;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {

  // Operations Mapping
  static struct fuse_operations safs_oper = {
    .getattr = safs_getattr,
    .readdir = safs_readdir,
    .open = safs_open,
    .read = safs_read,
    .write = safs_write,
    .mknod = safs_mknod,
    .setxattr = safs_setattr,
    .chmod = safs_chmod,
    .chown = safs_chown,
    .utimens = safs_utimens,
    .truncate = safs_truncate,
    .ftruncate = safs_ftruncate,
    .unlink = safs_unlink,
    .link = safs_link,
    .rename = safs_rename
  };

  // Initialize
  if (safs_init() < 0)
    exit(EXIT_FAILURE);

  // Attach Fuse
  return fuse_main(argc, argv, &safs_oper, NULL);
}
