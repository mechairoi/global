/*
 * Copyright (c) 2012
 *	Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sys/types.h>
#if !defined(_WIN32) || defined(__CYGWIN__)
#include <sys/wait.h>
#endif
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#ifdef STDC_HEADERS
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "parser.h"

/*
 * Function layer plugin parser sample
 */

#define INITIAL_BUFSIZE		1024

static char *argv[] = {
	HASKTAGS,
	"-cx",
	"--ignore-close-implementation",
	"--output=-",
	NULL, /* file path */
	NULL
};
static pid_t pid;
static FILE *ip, *op;
static char *linebuf;
static size_t bufsize;
static char *hasktagsnotfound = "Hasktags not found. Please see ./configure --help.";

static void
start_hasktags(const struct parser_param *param)
{
	int opipe[2], ipipe[2];

	if (strlen(HASKTAGS) == 0 || !strcmp(HASKTAGS, "no"))
		param->die(hasktagsnotfound);

	if (pipe(opipe) < 0 || pipe(ipipe) < 0)
		param->die("cannot create pipe.");
	pid = fork();
	if (pid == 0) {
		/* child process */
		close(opipe[1]);
		close(ipipe[0]);
		if (dup2(opipe[0], STDIN_FILENO) < 0
		 || dup2(ipipe[1], STDOUT_FILENO) < 0)
			param->die("dup2 failed.");
		close(opipe[0]);
		close(ipipe[1]);
		argv[4] = param->file;
		execvp(HASKTAGS, argv);
		param->die("execvp failed.");
	}
	/* parent process */
	if (pid < 0)
		param->die("fork failed.");
	close(opipe[0]);
	close(ipipe[1]);
	ip = fdopen(ipipe[0], "r");
	op = fdopen(opipe[1], "w");
	if (ip == NULL || op == NULL)
		param->die("fdopen failed.");

	bufsize = INITIAL_BUFSIZE;
	linebuf = malloc(bufsize);
	if (linebuf == NULL)
		param->die("short of memory.");
}

/* #ifdef __GNUC__ */ // XXX
/* static void terminate_hasktags(void) __attribute__((destructor)); */
/* #endif */

static void
terminate_hasktags(void)
{
	if (op == NULL)
		return;
	free(linebuf);
	int ret;
	do {
		ret = fclose(op);
	} while ((ret == -1) && (errno = EINTR));
	do {
		ret = fclose(ip);
	} while ((ret == -1) && (errno = EINTR));
//	while (waitpid(pid, NULL, 0) < 0 && (errno = EINTR)) ;
}

static char *
get_line(const struct parser_param *param)
{
	size_t linelen = 0;

	for (;;) {
		if (fgets(linebuf + linelen, bufsize - linelen, ip) == NULL) {
			if (linelen == 0)
				return NULL;
			break;
		}
		linelen += strlen(linebuf + linelen);
		if (linelen < bufsize - 1 || linebuf[linelen - 1] == '\n'
		 || feof(ip))
			break;
		bufsize *= 2;
		linebuf = realloc(linebuf, bufsize);
		if (linebuf == NULL)
			param->die("short of memory.");
	}
	while (linelen-- > 0
		&& (linebuf[linelen] == '\n' || linebuf[linelen] == '\r'))
		linebuf[linelen] = '\0';
	return linebuf;
}

static void
put_line(char *hasktags_x, const struct parser_param *param)
{
	int lineno;
	char *p, *tagname, *filename;

	tagname = hasktags_x;

	p = hasktags_x + (strlen(hasktags_x) - 1);
	if (!isdigit((unsigned char)*p))
		return;
	while (p >= hasktags_x && isdigit((unsigned char)*p))
		p--;
	if (p < hasktags_x)
		return;
	lineno = atoi(p-- + 1);

	while (p >= hasktags_x && *p-- != '$');
	if (p < hasktags_x)
		return;
	*p-- = '\0';

	filename = strstr(hasktags_x, param->file);
	if (filename == NULL || filename == hasktags_x)
		return;
	p = filename - 1;
	if (!isspace((unsigned char)*p))
		return;
	while (p >= hasktags_x && isspace((unsigned char)*p))
		*p-- = '\0';
	if (p < hasktags_x)
		return;

	p = filename + strlen(param->file);
	while (isspace((unsigned char)*p))
		*p++ = '\0';
	if (*p++ != '/' && *p++ != '^')
		return;

	param->put(PARSER_DEF, tagname, lineno, param->file, p, param->arg);
}

void
parser(const struct parser_param *param)
{
	char *hasktags_x;

	assert(param->size >= sizeof(*param));

	start_hasktags(param);

	/* Read output of hasktags command. */
	get_line(param); get_line(param); get_line(param);
	for (;;) {
		hasktags_x = get_line(param);
		if (hasktags_x == NULL)
			break;
		put_line(hasktags_x, param);
	}

	terminate_hasktags();
}
