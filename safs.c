// gcc -Wall safs.c `pkg-config fuse --cflags --libs` -o safs

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "safs.h"

// how did we decide 80 bytes for filepath?
char filePath[80] = {0};

// how did we decide 32 bytes for char name?
struct safs_dir_entry {
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
  .link = safs_link
};

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
// Link
////////////////////////////////////////////////////////////////////////////////
int safs_link (const char *path1, const char *path2){
  int fd;
  struct safs_dir_entry dir;
  struct safs_dir_entry newDir;
  struct stat st;
  int rtn;
  int found = 0;
  int index;

  fd = open("/tmp/safs/Directory~", O_RDWR);

  if (fd < 0)
      return -EIO;

  while (found == 0) {
    rtn = read(fd, &dir, sizeof(dir));
    if (rtn != sizeof(dir))
      break;

    if (strcmp(path1+1, dir.name) == 0 && (dir.inum != 0)) {
      found = 1;
      break;
    }
  }

  if (found == 0) {
    close(fd);
    return -ENOENT;
  }

  // find not use directory entry
  index = 0;
  while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
    if (strcmp(dir.name,".") == 0 || strcmp(dir.name,"..") == 0) {
      index++;
      continue;
    }
    if (dir.inum == 0)
      break;
    index++;
  }

  // create new directory entry
  memset(&newDir, 0, sizeof(dir));
  strcpy(newDir.name, path2+1);
  strcpy(newDir.storage, dir.storage);
  newDir.inum = dir.inum;

  // write directory entry
  if (lseek(fd, index * sizeof(dir), SEEK_SET) < 0) {
    close(fd);
    return -EIO;
  }
  if (write(fd, &newDir, sizeof(dir)) != sizeof(dir)) {
    close(fd);
    return -EIO;
  }

  // increment inode link count
  read_inode(dir.inum, &st);
  st.st_nlink = st.st_nlink + 1;
  write_inode(dir.inum, st);

  close(fd);
  return 0;
}

int safs_chmod (const char *path, mode_t mode){
    struct safs_dir_entry dir;
    struct stat st;
    int index = 0;
    int found = 0;
    int fd;

    fd = open("/tmp/safs/Directory~", O_RDWR);
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

int safs_chown (const char *path, uid_t uid, gid_t gid){
  // TBD
  return 0;
}

int safs_utimens (const char *path, const struct timespec tv[2]){
  // TBD
  return 0;
}

int safs_truncate (const char *path, off_t off) {
  return safs_ftruncate(path, off, NULL);
}

int safs_ftruncate (const char *path, off_t off, struct fuse_file_info *fi){
    struct safs_dir_entry dir;
    struct stat st;
    char ch = 0;
    int fd;

    fd = open("/tmp/safs/Directory~", O_RDWR);
    if (fd < 0)
      return -EIO;

    // read directory to find file
    while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
      if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
        read_inode(dir.inum, &st);
        if (st.st_size != off) {
          if (st.st_size < off) {
            // extend file to offset
            safs_write(path, &ch, 1, off, fi);
          }
          st.st_size = off;
          write_inode(dir.inum, st);
        }
        close(fd);
        return 0;
      }
    }

    close(fd);
    return -ENOENT;
}

int safs_setattr (const char * path, const char *name, const char *value, size_t size, int flag){
  // TBD
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Unlink (delete)
////////////////////////////////////////////////////////////////////////////////
int safs_unlink (const char *path) {
  struct safs_dir_entry dir;
  struct stat st;
  int index = 0;
  int fd;
  int rtn;

  fd = open(directory_path, O_RDWR);
  if (fd < 0)
    return -EIO;

  while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
    if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
      read_inode(dir.inum, &st);
      st.st_nlink = st.st_nlink - 1;
      write_inode(dir.inum, st);

      // free directory enter
      dir.inum = 0;
      rtn = lseek(fd, index * sizeof(dir), SEEK_SET);
      if (rtn < 0) {
        close(fd);
        return -EIO;
      }

      rtn = write(fd, &dir, sizeof(dir));
      close(fd);

      if (st.st_nlink != 0)
        return 0; // reference count not zero

      // found, remove file storage
      // char file_path[];
      // strcat(file_path, safs_path);
      // strcat(file_path, dir.name)

      char filePath[80];
      strcpy(filePath, safs_path);
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
int safs_mknod(const char *filename, mode_t mode, dev_t dev){
  struct safs_dir_entry dir;
  struct timespec tm;
  struct stat st;
  int index = 0;
  int fd;
  int rtn;

  if ((mode & S_IFMT) != S_IFREG)
    return -EPERM;

  fd = open(directory_path, O_RDWR, 0666);
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
  strcpy(filePath, safs_path);
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
  st.st_ino = inode_counter++;

  // make new directory entry
  memset(&dir, 0, sizeof(dir));
  strcpy(dir.name, filename+1);
  strcpy(dir.storage, filename+1);
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
int safs_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  struct safs_dir_entry dir;
  struct stat st;
  int found = 0;
  int cnt = 0;
  int fd;
  int rtn;

  fd = open(directory_path, O_RDWR);
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
  write_inode(dir.inum, st);

  // write file
  strcpy(filePath, safs_path);
  strcat(filePath, dir.storage);

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

////////////////////////////////////////////////////////////////////////////////
// Read Directory
////////////////////////////////////////////////////////////////////////////////
static int safs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  struct safs_dir_entry dir;
  struct stat st;
  int fd_dir;

  if (strcmp(path, "/") != 0)
    return -ENOENT;

  fd_dir = open(directory_path, O_RDWR, 0666);
  if (fd_dir < 0)
    return -EIO;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

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
static int safs_open(const char *path, struct fuse_file_info *fi) {
  struct safs_dir_entry dir;
  struct stat st;
  int fd;

  fd = open(directory_path, O_RDWR);
  if (fd < 0)
    return -EIO;

  // read directory to find file
  while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
    if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
      read_inode(dir.inum, &st);
      if ((fi->flags & O_RDONLY) && (st.st_mode & 0444) == 0) {
        close(fd);
        return -EPERM;
      }
      if ((fi->flags & O_WRONLY) && (st.st_mode & 0222) == 0) {
        close(fd);
        return -EPERM;
      }
      if (fi->flags & O_TRUNC) {
        st.st_size = 0;
        write_inode(dir.inum, st);
      }
      close(fd);
      return 0;
    }
  }

  if (fi->flags & O_CREAT) {
    safs_mknod(path, 0666, 0);
  }

  close(fd);
  return -ENOENT;
}

////////////////////////////////////////////////////////////////////////////////
// Read a file.
////////////////////////////////////////////////////////////////////////////////
static int safs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  int fd;
  int cnt = 0;
  struct safs_dir_entry dir;
  int found = 0;
  int rtn;
  struct stat st;

  fd = open(directory_path, O_RDWR);

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
  clock_gettime(CLOCK_REALTIME, &st.st_atim);
  write_inode(dir.inum, st);

  // read file
  strcpy(filePath,safs_path);
  strcat(filePath, dir.storage);
  fd = open(filePath, O_RDONLY);
  if (fd < 0)
    return -EIO;
  lseek(fd, offset, SEEK_SET);
  rtn = read(fd,buf, size);

  // copy to user space
  memcpy(buf, buf, rtn);
  close(fd);
  return(rtn);
}

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

int main(int argc, char *argv[]) {

  // Initialize
  if (safs_init() < 0)
    exit(EXIT_FAILURE);

  // Attach Fuse
  return fuse_main(argc, argv, &safs_oper, NULL);
}
