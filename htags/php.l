%{
/*
 * Copyright (c) 2002, 2004 Tama Communications Corporation
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

/*
 * scanner for php source code.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "global.h"
#include "anchor.h"
#include "common.h"
#include "incop.h"
#include "htags.h"
#include "path2url.h"
#include "../gctags/php_res.h"
#include "lexcommon.h"

#define LEXTEXT	phptext
#define LEXLENG	phpleng

#ifdef ECHO
#undef ECHO
#endif
#define ECHO	echos(LEXTEXT)

#define YY_USER_ACTION DEFAULT_YY_USER_ACTION

%}
 /* Definitions */
H		0[Xx][0-9A-Fa-f]+
N		[0-9]+
L		{N}L?
D1		{N}\.{N}([Ee][+-]?{N})?
D2		\.{N}([Ee][+-]?{N})?
NUMBER		-?({L}|{D1}|{D2})
ALPHA		[a-zA-Z_\x80-\xff]
ALPHANUM	[a-zA-Z_\x80-\xff0-9]
WORD		{ALPHA}{ALPHANUM}*

%start	PHP C_COMMENT CPP_COMMENT SHELL_COMMENT STRING LITERAL
%option 8bit noyywrap stack prefix="php"
%%
 /* Start PHP */
<INITIAL>"<?="		{ put_string(LEXTEXT); BEGIN PHP; }
<INITIAL>"<?"		{ put_string(LEXTEXT); BEGIN PHP; }
<INITIAL>"<?php"	{ put_string(LEXTEXT); BEGIN PHP; }
<INITIAL>"<%"		{ put_string(LEXTEXT); BEGIN PHP; }
<INITIAL>"<script[ \t]+language=(\")?php(\")?>" { put_string(LEXTEXT); BEGIN PHP; }
 /* Ignore HTML */
<INITIAL>.              ECHO;
 /* End of PHP */
<PHP>"?>"		{ put_string(LEXTEXT); BEGIN INITIAL; }
<PHP>"%>"               { put_string(LEXTEXT); BEGIN INITIAL; }
<PHP>"</script>"        { put_string(LEXTEXT); BEGIN INITIAL; }

 /* Comment */
<PHP>"/*"	{ echos(comment_begin); ECHO; yy_push_state(C_COMMENT); }
<C_COMMENT>"*/"	{ ECHO; echos(comment_end); yy_pop_state(); }
<C_COMMENT>.	{ put_char(LEXTEXT[0]); }
<PHP>"#"        { echos(comment_begin); ECHO; yy_push_state(SHELL_COMMENT); }
<PHP>"//"	{ echos(comment_begin); ECHO; yy_push_state(CPP_COMMENT); }

 /* String */
<PHP>\"		{ ECHO; yy_push_state(STRING); }
<STRING>\"	{ ECHO; yy_pop_state(); }
<STRING>\\.	{ put_char(LEXTEXT[0]); put_char(LEXTEXT[1]); }

 /* Literal */
<PHP>\'		{ ECHO; yy_push_state(LITERAL); }
<LITERAL>\'	{ ECHO; yy_pop_state(); }
<LITERAL>\\.	{ put_char(LEXTEXT[0]); put_char(LEXTEXT[1]); }

<PHP>^[ \t]*include[ \t]\( {
		int c;

		ECHO;
		/*
		 * include|('aaa/bbb.h');
		 *          ^
		 */
		while ((c = input()) && c != '\n' && isspace(c))
			echoc(c);
		if (c == '\n')
			unput(c);
		else if (c) {
			char path[MAXPATHLEN+1], *p = path;
			int sep = 0;

			if (c == '"' || c == '\'')
				sep = c;
			echoc(c);

			/* pick up path name */
			while ((c = input()) && c != '\n' && c != sep)
				*p++ = c;
			*p = '\0';
			if (c == sep) {
				struct data *inc;
				char *basename = locatestring(path, "/", MATCH_LAST);

				if (basename)
					basename++;
				else
					basename = path;
				inc = get_inc(basename);
				if (inc)
					put_include_anchor(inc, path);
				else
					echos(path);
				echoc(sep);
			} else {
				echos(path);
				if (c)
					unput(c);
			}
		}
	}
<PHP>{NUMBER}	ECHO;
<PHP,STRING>\${WORD}	 |
<PHP,STRING>\$\{{WORD}\} {
		struct anchor *a = NULL;
		char *p = LEXTEXT + 1;	/* skip '$' */
		int brace = 0, i = 0;

		/*
		 * extract name.
		 */
		if (*p == '{') {
			char buf[IDENTLEN];

			brace = 1;
			for (p++; *p && *p != '}'; p++) {
				buf[i++] = *p;
				if (i >= sizeof(buf))
					die("Too long name '%s'.", LEXTEXT);
			}
			buf[i] = '\0';
			p = buf;
		} else {
			i = LEXLENG - 1;
		}
		if (symbol && (a = anchor_get(p, i, 'Y', LINENO)) != NULL) {
			echoc('$');
			if (brace)
				echoc('{');
			put_anchor(gettag(a), a->type, LINENO);
			if (brace)
				echoc('}');
			a->done = 1;
		} else {
			ECHO;
		}
	}
<PHP>{WORD} {
		struct anchor *a = NULL;
		if (reserved_word(LEXTEXT, LEXLENG))
			put_reserved_word(LEXTEXT);
		else {
			a = anchor_get(LEXTEXT, LEXLENG, 0, LINENO);
			if (a) {
				put_anchor(gettag(a), a->type, LINENO);
				a->done = 1;
			} else {
				ECHO;
			}
		}
	}
<PHP>[{}]	{ put_brace(LEXTEXT); }
 /* New line */
\n		DEFAULT_END_OF_LINE_ACTION
.		{ put_char(LEXTEXT[0]); }

%%
void
php_parser_init(ip)
	FILE *ip;
{
	DEFAULT_BEGIN_OF_FILE_ACTION
}