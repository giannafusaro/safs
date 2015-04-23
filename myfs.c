// gcc -Wall myfs.c `pkg-config fuse --cflags --libs` -o myfs

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "myfs.h"

// how did we decide 80 bytes for filepath?
char filePath[80] = {0};
int inodeCnt = 1;

// how did we decide 32 bytes for char name?
struct myfs_dir_entry {
  char name[32];
  unsigned int inum;
};

// operations mapping
static struct fuse_operations myfs_oper = {
  .getattr = myfs_getattr,
  .readdir = myfs_readdir,
  .open = myfs_open,
  .read = myfs_read,
  .write = myfs_write,
  .mknod = myfs_mknod,
  .setxattr = myfs_setattr,
  .chmod = myfs_chmod,
  .chown = myfs_chown,
  .utimens = myfs_utimens,
  .utime = myfs_utime,
  .truncate = myfs_truncate,
  .ftruncate = myfs_ftruncate,
  .unlink = myfs_unlink,
  .link = myfs_link
};

////////////////////////////////////////////////////////////////////////////////
// Write inode stat from inode file
////////////////////////////////////////////////////////////////////////////////
int write_inode(const unsigned int in, struct stat st) {
  int fd;   /* file descriptor */
  int rtn;  /* returned value */

  fd = open("/tmp/myfs/Inodes~", O_CREAT|O_RDWR, 0666);
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

  fd = open("/tmp/myfs/Inodes~", O_CREAT|O_RDWR, 0666);
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
// TODO
////////////////////////////////////////////////////////////////////////////////
int myfs_link (const char *path1, const char *path2){
  // TBD
  return 0;
}

int myfs_chmod (const char *path, mode_t mode){
  // TBD
  return 0;
}

int myfs_chown (const char *path, uid_t uid, gid_t gid){
  // TBD
  return 0;
}

int myfs_utimens (const char *path, const struct timespec tv[2]){
  // TBD
  return 0;
}

int myfs_utime (const char *path, struct utimbuf *tb){
  // TBD
  return 0;
}

int myfs_truncate (const char *path, off_t off){
  // TBD
    return 0;
}

int myfs_ftruncate (const char *path, off_t off, struct fuse_file_info *fi){
  // TBD
  return 0;
}

int myfs_setattr (const char * path, const char *name, const char *value, size_t size, int flag){
  // TBD
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Unlink (delete)
////////////////////////////////////////////////////////////////////////////////
int myfs_unlink (const char *path) {
  struct myfs_dir_entry dir;
  struct stat st;
  int index = 0;
  int fd;
  int rtn;

  fd = open("/tmp/myfs/Directory~", O_RDWR);
  if (fd < 0)
    return -EIO;

  while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
    if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
      read_inode(dir.inum, &st);
      st.st_nlink = st.st_nlink - 1;
      write_inode(dir.inum, st);

      // reference count not zero
      if (st.st_nlink != 0)
        return 0;

      // free directory enter
      dir.inum = 0;
      rtn = lseek(fd, index * sizeof(dir), SEEK_SET);
      if (rtn < 0) {
        close(fd);
        return -EIO;
      }

      rtn = write(fd, &dir, sizeof(dir));
      close(fd);

      // found, remove file storage
      strcpy(filePath,"/tmp/myfs/");
      strcat(filePath, dir.name);
      unlink(filePath);

      if (rtn != sizeof(dir)) {
        return -EIO;
      } else {
        return 0;
      }
    }
    index++;
  }

  close(fd);
  return -ENOENT;
}

