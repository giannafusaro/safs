////////////////////////////////////////////////////////////////////////////////
//////// safs declarations
////////////////////////////////////////////////////////////////////////////////


/* MANDATORY METHODS
      stat()
      open()
      read()
      write()
      close()
      fseek()
      unlink()
      fsync()
      lsmod()
      insmod()
      rmmod()
      ls -al
      cat > filename
      touch file
      -vi file
      <echo "File"> file
*/







int read_inode(const unsigned int in, struct stat *stp);
int write_inode(const unsigned int in, struct stat st);

/* SAFS_LINK
// creates a hard link to an existing file
preconditions:
  - new path does not exist, user must have sufficient storage, etc (see man page for link(2))
postconditions:
  - new name functions exactly like old one, and refers to the same file
  - new name has same permissions and and ownership, cannot tell which link is original
return values:
  - success => 0, failure => -1, or error => errno */
int safs_link (const char *path1, const char *path2);

/* SAFS_UNLINK
// deletes a name from the file system.
preconditions:
  - path must exist, etc. see man unlink() page for details
postconditions:
  - if name links to file not open by any processes, the file name and file is deleted
  - if name links to file open by processes, the link is deleted but the file remains until any
    all file descriptors referring to that file are closed
  - if name links to symbolic link, the link is removed
return values: success => 0, failure => -1, or error => errno */
int safs_unlink (const char *path);



int safs_utimens (const char *path, const struct timespec tv[2]);
int safs_utime (const char *path, struct utimbuf *tb);


/* SAFS_FTRUNCATE, SAFS_FTRUNCATE
// truncate or extend a file, given by path or file descriptor, to a specified length bytes in size
preconditions:
  files to be modified cannot have open file descriptors associated with file
  for SAFS_FTRUNCATE, the file to be modified must be open for writing
postconditions:
  for files exceeding specified length, extra data is discarded
  for files smaller specified length, file is extended with extra data (zeros to indicated length)
return values: success => 0, failure => -1, or error => errno */
int safs_truncate (const char *path, off_t off);
int safs_ftruncate (const char *path, off_t off, struct fuse_file_info *fi);


int safs_setattr (const char *path, const char *name, const char *value, size_t size, int flag);
static int safs_getattr(const char *path, struct stat *stbuf);


static int safs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

int safs_mknod (const char *filename, mode_t mode, dev_t dev);

/* SASF_WRITE
// writes up to size bytes to the given path from the buffer pointed to by buf
// if lseek() calls write, write will begin writing bytes at the offset and the offset is incremented by # bytes written
// if o_append flag is set, file offset is set to the end of the file
// if size is 0 and file is regular file => errno (if error) | 0 (if file is not regular, no results specified)
// precondition:
    for all bytes specified by size to be written to the file, the physical storage must have sufficient size
// postcondition:
    size number of bytes are written to the file as given by the path from buffer pointed to by buf
// return value: success => number of bytes written, failure => -1, error => errno */
int safs_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);


/* SASF_OPEN
// opens a file, directory, or URL just as if you had double-clicked the file's icon. If no application name is spec-
     ified, the default application as determined via LaunchServices is used to open the specified files.
preconditions:
postconditions: opened applications inherit environment variables just as if you had launched the application directly through its full path
                ex// If the file is in the form of a URL, the file will be opened as a URL.
                You can specify one or more file names (or pathnames), which are interpreted relative to the shell or Terminal window's current working
                directory. For example, the following command would open all Word files in the current working directory:
                open *.doc */
static int safs_open(const char *path, struct fuse_file_info *fi);




static int safs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);




/////////////////////////////
/////// UNNECESSARY /////////
/////////////////////////////
/* CHMOD
preconditions: owner of file or super-user is changing mode of path
purpose: change the permissions of the given path as specified by mode bits
postconditions: return 0 => success | return > 1 if error */
// int safs_chmod (const char *path, mode_t mode);
// int safs_chown (const char *path, uid_t uid, gid_t gid);
