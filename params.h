////////////////////////////////////////////////////////////////////////////////
// params.h
////////////////////////////////////////////////////////////////////////////////
// Here we list:
//   - included c and fuse libraries
//   - the fuse version we are using
//   - struct definitions
//   - constant variables or initializations
//     used throughout the project
////////////////////////////////////////////////////////////////////////////////

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// how did we decide 80 bytes for filepath?
char filePath[80] = {0};

// how did we decide 32 bytes for char name?
struct safs_dir_entry {
  char name[32];
  char storage[32];
  unsigned int inum;
};

struct safs_dentry {
  char name[32];
  char storage[32];
  unsigned int inum;
};

// Counters
int inode_counter = 1;

// Paths
char safs_path[] = "/tmp/safs/";
char inodes_path[] = "/tmp/safs/Inodes~";
char directory_path[] = "/tmp/safs/Directory~";

/* PERMISSIONS

  files
  - 777: no restrictions or permission, anything goes
  - 755: file's owner may read, write, and execute the file.
         all others may read and execute
  - 700: file's owner may read, write, and execute the file
         No other users have rights
  - 666: All users may read and write the file
  - 644: Owner may read and write a file, while all others
         may only read the file.
  - 600: Owner may read and write a file
         All others have no rights

  directories
  - 777: No restrictions or permissions
  - 755: Directory owner has full access
         All others may list the directory but cannot
         create or delete files
  - 700: Directory owner has full access
         Nobody else has rights */


/* FUSE STRUCTURES

struct fuse_file_info{
  int flags
  unsigned long fh_old
  int whitepage
  unsigned int direct_io: 1
  unsigned int keep_cache: 1
  unsigned int flush: 1
  unsigned int nonseekable: 1
  unsigned int padding: 27
  uint64_t fh
  uint64_t lock_owner
  uint64_t poll_events
} */
