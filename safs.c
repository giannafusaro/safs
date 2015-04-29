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

struct safs_dentry {
  char name[32];
  char storage[32];
  unsigned int inum;
};

// Counters
int inode_number = 1;

// Paths
char safs_path[] = "/tmp/safs/";
char inodes_path[] = "/tmp/safs/Inodes~";
char directory_path[] = "/tmp/safs/Directory~";
char tmp_directory_path[] = "/tmp/safs/.Directory~";

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
  int fd;  /* file descriptor */
  int rtn;  /* returned value */

  printf("\n<write inode>\n");
  printf("  st.ino: %zu \n", st.st_ino);

  fd = open(inodes_path, O_CREAT|O_RDWR, 0666);
  if (fd < 0)
    return fd;

  printf("  fd: %d \n", fd);

  rtn = lseek(fd, in * sizeof(st), SEEK_SET);
  if (rtn < 0) {
    close(fd);
    return rtn;
  }

  printf("  lseek rtn: %d \n", rtn);

  rtn = write(fd, &st, sizeof(st));
  if (rtn != sizeof(st)) {
    close(fd);
    return rtn;
  }

  printf("  write rtn: %d \n", rtn);
  printf("</write node>\n\n");
  close(fd);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Read inode stat from inode file
////////////////////////////////////////////////////////////////////////////////
int read_inode(off_t offset, struct stat *metadata) {
  int inode;

  // Get Inodes~ file descriptor
  inode = open(inodes_path, O_CREAT|O_RDWR, 0666);
  if (inode < 0)
    return inode;

  // Determine offset location (in bytes from beginning of file)
  if (lseek(inode, offset * sizeof(struct stat), SEEK_SET) < 0) {
    close(inode);
    return -errno;
  }

  // Read bytes from inode into the buffer. Offset is incremented by
  // number of bytes read. If current offset == || > EOF, no bytes are read.
  if (read(inode, metadata, sizeof(struct stat)) != sizeof(struct stat)) {
    close(inode);
    return -errno;
  }

  close(inode);
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

  fd = open(directory_path, O_RDWR);

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

  // create new directory entry
  memset(&newDir, 0, sizeof(dir));
  strcpy(newDir.name, path2+1);
  strcpy(newDir.storage, dir.storage);
  newDir.inum = dir.inum;

  // find not use directory entry
  index = 0;
  lseek(fd, 0, SEEK_SET);
  while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
    if (strcmp(dir.name,".") == 0 || strcmp(dir.name,"..") == 0) {
      index++;
      continue;
    }
    if (dir.inum == 0)
      break;
    index++;
  }

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
int safs_chown (const char *path, uid_t uid, gid_t gid){
  // TBD
  return 0;
}

