/*
** wifimgrsu.c
**	execute command as super-user on behalf of wifimgr(8)
*/

/*
** Copyright (c) 2009, J.R. Oldroyd, Open Advisors Limited
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the author, the author's organization nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
** 
** THIS SOFTWARE IS PROVIDED BY OPEN ADVISORS LIMITED ''AS IS'' AND ANY
** EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL OPEN ADVISORS LIMITED BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
** $Id: wifimgrsu.c 102 2014-05-10 20:53:58Z jr $
*/

#include "wifimgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>

#ifndef WITHOUT_NLS
#include <libintl.h>
#else
#define gettext(x)	(x)
#endif

#define	PATH		"/bin:/usr/bin:/sbin:/usr/sbin"

int
main(int argc, char ** argv) {
	char			line[1024];
	char			cmd[1024];
	int			auth = 0;
	int			copy_stdout;
	char *			arg;
	int			xst;

#ifndef WITHOUT_NLS
	/* set output character set */
	bind_textdomain_codeset("wifimgr", "UTF-8");

	/* set language catalog domain */
	textdomain("wifimgr");
#endif

	if (isatty(0) || isatty(1)) {
		fprintf(stderr, gettext("wifimgrsu: for invocation by wifimgr(8) only\n"));
		exit(1);
	}

	/* set line buffering */
	setvbuf(stdin, (char *) NULL, _IOLBF, 0);
	setvbuf(stdout, (char *) NULL, _IOLBF, 0);

	/* set safe PATH (required for shell scripts such as /etc/rc.d/netif) */
	setenv("PATH", PATH, 1);

	/* loop reading commands */
	while (fgets(line, sizeof(line), stdin) != NULL) {
		chop(line);
		if (strncmp(line, "password ", 9) == 0) {
			char *			pass;
			char *			su_pass;

			pass = strdup(index(line, ' ') + 1);
			/* use getpwuid(0) here rather than getpwnam("root") in case "toor" or
			** other super-user name is being used */
			su_pass = strdup(getpwuid(0)->pw_passwd);
			if (strcmp(crypt(pass, su_pass), su_pass) != 0)
				printf("FAIL %s\n", gettext("invalid password"));
			else {
				auth = 1;
				printf("OK\n");
			}
			continue;
		}

		/* go no further unless authorization passed */
		if (!auth) {
			printf("FAIL %s\n", gettext("not authorized"));
			continue;
		}

		/* flag for commands that produce stdout */
		copy_stdout = 0;

		if (strcmp(line, "cat_netfile") == 0) {
			if (eaccess(NETWORKS_FILE, F_OK) == 0)
				sprintf(cmd, "%s %s", PATH_CAT, NETWORKS_FILE);
			else
				sprintf(cmd, "");
			copy_stdout = 1;
		}
		else if (strncmp(line, "write_netfile ", 14) == 0) {
			arg = index(line, ' ') + 1;
			sprintf(cmd, "%s %s %s", PATH_CP, arg, NETWORKS_FILE);
		}
		else if (strcmp(line, "diff_netfile") == 0)
			sprintf(cmd, "%s -q %s %s.save >/dev/null", PATH_DIFF, NETWORKS_FILE, NETWORKS_FILE);
		else if (strcmp(line, "backup_netfile") == 0)
			sprintf(cmd, "%s -p %s %s.save", PATH_CP, NETWORKS_FILE, NETWORKS_FILE);
		else if (strncmp(line, "restart_netif ", 14) == 0) {
			arg = index(line, ' ') + 1;
			sprintf(cmd, "%s restart %s >/dev/null 2>&1", PATH_NETIF, arg);
		}
		else if (strncmp(line, "start_netif ", 12) == 0) {
			arg = index(line, ' ') + 1;
			sprintf(cmd, "%s start %s >/dev/null 2>&1", PATH_NETIF, arg);
		}
		else if (strncmp(line, "stop_netif ", 11) == 0) {
			arg = index(line, ' ') + 1;
			sprintf(cmd, "%s stop %s >/dev/null 2>&1", PATH_NETIF, arg);
		}
		else {
			printf("FAIL %s\n", gettext("invalid command"));
			continue;
		}

		xst = system(cmd);

		if (copy_stdout)
			printf("EOF\n");

		if (WEXITSTATUS(xst) == 0)
			printf("OK\n");
		else
			printf("FAIL %s %d\n", gettext("exit status"), WEXITSTATUS(xst));
	}
}