////////////////////////////////////////////////////////////////////////////////
// Make Node
////////////////////////////////////////////////////////////////////////////////
int myfs_mknod(const char *filename, mode_t mode, dev_t dev){
  struct myfs_dir_entry dir;
  struct timespec tm;
  struct stat st;
  int index = 0;
  int fd;
  int rtn;

  if ((mode & S_IFMT) != S_IFREG)
    return -EPERM;

  fd = open("/tmp/myfs/Directory~", O_RDWR, 0666);
  if (fd < 0)
    return -EIO;

  // find not use directory entry
  while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
    if (strcmp(dir.name,".") == 0 || strcmp(dir.name,"..") == 0) {
      index++;
      continue;
    }
    if (dir.inum == 0)
      break;

    index++;
  }

  // create file to save contents
  strcpy(filePath,"/tmp/myfs");
  strcat(filePath, filename);

  rtn = open(filePath, O_CREAT|O_TRUNC|O_RDONLY, mode);
  if (rtn < 0) {
    close(fd);
    return -EIO;
  }
  close(rtn);

  // initialize stat
  st.st_mode = mode;
  st.st_nlink = 1;
  st.st_size = 0;
  st.st_uid = getuid();
  st.st_gid = getgid();
  clock_gettime(CLOCK_REALTIME, &tm);
  st.st_atim = tm;
  st.st_mtim = tm;
  st.st_ctim = tm;
  st.st_ino = inodeCnt++;

  // make new directory entry
  memset(&dir, 0, sizeof(dir));
  strcpy(dir.name, filename+1);
  dir.inum = st.st_ino;

  // write inode
  rtn = write_inode(st.st_ino, st);
  if (rtn != 0) {
    close(fd);
    return(rtn);
  }

  // create directory entry
  if (lseek(fd, index * sizeof(dir), SEEK_SET) < 0) {
    close(fd);
    return -EIO;
  }

  if (write(fd, &dir, sizeof(dir)) != sizeof(dir)) {
    close(fd);
    return -EIO;
  }

  close(fd);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Write to a file
////////////////////////////////////////////////////////////////////////////////
int myfs_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  struct myfs_dir_entry dir;
  struct stat st;
  int found = 0;
  int cnt = 0;
  int fd;
  int rtn;

  fd = open("/tmp/myfs/Directory~", O_RDWR);
  if (fd < 0)
    return -EIO;

  // find directory entry
  while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
    if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
      found = 1;
      break;
    }
    cnt = cnt + 1;
  }

  if (found == 0) {
    close(fd);
    return -ENOENT;
  }
  close(fd);

  // update inode
  read_inode(dir.inum, &st);
  st.st_size = offset + size;
  clock_gettime(CLOCK_REALTIME, &st.st_atim);
  clock_gettime(CLOCK_REALTIME, &st.st_mtim);
  rtn = lseek(fd, cnt * sizeof(dir), SEEK_SET);
  rtn = write(fd, &dir, sizeof(dir));
  write_inode(dir.inum, st);

  // write file
  strcpy(filePath,"/tmp/myfs/");
  strcat(filePath, dir.name);

  fd = open(filePath, O_WRONLY);
  if (fd < 0)
    return -EIO;

  lseek(fd, offset, SEEK_SET);
  rtn = write(fd,buf, size);

  close(fd);
  return(rtn);
}

////////////////////////////////////////////////////////////////////////////////
// Get file attributes.
////////////////////////////////////////////////////////////////////////////////
static int myfs_getattr(const char *path, struct stat *stbuf) {
  struct myfs_dir_entry dir;
  int res = 0;
  int fd;

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else {
    fd = open("/tmp/myfs/Directory~", O_RDWR,0666);
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
  }

  close(fd);
  return res;
}

////////////////////////////////////////////////////////////////////////////////
// Read Directory
////////////////////////////////////////////////////////////////////////////////
static int myfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  struct myfs_dir_entry dir;
  struct stat st;
  int fd_dir;

  if (strcmp(path, "/") != 0)
    return -ENOENT;

  fd_dir = open("/tmp/myfs/Directory~", O_RDWR, 0666);
  if (fd_dir < 0)
    return -EIO;

  while (read(fd_dir, &dir, sizeof(dir)) == sizeof(dir)) {
    if (dir.inum != 0) {
      // read stat from inode
      read_inode(dir.inum, &st);
      // give to fuse
      filler(buf, dir.name, &st, 0);
    }
  }

  if (fd_dir >= 0)
    close(fd_dir);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Open a file.
