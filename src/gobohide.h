#ifndef _LINUX_GOBOHIDE_H
#define _LINUX_GOBOHIDE_H

/* Gobolinux internal ioctls */

#define GOBOHIDE_HIDEINODE   0x0000001 /* Hide a given inode number */
#define GOBOHIDE_UNHIDEINODE 0x0000002 /* Unhide a given inode number */
#define GOBOHIDE_COUNTHIDDEN 0x0000003 /* Get the number of inodes hidden */
#define GOBOHIDE_GETHIDDEN   0x0000004 /* Get the inodes hidden */

struct gobolinux_hide_stats {
	int hidden_inodes;      /* how many inodes we're hiding */
	int filled_size;        /* how many inodes we filled on the hidden_list */
	char **hidden_list;     /* the hidden list */
};

struct gobolinux_hide {
	char operation;                     /* the operation to be performed */
	ino_t inode;                        /* the inode number */
	const char *pathname;               /* the pathname being submitted */
	char symlink;                       /* is inode a symlink? */
	struct gobolinux_hide_stats stats;  /* statistics about the inodes being hidden */
};

#endif  /* _LINUX_GOBOHIDE_H */
