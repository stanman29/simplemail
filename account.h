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
** account.h
*/


#ifndef SM__ACCOUNT_H
#define SM__ACCOUNT_H

#ifndef SM__LISTS_H
#include "lists.h"
#endif

#ifndef SM__SMTP_H
#include "smtp.h"
#endif

#ifndef SM__POP3_H
#include "pop3.h"
#endif

#ifndef SM__IMAP_H
#include "imap.h"
#endif

struct folder;
struct imap_server;

/**
 * Represents a full email account.
 */
struct account
{
	/** Embedded node structure */
	struct node node;

	/** The name of the account that is presented to the user */
	char *account_name;

	/** The name of the user */
	char *name;

	/** The users eMail adress */
	char *email;

	/** The reply to adresse for this account */
	char *reply;

	/**
	 * Name of the default signature to use.
	 *
	 * @see signature.c
	 */
	char *def_signature;

	/** Which kind of receiving server? 0 POP3, 1 IMAP4 */
	int recv_type;

	/** POP3 server settings */
	struct pop3_server *pop;

	/** IMAP server settings */
	struct imap_server *imap;

	/** SMTP server settings */
	struct smtp_server *smtp;
};

struct account *account_malloc(void);
struct account *account_duplicate(struct account *a);
void account_free(struct account *a);
struct account *account_find_by_from(char *email);
struct imap_server *account_find_imap_server_by_folder(struct folder *f);

#define account_find_by_number(number) \
	((struct account*)list_find(&user.config.account_list,number))

#endif