////////////////////////////////////////////////////////////////////////////////
static int myfs_open(const char *path, struct fuse_file_info *fi) {
  struct myfs_dir_entry dir;
  int fd;

  fd = open("/tmp/myfs/Directory~", O_RDWR);
  if (fd < 0)
    return -EIO;

  // read directory to find file
  while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
    if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
      close(fd);
      return 0;
    }
  }

  close(fd);
  return -ENOENT;
}

////////////////////////////////////////////////////////////////////////////////
// Read a file.
////////////////////////////////////////////////////////////////////////////////
static int myfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  int fd;
  int rtn = 0;
  char myPath[80] = {0};
  static char iobuf[4096];

  strcat(myPath,"/tmp/myfs");
  strcat(myPath, path);

  fd = open(myPath, O_RDONLY);
  if (fd < 0)
    return -EIO;

  // seek to position
  if (lseek(fd, offset, SEEK_SET) == ENXIO) {
    close(fd);
    return 0;
  }

  // read content
  rtn = read(fd, &iobuf, size);
  if (rtn < 0) {
    close(fd);
    return(rtn);
  }

  // copy to user space
  memcpy(buf, iobuf, rtn);

  close(fd);
  return rtn;
}

////////////////////////////////////////////////////////////////////////////////
// Initialization
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
  struct myfs_dir_entry dir;
  struct stat st;
  int fd_dir;
  int rtn;

  // create directory and inode files
  fd_dir = open("/tmp/myfs/Directory~", O_RDWR, 0666);
  if (fd_dir < 0) {
    rtn = mkdir("/tmp/myfs",0777);

    // truncate inodes file
    fd_dir = open("/tmp/myfs/Inodes~", O_CREAT|O_RDWR|O_TRUNC, 0666);
    if (fd_dir < 0)
      return -EIO;
    close(fd_dir);

    // create directory file
    fd_dir = open("/tmp/myfs/Directory~", O_CREAT|O_RDWR, 0666);
    if (fd_dir < 0)
      return -EIO;

    // make . directory entry
    memset(&dir, 0, sizeof(dir));
    strcpy(dir.name, ".");
    st.st_mode = S_IFDIR | 0755;
    st.st_nlink = 1;
    st.st_size = 0;
    st.st_ino = inodeCnt++;
    st.st_uid = getuid();
    st.st_gid = getgid();
    clock_gettime(CLOCK_REALTIME, &st.st_atim);
    clock_gettime(CLOCK_REALTIME, &st.st_mtim);
    clock_gettime(CLOCK_REALTIME, &st.st_ctim);
    dir.inum = st.st_ino;
    write(fd_dir, &dir, sizeof(dir));

    rtn = write_inode(st.st_ino, st);
    if (rtn != 0) {
      close(fd_dir);
      return(rtn);
    }

    // make .. directory entry
    strcpy(dir.name, "..");
    st.st_mode = S_IFDIR | 0755;
    st.st_nlink = 1;
    st.st_size = 0;
    st.st_ino = inodeCnt++;
    st.st_uid = getuid();
    st.st_gid = getgid();
    clock_gettime(CLOCK_REALTIME, &st.st_atim);
    clock_gettime(CLOCK_REALTIME, &st.st_mtim);
    clock_gettime(CLOCK_REALTIME, &st.st_ctim);
    dir.inum = st.st_ino;
    write(fd_dir, &dir, sizeof(dir));

    rtn = write_inode(st.st_ino, st);
    if (rtn != 0) {
      close(fd_dir);
      return(rtn);
    }
  }

  return fuse_main(argc, argv, &myfs_oper, NULL);
}
