/*
 * Copyright (c) 2014 Fujitsu Ltd.
 * Author: Xiaoguang Wang <wangxg.fnst@cn.fujitsu.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "test.h"
#include "usctest.h"
#include "safe_macros.h"

#define MAX_SANE_HARD_LINKS	65535

int tst_fs_fill_hardlinks(void (*cleanup) (void), const char *dir)
{
	unsigned int i, j;
	char base_filename[PATH_MAX], link_filename[PATH_MAX];
	struct stat s;

	if (stat(dir, &s) == -1 && errno == ENOENT)
		SAFE_MKDIR(cleanup, dir, 0744);

	SAFE_STAT(cleanup, dir, &s);
	if (!S_ISDIR(s.st_mode))
		tst_brkm(TBROK, cleanup, "%s is not directory", dir);

	sprintf(base_filename, "%s/testfile0", dir);
	SAFE_TOUCH(cleanup, base_filename, 0644, NULL);

	for (i = 1; i < MAX_SANE_HARD_LINKS; i++) {
		sprintf(link_filename, "%s/testfile%d", dir, i);

		if (link(base_filename, link_filename) == 0)
			continue;

		switch (errno) {
		case EMLINK:
			SAFE_STAT(cleanup, base_filename, &s);
			if (s.st_nlink != i) {
				tst_brkm(TBROK, cleanup, "wrong number of "
					 "hard links for %s have %i, should be"
					 " %d", base_filename,
					 (int)s.st_nlink, i);
			} else {
				tst_resm(TINFO, "the maximum number of hard "
					 "links to %s is hit: %d",
					 base_filename, (int)s.st_nlink);
				return s.st_nlink;
			}
		case ENOSPC:
		case EDQUOT:
			tst_resm(TINFO | TERRNO, "link(%s, %s) failed",
				 base_filename, link_filename);
			goto max_hardlinks_cleanup;
		default:
			tst_brkm(TBROK, cleanup, "link(%s, %s) failed "
				 "unexpectedly: %s", base_filename,
				 link_filename, strerror(errno));
		}
	}

	tst_resm(TINFO, "Failed reach the hardlinks limit");

max_hardlinks_cleanup:
	for (j = 0; j < i; j++) {
		sprintf(link_filename, "%s/testfile%d", dir, j);
		SAFE_UNLINK(cleanup, link_filename);
	}

	return 0;
}

int tst_fs_fill_subdirs(void (*cleanup) (void), const char *dir)
{
	unsigned int i, j;
	char dirname[PATH_MAX];
	struct stat s;

	if (stat(dir, &s) == -1 && errno == ENOENT)
		SAFE_MKDIR(cleanup, dir, 0744);

	SAFE_STAT(cleanup, dir, &s);
	if (!S_ISDIR(s.st_mode))
		tst_brkm(TBROK, cleanup, "%s is not directory", dir);

	for (i = 0; i < MAX_SANE_HARD_LINKS; i++) {
		sprintf(dirname, "%s/testdir%d", dir, i);

		if (mkdir(dirname, 0755) == 0)
			continue;

		switch (errno) {
		case EMLINK:
			SAFE_STAT(cleanup, dir, &s);
			/*
			 * i+2 because there are two links to each newly
			 * created directory (the '.' and link from parent dir)
			 */
			if (s.st_nlink != (i + 2)) {
				tst_brkm(TBROK, cleanup, "%s link counts have"
					 "%d, should be %d", dir,
					 (int)s.st_nlink, i + 2);
			} else {
				tst_resm(TINFO, "the maximum subdirectories in "
				 "%s is hit: %d", dir, (int)s.st_nlink);
				return s.st_nlink;
			}
		case ENOSPC:
		case EDQUOT:
			tst_resm(TINFO | TERRNO, "mkdir(%s, 0755) failed",
			         dirname);
			goto max_subdirs_cleanup;
		default:
			tst_brkm(TBROK, cleanup, "mkdir(%s, 0755) failed "
				 "unexpectedly: %s", dirname,
				 strerror(errno));
		}

	}

	tst_resm(TINFO, "Failed reach the subdirs limit");

max_subdirs_cleanup:
	for (j = 0; j < i; j++) {
		sprintf(dirname, "%s/testdir%d", dir, j);
		SAFE_RMDIR(cleanup, dirname);
	}

	return 0;
}
