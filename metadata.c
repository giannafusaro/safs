////////////////////////////////////////////////////////////////////////////////
// metadata.c
////////////////////////////////////////////////////////////////////////////////
// Here we define metadata operations:
//   - write_inode, read_inode
//   - get_attr, set_attr, utimes
//   - chmod
//   - open, write, read, rename
////////////////////////////////////////////////////////////////////////////////

#include "metadata.h"

////////////////////////////////////////////////////////////////////////////////
// Write inode stat from inode file
////////////////////////////////////////////////////////////////////////////////
int write_inode(const unsigned int in, struct stat st) {
  int fd;   /* file descriptor */
  int rtn;  /* returned value */

  fd = open(inodes_path, O_CREAT|O_RDWR, 0666);
  if (fd < 0)
    return fd;

  rtn = lseek(fd, in * sizeof(st), SEEK_SET);
  if (rtn < 0) {
    close(fd);
    return rtn;
  }

  rtn = write(fd, &st, sizeof(st));
  if (rtn != sizeof(st)) {
    close(fd);
    return rtn;
  }

  close(fd);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Read inode stat from inode file
////////////////////////////////////////////////////////////////////////////////
int read_inode(const unsigned int in, struct stat *stp) {
  int fd;   /* file descriptor */
  int rtn;  /* returned value */

  fd = open(inodes_path, O_CREAT|O_RDWR, 0666);

  // Inodes file not found
  if (fd < 0)
    return fd;

  rtn = lseek(fd, in * sizeof(struct stat), SEEK_SET);
  if (rtn < 0) {
    close(fd);
    return rtn;
  }

  rtn = read(fd, stp, sizeof(struct stat));
  if (rtn != sizeof(struct stat)) {
    close(fd);
    return rtn;
  }

  close(fd);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Chmod
////////////////////////////////////////////////////////////////////////////////
int safs_chmod (const char *path, mode_t mode){
    struct safs_dir_entry dir;
    struct stat st;
    int index = 0;
    int found = 0;
    int fd;

    fd = open(directory_path, O_RDWR);
    if (fd < 0)
      return -EIO;

    while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
      if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
        read_inode(dir.inum, &st);
        st.st_mode = mode;
        write_inode(dir.inum, st);
        found = 1;
        break;
      }
      index++;
    }

    close(fd);
    if (found)
      return 0;
    else
      return -ENOENT;
}

////////////////////////////////////////////////////////////////////////////////
// TODO
////////////////////////////////////////////////////////////////////////////////
int safs_chown (const char *path, uid_t uid, gid_t gid) {
  // TBD
  return 0;
}

int safs_utimens (const char *path, const struct timespec tv[2]) {
  // TBD
  return 0;
}

int safs_setattr (const char * path, const char *name, const char *value, size_t size, int flag) {
  // TBD
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Stat
////////////////////////////////////////////////////////////////////////////////
int stat(const char *path, struct stat *buffer) {
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get file attributes.
////////////////////////////////////////////////////////////////////////////////
static int safs_getattr(const char *path, struct stat *stbuf) {
  struct safs_dir_entry dir;
  int res = 0;
  int fd;

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else {
    fd = open(directory_path, O_RDWR,0666);
    if (fd < 0)
      return -EIO;

    // find directory entry
    while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
      if (strcmp(dir.name,path+1) == 0 && (dir.inum != 0)) {
        read_inode(dir.inum, stbuf);
        close(fd);
        return 0;
      }
    }
    res = -ENOENT;
    close(fd);
  }
  return res;
}
