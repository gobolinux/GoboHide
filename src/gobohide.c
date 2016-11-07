/* gobohide.c: Set/Unset the "hide directory" flag to a directory */

/*
 * Copyright (C) 2002-2016 GoboLinux.org
 *
 * This program is Free Software; you can redistributed it
 * and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * Authors: Felipe W Damasio <felipewd@terra.com.br>
 *          Lucas C. Villa Real <lucasvr@gobolinux.org>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <locale.h>
#include <libintl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/netlink.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>
#include <netlink/cli/utils.h>
#include "gobohide.h"

static struct nla_policy gobohide_kernel2user_policy[GOBOHIDE_TYPE_MAX+1] = {
	[GOBOHIDE_TYPE_PATH]  = { .type = NLA_STRING },
	[GOBOHIDE_TYPE_LIST_SIZE] = { .type = NLA_U32 },
};

#define _(message) gettext(message)

static const char version[] = "1.0";
static uint32_t gobohide_list_size;

static int parse_cmd_list_size(struct nl_cache_ops *unused, struct genl_cmd *cmd,
			 struct genl_info *info, void *arg)
{
	if (! info->attrs[GOBOHIDE_TYPE_LIST_SIZE]) {
		fprintf(stderr, "Invalid reply message received\n");
		return NL_SKIP;
	}
	gobohide_list_size = nla_get_u32(info->attrs[GOBOHIDE_TYPE_LIST_SIZE]);
	return 0;
}

static int parse_cmd_list_reply(struct nl_cache_ops *unused, struct genl_cmd *cmd,
			 struct genl_info *info, void *arg)
{
	struct nlattr *attrs[GOBOHIDE_TYPE_MAX+1];
	char *pathattr;
	int ret;

	if (! info->attrs[GOBOHIDE_TYPE_PATH]) {
		fprintf(stderr, "Invalid reply message received\n");
		return NL_SKIP;
	}

	ret = nla_parse(attrs, GOBOHIDE_TYPE_MAX, info->attrs[GOBOHIDE_TYPE_PATH],
			PATH_MAX, gobohide_kernel2user_policy);
	if (ret < 0) {
		nl_perror(ret, "Error while parsing the generic netlink message");
		return ret;
	}

	pathattr = nla_get_string(attrs[GOBOHIDE_TYPE_PATH]);
	if (! pathattr) {
		fprintf(stderr, "Error while parsing ATTR_PATH\n");
		return NL_SKIP;
	}
	printf("%s\n", pathattr);
	return 0;
}

static int parse_cb(struct nl_msg *msg, void *arg)
{
	return genl_handle_msg(msg, NULL);
}

static struct genl_cmd cmds[] = {
	{
		.c_id		= GOBOHIDE_CMD_LIST_SIZE,
		.c_name		= "GOBOHIDE_LIST_SIZE",
		.c_maxattr	= GOBOHIDE_CMD_MAX,
		.c_attr_policy	= gobohide_kernel2user_policy,
		.c_msg_parser	= &parse_cmd_list_size,
	},
	{
		.c_id		= GOBOHIDE_CMD_LIST_REPLY,
		.c_name		= "GOBOHIDE_LIST_REPLY",
		.c_maxattr	= GOBOHIDE_CMD_MAX,
		.c_attr_policy	= gobohide_kernel2user_policy,
		.c_msg_parser	= &parse_cmd_list_reply,
	},
};

#define ARRAY_SIZE(X) (sizeof(X) / sizeof((X)[0]))

static struct genl_ops ops = {
	.o_name = "gobohide",
	.o_cmds = cmds,
	.o_ncmds = ARRAY_SIZE(cmds),
};

static void init_netlink()
{
	int ret = genl_register_family(&ops);
	if (ret < 0)
		nl_cli_fatal(ret, "Unable to register Generic Netlink family");
}

static int send_netlink_cmd(int command, ino_t ino, char *pathname)
{
	struct nl_sock *sock;
	struct nl_msg *msg;
	void *hdr;
	int ret;

	sock = nl_cli_alloc_socket();
	nl_cli_connect(sock, NETLINK_GENERIC);

	if ((ret = genl_ops_resolve(sock, &ops)) < 0)
		nl_cli_fatal(ret, "Unable to resolve family name");

	if (genl_ctrl_resolve(sock, "nlctrl") != GENL_ID_CTRL)
		nl_cli_fatal(NLE_INVAL, "Resolving of \"nlctrl\" failed");

	msg = nlmsg_alloc();
	if (msg == NULL)
		nl_cli_fatal(NLE_NOMEM, "Unable to allocate netlink message");

	hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, ops.o_id, 0, 0, command, 0x01);
	if (hdr == NULL)
		nl_cli_fatal(ENOMEM, "Unable to write genl header");

	switch (command) {
		case GOBOHIDE_CMD_HIDE:
		case GOBOHIDE_CMD_UNHIDE:
			if ((ret = nla_put_u64(msg, GOBOHIDE_CMD_ATTR_INODE, ino)) < 0)
				nl_cli_fatal(ret, "Unable to add attribute: %s", nl_geterror(ret));
			if ((ret = nla_put_string(msg, GOBOHIDE_CMD_ATTR_PATH, pathname)) < 0)
				nl_cli_fatal(ret, "Unable to add attribute: %s", nl_geterror(ret));
			break;

		case GOBOHIDE_CMD_FLUSH:
		case GOBOHIDE_CMD_LIST:
			break;
	}

	if ((ret = nl_send_auto_complete(sock, msg)) < 0)
		nl_cli_fatal(ret, "Unable to send message: %s", nl_geterror(ret));

	nlmsg_free(msg);

	if (command == GOBOHIDE_CMD_LIST) {
		int msgs = 0;

		ret = nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, parse_cb, NULL);
		if (ret < 0)
			nl_cli_fatal(ret, "Unable to modify valid message callback");

		while ((ret = nl_recvmsgs_default(sock)) >= 0) {
			/* let the callback process the message */
			msgs++;
			/* check local counter against global */
			if (msgs > gobohide_list_size)
				break;
		}
	}

	nl_close(sock);
	nl_socket_free(sock);

	return 0;
}

