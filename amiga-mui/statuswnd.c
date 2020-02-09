/***************************************************************************
 SimpleMail - Copyright (C) 2000 Hynek Schlawack and Sebastian Bauer

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
***************************************************************************/

/*
** filterwnd.c
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libraries/iffparse.h> /* MAKE_ID */
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "atcleanup.h"
#include "codesets.h"
#include "configuration.h"
#include "debug.h"
#include "simplemail.h"
#include "smintl.h"
#include "support_indep.h"

#include "muistuff.h"
#include "statuswnd.h"
#include "subthreads.h"
#include "transwndclass.h"

struct MUI_NListtree_TreeNode *FindListtreeUserData(Object *tree, APTR udata);

static Object *status_wnd;

static char *status_title;
static int skip; /* skip current server */

/**************************************************************************
 Called when abort button presses
**************************************************************************/
static void statuswnd_abort(void)
{
	thread_abort(NULL);
}


/**************************************************************************
 Called when abort button presses
**************************************************************************/
static void statuswnd_skip(void)
{
	skip = 1;
	thread_abort(NULL);
}

/**
 * The clean up function of the status window.
 *
 * @param user_data
 */
void statuswnd_cleanup(void *user_data)
{
	free(status_title);
}

/**************************************************************************
 Open the status window
**************************************************************************/
int statuswnd_open(int active)
{
	if (!status_wnd)
	{
		status_wnd = transwndObject,
			MUIA_Window_ID,	MAKE_ID('S','T','A','T'),
			MUIA_Window_Title, status_title,
			End;

		if (status_wnd)
		{
			DoMethod(status_wnd, MUIM_Notify, MUIA_transwnd_Aborted, TRUE, (IPTR)status_wnd, 3, MUIM_CallHook, (IPTR)&hook_standard, (IPTR)statuswnd_abort);
			DoMethod(status_wnd, MUIM_Notify, MUIA_transwnd_Skipped, TRUE, (IPTR)status_wnd, 3, MUIM_CallHook, (IPTR)&hook_standard, (IPTR)statuswnd_skip);
			DoMethod(App, OM_ADDMEMBER, (IPTR)status_wnd);

			statuswnd_set_head(NULL);

			atcleanup(statuswnd_cleanup,NULL);
		}
	}

	if (status_wnd)
	{
		SetAttrs(status_wnd,
				MUIA_Window_Activate, active,
				MUIA_Window_Open, TRUE,
				TAG_DONE);
		return 1;
	}
	return 0;
}

/**************************************************************************
 Open the status window
**************************************************************************/
void statuswnd_close(void)
{
	if (status_wnd)
	{
		set(status_wnd, MUIA_Window_Open, FALSE);
	}
}

/**************************************************************************
 Display the status title
**************************************************************************/
void statuswnd_set_title(const char *title)
{
	free(status_title);
	status_title = mystrdup(title);

	if (!status_wnd) return;
	set(status_wnd, MUIA_Window_Title, status_title);
}

/**************************************************************************
 Display the status title (utf8)
**************************************************************************/
void statuswnd_set_title_utf8(const char *title)
{
	free(status_title);
	status_title = utf8tostrcreate(title, user.config.default_codeset);

	if (!status_wnd) return;
	set(status_wnd, MUIA_Window_Title, status_title);
}

/**************************************************************************
 Initialize the gauge bar with the given maximum value
**************************************************************************/
void statuswnd_init_gauge(int maximal)
{
	if (!status_wnd) return;
	set(status_wnd, MUIA_transwnd_Gauge1_Max, maximal);
}

/**************************************************************************
 Set the current gauge value
**************************************************************************/
void statuswnd_set_gauge(int value)
{
	if (!status_wnd) return;
	set(status_wnd, MUIA_transwnd_Gauge1_Val, value);
}

/**************************************************************************
 Set the current gauge text
**************************************************************************/
void statuswnd_set_gauge_text(char *text)
{
	set(status_wnd, MUIA_transwnd_Gauge1_Str, text);
}

/**************************************************************************
 Set the status line
**************************************************************************/
void statuswnd_set_status(char *text)
{
	if (!status_wnd) return;
	set(status_wnd, MUIA_transwnd_Status, text);
}

/**************************************************************************
 Set the head of the status window
**************************************************************************/
void statuswnd_set_head(char *text)
{
	static char *head_text;
	if (text != NULL) head_text = text;
	if (!status_wnd) return;
	set(status_wnd, MUIA_transwnd_Head, head_text);
}

/**************************************************************************
 Insert mail
**************************************************************************/
void statuswnd_mail_list_insert(int mno, int mflags, int msize)
{
	DoMethod(status_wnd, MUIM_transwnd_InsertMailSize, mno, mflags, msize);
}

/**************************************************************************
 Set flags
**************************************************************************/
void statuswnd_mail_list_set_flags(int mno, int mflags)
{
	DoMethod(status_wnd, MUIM_transwnd_SetMailFlags, mno, mflags);
}

/**************************************************************************
 Insert mail info
**************************************************************************/
void statuswnd_mail_list_set_info(int mno, char *from, char *subject, char *date)
{
	DoMethod(status_wnd, MUIM_transwnd_InsertMailInfo, mno, (IPTR)from, (IPTR)subject, (IPTR)date);
}

/**************************************************************************
 returns -1 if the mail is not in the mail selection
**************************************************************************/
int statuswnd_mail_list_get_flags(int mno)
{
	return (int)DoMethod(status_wnd, MUIM_transwnd_GetMailFlags, mno);
}

/**************************************************************************

**************************************************************************/
void statuswnd_mail_list_clear(void)
{
	DoMethod(status_wnd, MUIM_transwnd_Clear);
}

/**************************************************************************

**************************************************************************/
void statuswnd_mail_list_freeze(void)
{
	set(status_wnd, MUIA_transwnd_QuietList, TRUE);
}

/**************************************************************************

**************************************************************************/
void statuswnd_mail_list_thaw(void)
{
	set(status_wnd,MUIA_transwnd_QuietList,FALSE);
}

/**************************************************************************
 Wait for user interaction. Return -1 for aborting
**************************************************************************/
int statuswnd_wait(void)
{
	return (int)DoMethod(status_wnd, MUIM_transwnd_Wait);
}

/**************************************************************************
 Returns wheather the current server should be skipped
**************************************************************************/
int statuswnd_skipped(void)
{
	if (skip)
	{
		skip = 0;
		return 1;
	}
	return 0;
}

/***************************************************************************
 Returns 0 if user has aborted the statistic listing
**************************************************************************/
int statuswnd_more_statistics(void)
{
  LONG start_pressed = xget(status_wnd,MUIA_transwnd_StartPressed);
  return !start_pressed;
}

/***************************************************************************
 Returns 1 if the statuswnd is opend
**************************************************************************/
int statuswnd_is_opened(void)
{
	if (status_wnd) return (int)xget(status_wnd,MUIA_Window_Open);
	return 0;
}

