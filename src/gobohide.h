#ifndef __GOBOHIDE_H
#define __GOBOHIDE_H

#define GOBOHIDE_GENL_NAME    "gobohide"
#define GOBOHIDE_GENL_VERSION  0x01

/* netlink commands */
enum {
	GOBOHIDE_CMD_INVALID = 0,
	GOBOHIDE_CMD_HIDE,       /* userspace -> kernel */
	GOBOHIDE_CMD_UNHIDE,     /* userspace -> kernel */
	GOBOHIDE_CMD_FLUSH,      /* userspace -> kernel */
	GOBOHIDE_CMD_LIST,       /* userspace -> kernel */
	GOBOHIDE_CMD_LIST_SIZE,  /* kernel -> userspace */
	GOBOHIDE_CMD_LIST_REPLY, /* kernel -> userspace */
	__GOBOHIDE_CMD_MAX
};
#define GOBOHIDE_CMD_MAX (__GOBOHIDE_CMD_MAX - 1)

/* netlink policies */
enum {
	GOBOHIDE_CMD_ATTR_UNSPEC = 0,
	GOBOHIDE_CMD_ATTR_PATH,
	GOBOHIDE_CMD_ATTR_INODE,
	__GOBOHIDE_CMD_ATTR_MAX,
};
#define GOBOHIDE_CMD_ATTR_MAX (__GOBOHIDE_CMD_ATTR_MAX - 1)

/* netlink data types (kernel -> userspace) */
enum {
	GOBOHIDE_TYPE_UNSPECT = 0,
	GOBOHIDE_TYPE_PATH,
	GOBOHIDE_TYPE_LIST_SIZE,
	__GOBOHIDE_TYPE_MAX,
};
#define GOBOHIDE_TYPE_MAX (__GOBOHIDE_TYPE_MAX - 1)

#endif /* __GOBOHIDE_H */
