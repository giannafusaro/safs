////////////////////////////////////////////////////////////////////////////////
// file_ops.c
////////////////////////////////////////////////////////////////////////////////
// Here we define file operations:
//   - mknode, readdir
//   - link, unlink
//   - truncate, ftruncate
//   - open, write, read, rename
////////////////////////////////////////////////////////////////////////////////

#include "file_ops.h"

////////////////////////////////////////////////////////////////////////////////
// Read directory
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
// Rename file
////////////////////////////////////////////////////////////////////////////////
int safs_rename(const char *oldname, const char *newname) {
    struct safs_dir_entry dir;
    int index=0;
    int fd;

    fd = open(directory_path, O_RDWR);
    if (fd < 0)
      return -EIO;

    // read directory to find file
    while (read(fd, &dir, sizeof(dir)) == sizeof(dir)) {
      if (strcmp(oldname+1, dir.name) == 0 && (dir.inum != 0)) {
    	strcpy(dir.name, newname+1);
    	lseek(fd, index * sizeof(dir), SEEK_SET);
    	write(fd, &dir, sizeof(dir));
        close(fd);
        return 0;
      }
      index++;
    }

    close(fd);
    return -ENOENT;
}