int safs_utimens (const char *path, const struct timespec tv[2]){
    struct safs_dir_entry dir;
    struct timespec tm;
    struct stat st;
    int fd;

    fd = open(directory_path, O_RDWR);
    if (fd < 0)
      return -EIO;

    // read directory to find file
    while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
      if (strcmp(path+1, dir.name) == 0 && (dir.inum != 0)) {
        read_inode(dir.inum, &st);
        clock_gettime(CLOCK_REALTIME, &tm);
        st.st_atim = tv[0];
        st.st_mtim = tv[1];
        write_inode(dir.inum, st);
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
// Truncate
////////////////////////////////////////////////////////////////////////////////
int safs_truncate (const char *path, off_t off) {
  return safs_ftruncate(path, off, NULL);
}

////////////////////////////////////////////////////////////////////////////////
// FTruncate
////////////////////////////////////////////////////////////////////////////////
int safs_ftruncate (const char *path, off_t off, struct fuse_file_info *fi){
  struct safs_dir_entry dir;
  struct stat st;
  char ch = 0;
  int fd;

  fd = open(directory_path, O_RDWR);
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

////////////////////////////////////////////////////////////////////////////////
// Unlink (delete)
////////////////////////////////////////////////////////////////////////////////
int safs_unlink (const char *path) {
  struct safs_dentry dentry;
  struct stat metadata;
  int tmp_directory;
  int new_directory;
  int new_inode_number = 2;

  // Rename Directory~ to .Directory~
  if (rename(directory_path, tmp_directory_path) < 0)
    return -errno;

  // Get .Directory~ file descriptor
  tmp_directory = open(tmp_directory_path, O_RDWR);
  if (tmp_directory < 0)
    return -errno;

  // Create new Directory~ file descriptor
  new_directory = open(directory_path, O_CREAT|O_RDWR, 0666);
  if (new_directory < 0)
    return -errno;

  // Build a new Directory~
  while (read(tmp_directory, &dentry, sizeof(dentry)) == sizeof(dentry)) {
    if (dentry.name[0] != '\0') {
      printf("  [unlink loop] %s -> %s \n", path+1, dentry.name);
      if (strcmp(path+1, dentry.name) == 0) {

        // Remove hardlinks
        read_inode(dentry.inum, &metadata);
        metadata.st_nlink -= 1;
        write_inode(dentry.inum, metadata);

        // Remove actual file from safs_path
        strcpy(filePath, safs_path);
        strcat(filePath, dentry.name);
        printf("filePath: %s \n", filePath);
        dentry.inum = 0;
        if (unlink(filePath) < 0)
          return -errno;

      } else {

        // Calculate new offset
        if (lseek(new_directory, new_inode_number * sizeof(dentry), SEEK_SET) < 0) {
          close(new_directory);
          return -errno;
        }

        // Write to the new Directory~
        if (write(new_directory, &dentry, sizeof(dentry)) != sizeof(dentry)) {
          close(new_directory);
          return -errno;
        } else {
          new_inode_number++;
        }
      }
    }
  }

  close(new_directory);
  close(tmp_directory);
  if (unlink(tmp_directory_path) < 0) {
    return -errno;
  } else {
    return 0;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Make Node
////////////////////////////////////////////////////////////////////////////////
int safs_mknod(const char *filename, mode_t mode, dev_t dev){
  struct safs_dentry dentry;
  struct timespec timestamp;
  struct stat metadata;
  int directory;

  printf("\n<MKNOD>\n");

  // Mode must be a regular file
  if ((mode & S_IFMT) != S_IFREG)
    return -EPERM;

  // Get Directory~ file descriptor
  directory = open(directory_path, O_RDWR, 0666);
  if (directory < 0)
    return -EIO;

  printf("new inode number: %d \n", inode_number);

  // create file to save contents
  strcpy(filePath, safs_path);
  strcat(filePath, filename+1);

  printf("filepath: %s \n", filePath);

  // Create file
  if (open(filePath, O_CREAT|O_TRUNC|O_RDONLY, mode) < 0) {
    close(directory);
    return -EIO;
  }

  // Set Metadata
  metadata.st_mode = mode;
  metadata.st_nlink = 1;
  metadata.st_size = 0;
  metadata.st_uid = getuid();
  metadata.st_gid = getgid();
  clock_gettime(CLOCK_REALTIME, &timestamp);
  metadata.st_atim = timestamp;
  metadata.st_mtim = timestamp;
  metadata.st_ctim = timestamp;
  metadata.st_ino = inode_number;

  // make new directory entry
  memset(&dentry, 0, sizeof(dentry));
  strcpy(dentry.name, filename+1);
  strcpy(dentry.storage, filename+1);
  dentry.inum = metadata.st_ino;

  printf("dentry.inum: %d \nmetadata.st_ino: %zu \n", dentry.inum, metadata.st_ino);

  // write inode
  if (write_inode(metadata.st_ino, metadata) != 0) {
    close(directory);
    return -EIO;
  }

  // calculate offset in Directory~ file to write
  if (lseek(directory, inode_number * sizeof(dentry), SEEK_SET) < 0) {
    close(directory);
    return -EIO;
  }

  // create directory entry
  if (write(directory, &dentry, sizeof(dentry)) != sizeof(dentry)) {
    close(directory);
    return -EIO;
  }

  inode_number++;

  printf("</MKNOD>\n\n");

  close(directory);
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

  printf("\n<write>\n");

  fd = open(directory_path, O_RDWR);
  if (fd < 0)
    return -EIO;

  // find directory entry
  while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
    printf("  [loop] %s -> %d \n", dir.name, dir.inum);
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
  printf("dir.inum: %d \n", dir.inum);

  st.st_size = offset + size;
  clock_gettime(CLOCK_REALTIME, &st.st_atim);
  clock_gettime(CLOCK_REALTIME, &st.st_mtim);
  write_inode(dir.inum, st);

  printf("dir.inum: %d \n", dir.inum);

  // write file
  strcpy(filePath, safs_path);
  strcat(filePath, dir.storage);
  printf("filePath: %s \n", filePath);

  fd = open(filePath, O_WRONLY);
  if (fd < 0)
    return -EIO;

  lseek(fd, offset, SEEK_SET);
  rtn = write(fd,buf, size);

  close(fd);
  printf("\n</write>\n");

  return(rtn);
}

////////////////////////////////////////////////////////////////////////////////
// Get file attributes.
////////////////////////////////////////////////////////////////////////////////
static int safs_getattr(const char *path, struct stat *metadata) {
  struct safs_dentry dentry;
  int directory;

  // Attributes for root node
  if (strcmp(path, "/") == 0) {
    metadata->st_mode = S_IFDIR | 0755;
    metadata->st_nlink = 2;
    return 0;
  }

  // get Directory~ file descriptor
  directory = open(directory_path, O_RDWR, 0666);
  if (directory < 0)
    return -EIO;

  // find directory entry
  while (read(directory, &dentry, sizeof(dentry)) == sizeof(dentry)) {
    if (strcmp(dentry.name,path+1) == 0 && (dentry.inum != 0)) {
      read_inode(dentry.inum, metadata);
      close(directory);
      return 0;
    }
  }

  close(directory);
  return -ENOENT;
}

////////////////////////////////////////////////////////////////////////////////
// Read Directory
////////////////////////////////////////////////////////////////////////////////
static int safs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
  struct safs_dentry dentry;
  struct stat stat;
  int directory;

  printf("\n<READ DIR>\n");
  printf("path: %s \n", path);

  // Flat file systems don't have folders...
  if (strcmp(path, "/") != 0)
    return -ENOENT;

  // Get Directory~ file descriptor
  directory = open(directory_path, O_RDWR, 0666);
  if (directory < 0)
    return -EIO;

  // Add dot entries
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  // Add remaining dentries
  while (read(directory, &dentry, sizeof(dentry)) == sizeof(dentry)) {
    printf("  [loop] inum: %u, name: %s \n", dentry.inum, dentry.name);
    if (dentry.inum != 0) {
      read_inode(dentry.inum, &stat);      // read stat from inode
      filler(buf, dentry.name, &stat, 0);  // give to fuse
    }
  }
  printf("</READ DIR>\n\n");

  // Close Directory~ file
  if (directory >= 0)
    close(directory);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Open a file.
////////////////////////////////////////////////////////////////////////////////
static int safs_open(const char *path, struct fuse_file_info *fi) {
  struct safs_dentry dentry;
  struct stat metadata;
  int directory;

  // Get Directory~ file descriptor
  directory = open(directory_path, O_RDWR);
  if (directory < 0)
    return -EIO;

  printf("\n<OPEN>\n");
  printf("path: %s \n", path);

  // Check Directory~ for file
  while (read(directory, &dentry, sizeof(dentry)) == sizeof(dentry)) {
    printf("   [loop] %s -> %d \n", dentry.name, dentry.inum);
    if (strcmp(path+1, dentry.name) == 0 && (dentry.inum != 0)) {
      printf("   found!\n");
      read_inode(dentry.inum, &metadata); // Get Inode metadata

      // Can't write to a read-only file
      if ((fi->flags & O_RDONLY) && (metadata.st_mode & 0444) == 0) {
        close(directory);
        return -EPERM;
      }

      // Can't read from a write-only file
      if ((fi->flags & O_WRONLY) && (metadata.st_mode & 0222) == 0) {
        close(directory);
        return -EPERM;
      }

      // Truncate file
      if (fi->flags & O_TRUNC) {
        metadata.st_size = 0;
        write_inode(dentry.inum, metadata);
      }

      close(directory);
      printf("</OPEN>\n\n");
      return 0;
    }
  }

  close(directory);
  printf("couldn't find node.\n</OPEN>\n\n");
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
  struct safs_dentry dentry;
  int directory;

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
  directory = open(directory_path, O_CREAT|O_RDWR, 0666);
  if (directory < 0) {
    printf("Could not create %s \n", directory_path);
    return -errno;
  } else {
    // Set the current inode_number
    while (read(directory, &dentry, sizeof(dentry)) == sizeof(dentry))
      if (dentry.inum > inode_number)
        inode_number = dentry.inum;
    close(directory);
  }

  printf("initial inode number: %d \n", inode_number);
  return 0;
}

int main(int argc, char *argv[]) {

  // Initialize
  if (safs_init() < 0)
    exit(EXIT_FAILURE);

  // Attach Fuse
  return fuse_main(argc, argv, &safs_oper, NULL);
}
