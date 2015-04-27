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

/* SAFS_MKNODE
// creates a filesystem node named pathname, with attributes given by mode and dev.
// mode specifies permissions to use and type of node to be created
// It should be a combination (using bitwise OR) of one of the file types listed below
// and the permissions for the new node. */
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
// open or create a file as specified by path for writing or reading as specified by oflag
// Unless the 'default_permissions' mount option is given, open should check if the operation is permitted for the given flags.
   Optionally open may also return an arbitrary filehandle in the fuse_file_info structure, which will be passed to all file operations.

return values
     success => a non-negative integer, termed a file descriptor
     failure => -1 on failure, and sets errno to indicate the
     error => errno*/
static int safs_open(const char *path, struct fuse_file_info *fi);




static int safs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);


/* CHMOD
change the permissions of the given path as specified by mode bits
preconditions: owner of file or super-user is changing mode of path
postconditions: permissions of path are changed as specified by mode bits
return values:
  - return 0 => success | return > 1 if error */
int safs_chmod (const char *path, mode_t mode);

/* CHOWN
change the permissions of the given path as specified by mode bits
preconditions: owner of file or super-user is changing mode of path
postconditions: permissions of path are changed as specified by mode bits
return values:
  - return 0 => success | return > 1 if error */
int safs_chown (const char *path, uid_t uid, gid_t gid);



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