void err_quit(int status, char *file)
{
	fprintf(stderr, _("%s is neither a directory "
			 "nor a symbolic link\n"), file);
	exit(status);
}

int cmd_hide_unhide(char *pathname, int operation)
{
	struct stat stats;
	int fd;

	if (strlen(pathname) > PATH_MAX-1) {
		fprintf(stderr, "Error: %s > PATH_MAX\n", pathname);
		exit(EXIT_FAILURE);
	}

	/* We're only interested in directories */
	fd = open(pathname, O_RDONLY|O_NOFOLLOW);
	if (fd < 0) { /* We're opening a symlink */
		fd = open(pathname, O_RDONLY); /* The symlink must point to a valid directory */
		if (fd < 0) {
			perror(pathname);
			exit(EXIT_FAILURE);
		}
		if (lstat(pathname, &stats) == -1) { /* Do not follow the link */
			perror("lstat");
			exit(EXIT_FAILURE);
		}
		if (!S_ISLNK(stats.st_mode))
			err_quit(1, pathname);
	} else {
		/* We opened a directory, let's get its inode number */
		if (fstat(fd, &stats) == -1) {
			perror("fstat");
			exit(EXIT_FAILURE);
		}
		if (!S_ISDIR(stats.st_mode))
			err_quit(1, pathname);
	}
	close (fd);

	return send_netlink_cmd(operation, stats.st_ino, pathname);
}

int cmd_flush()
{
	int ret;
	ret = send_netlink_cmd(GOBOHIDE_CMD_FLUSH, 0, NULL);
	return ret;
}

int cmd_list()
{
	int ret;
	ret = send_netlink_cmd(GOBOHIDE_CMD_LIST, 0, NULL);
	return ret;
}

void usage(char *program_name, int status)
{
	if (status) { /* Show help */
		fprintf(stdout, _(
		"%s: Hide/Unhide a directory\n\n"
		"-h, --hide     Hide the directory\n"
		"-u, --unhide   Unhide the directory\n"
		"-l, --list     List the hidden directories\n"
		"-f, --flush    Flush the hide list\n"
		"    --version  Show the program version\n"
		"    --help     Show this message\n"),
		program_name);
	} else {
		fprintf(stdout, _(
		"Copyright (C) 2002-2006 CScience.ORG World Domination Inc.\n\n"
		"This program is Free Software; you can redistributed it\n"
		"and/or modify it under the terms of the GNU General Public\n"
		"License as published by the Free Software Foundation.\n\n"
		"%s version %s\n"), program_name, version);
	}
	exit (0);
}

int main(int argc, char **argv)
{
	int c, ret = 0;
	int a = -1;
	const char *pathname = NULL;
	char *program_name = argv[0];

	int show_help = 0;
	int show_version = 0;
	const char shortopts[]  = "h:u:lf";
	struct option longopts[] = {
		{"hide",     1, 0, 'h'},
		{"unhide",   1, 0, 'u'},
		{"list",     0, 0, 'l'},
		{"flush",    0, 0, 'f'},
		{"help",     0, &show_help, 1},
		{"version",  0, &show_version, 1},
		{ 0, 0, 0, 0 }
	};


	while ((c = getopt_long(argc, argv, shortopts, longopts, 0)) != -1) {
		switch (c) {
		case 'h': a = GOBOHIDE_CMD_HIDE;
			  pathname = optarg;
			  break;
		case 'u': a = GOBOHIDE_CMD_UNHIDE;
			  pathname = optarg;
			  break;
		case 'l': a = GOBOHIDE_CMD_LIST;
			  break;
		case 'f': a = GOBOHIDE_CMD_FLUSH;
			  break;
		}
	}

	setlocale(LC_ALL, "");
	textdomain("gobohide");
	bindtextdomain("gobohide", LOCALEDIR);

	if (show_help)
		usage(program_name, 1);
	if (show_version)
		usage(program_name, 0);

	/* Only the superuser is allowed to execute further */
	if (getuid() != 0) {
		fprintf(stderr, _("Must be superuser\n"));
		exit(EXIT_SUCCESS);
	}

	init_netlink();

	switch (a) {
		case -1:
			fprintf(stderr, _("%s: You must specify at least one option!\n\n"
				"try '%s --help' for more information\n"), program_name, program_name);
			break;

		case GOBOHIDE_CMD_LIST:
			ret = cmd_list();
			break;

		case GOBOHIDE_CMD_FLUSH:
			ret = cmd_flush();
			break;
				  
		case GOBOHIDE_CMD_HIDE:
		case GOBOHIDE_CMD_UNHIDE:
			ret = cmd_hide_unhide((char *) pathname, a);
			while (ret == 0 && optind < argc)
				ret = cmd_hide_unhide(argv[optind++], a);
	}

	return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
