/*
 * Copyright (c) 1997, 1998, 1999 Shigio Yamaguchi
 * Copyright (c) 1999, 2000, 2001 Tama Communications Corporation
 *
 * This file is part of GNU GLOBAL.
 *
 * GNU GLOBAL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * GNU GLOBAL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _DBOP_H_
#define _DBOP_H_

#include "gparam.h"
#include "db.h"
#include "regex.h"
#ifdef USE_POSTGRES
#include "libpq-fe.h"
#endif
#include "strbuf.h"

#define DBOP_PAGESIZE	8192

typedef	struct {
	/*
	 * (1) COMMON PART
	 */
	int mode;			/* 0:read, 1:create, 2:modify */
	int openflags;			/* flags of xxxx_open() */
	int ioflags;			/* flags of xxxx_first() */
	char *lastdat;			/* the data of last located record */
	regex_t	*preg;			/* compiled regular expression */
	/*
	 * (2) DB185 PART
	 */
	DB *db;				/* descripter of DB */
	char dbname[MAXPATHLEN+1];	/* dbname */
	char key[MAXKEYLEN+1];		/* key */
	int keylen;			/* key length */
	char prev[MAXKEYLEN+1];		/* previous key value */
	int perm;			/* file permission */
#ifdef USE_POSTGRES
	/*
	 * (3) POSTGRES PART
	 */
	char tblname[80];		/* table name */
	PGresult *res;			/* result structure */
	/* SQL statement templete */
	STRBUF *get_stmt;		/* pgop_get() */
	int get_stmt_len;
	STRBUF *getkey_stmt;		/* pgop_getkey_by_fid() */
	int getkey_stmt_len;
	STRBUF *put_stmt;		/* pgop_put() */
	int put_stmt_len;
	STRBUF *fetch_stmt;		/* pgop_first(), pgop_next() */
	int fetch_stmt_len;
	STRBUF *delete_stmt;		/* pgop_delete() */
	int delete_stmt_len;
#endif /* POSTGRES */
} DBOP;

/*
 * openflags
 */
#define	DBOP_DUP	1		/* allow duplicate records	*/
#define DBOP_REMOVE	2		/* remove file when closed	*/
#define DBOP_POSTGRES	4		/* use postgres database	*/
/*
 * ioflags
 */
#define DBOP_KEY	1		/* read key part		*/
#define DBOP_PREFIX	2		/* prefixed read		*/

void dbop_setinfo(char *info);
DBOP *dbop_open(const char *, int, int, int);
char *dbop_get(DBOP *, const char *);
void dbop_put(DBOP *, const char *, const char *, const char *);
void dbop_delete(DBOP *, const char *);
char *dbop_getkey_by_fid(DBOP *, const char *);
void dbop_delete_by_fid(DBOP *, const char *);
void dbop_update(DBOP *, const char *, const char *, const char *);
char *dbop_first(DBOP *, const char *, regex_t *, int);
char *dbop_next(DBOP *);
char *dbop_lastdat(DBOP *);
void dbop_close(DBOP *);

#endif /* _DBOP_H_ */
