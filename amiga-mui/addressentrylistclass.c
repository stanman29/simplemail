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
** addressentrylistclass.c
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libraries/iffparse.h>
#include <libraries/mui.h>
#include <mui/NListview_MCC.h>

#include <clib/alib_protos.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>

#include "addressbook.h"
#include "codesets.h"
#include "configuration.h"
#include "debug.h"
#include "simplemail.h"
#include "smintl.h"
#include "support.h"
#include "support_indep.h"

#include "addressentrylistclass.h"
#include "compiler.h"
#include "mailtreelistclass.h"
#include "muistuff.h"

struct AddressEntryList_Data
{
	struct Hook construct_hook;
	struct Hook compare_hook;
	struct Hook destruct_hook;
	struct Hook display_hook;

	ULONG type;

	char alias_buf[64];
	char realname_buf[64];
	char description_buf[128];
	char email_buf[128];

	char *pattern;

	/* Menu */
	Object *title_menu;
	Object *show_realname_item;
	Object *show_nickname_item;
	Object *show_description_item;
	Object *show_email_item;
};

/********************************************
 Constructor for addressgroup entries
*********************************************/
STATIC ASM SAVEDS struct addressbook_entry_new *addressentry_construct(REG(a0, struct Hook *h), REG(a2, Object *obj),REG(a1,struct NList_ConstructMessage *msg))
{
	struct addressbook_entry_new *entry = (struct addressbook_entry_new *)msg->entry;
	return addressbook_duplicate_entry_new(entry);
}

/********************************************
 Destructor for addressentry entries
*********************************************/
STATIC ASM SAVEDS VOID addressentry_destruct(REG(a0, struct Hook *h), REG(a2, Object *obj),REG(a1,struct NList_DestructMessage *msg))
{
	struct addressbook_entry_new *entry = (struct addressbook_entry_new *)msg->entry;
	addressbook_free_entry_new(entry);
}

/********************************************
 Dislayfunction function for addressentrys
*********************************************/
STATIC ASM SAVEDS VOID addressentry_display(REG(a0,struct Hook *h),REG(a2,Object *obj), REG(a1,struct NList_DisplayMessage *msg))
{
	char **array = msg->strings;
	char **preparse = msg->preparses;
	struct addressbook_entry_new *entry = (struct addressbook_entry_new*)msg->entry;
	struct AddressEntryList_Data *data = (struct AddressEntryList_Data*)h->h_Data;

	if (entry)
	{
		utf8tostr(entry->alias, data->alias_buf, sizeof(data->alias_buf), user.config.default_codeset);
		utf8tostr(entry->realname, data->realname_buf, sizeof(data->realname_buf), user.config.default_codeset);
		utf8tostr(entry->description, data->description_buf, sizeof(data->description_buf), user.config.default_codeset);

		*array++ = data->realname_buf;
		*array++ = data->alias_buf;
		*array++ = data->description_buf;

		if (entry->email_array) *array = entry->email_array[0];
		else *array = NULL;

		if (data->pattern)
		{
			int pl = strlen(data->pattern);
			int i;

			if (!utf8stricmp_len(entry->alias, data->pattern, pl)) preparse[1] = "\033b";
			if (!utf8stricmp_len(entry->realname, data->pattern, pl)) preparse[0] = "\033b";

			for (i=0;i<array_length(entry->email_array);i++)
			{
				if (!utf8stricmp_len(entry->email_array[i],data->pattern, pl))
				{
					/* Check if this email is displayed, if not append it */
					if (i != 0)
					{
						sm_snprintf(data->email_buf, sizeof(data->email_buf), "%s,\033b%s", entry->email_array[0], entry->email_array[1]);
						*array = data->email_buf;
					} else preparse[3] = "\033b";
					break;
				}
			}

		}
	} else
	{
		*array++ = Q_("?people:Name");
		*array++ = _("Alias");
		*array++ = _("Description");
		*array = _("Address");
	}
}

/********************************************
 Comparefunction for addressentrys
*********************************************/
STATIC ASM SAVEDS LONG addressentry_compare(REG(a0, struct Hook *h), REG(a2, Object *obj), REG(a1,struct NList_CompareMessage *msg))
{
	struct addressbook_entry_new *entry1 = (struct addressbook_entry_new *)msg->entry1;
	struct addressbook_entry_new *entry2 = (struct addressbook_entry_new *)msg->entry2;

	return utf8stricmp(entry1->realname,entry2->realname);
}

/******************************
 Update the format of the list
*******************************/
STATIC VOID AddressEntryList_UpdateFormat(struct IClass *cl,Object *obj)
{
	struct AddressEntryList_Data *data = (struct AddressEntryList_Data*)INST_DATA(cl,obj);
	char buf[256];

	buf[0] = 0;

	if (xget(data->show_realname_item,MUIA_Menuitem_Checked)) strcat(buf,"COL=0,");
	if (xget(data->show_nickname_item,MUIA_Menuitem_Checked)) strcat(buf,"COL=1,");
	if (xget(data->show_description_item,MUIA_Menuitem_Checked)) strcat(buf,"COL=2,");
	if (xget(data->show_email_item,MUIA_Menuitem_Checked)) strcat(buf,"COL=3,");

	if (strlen(buf)) buf[strlen(buf)-1] = 0;

	set(obj, MUIA_NList_Format, buf);
}


/********************** BOOPSI Methods **********************/

/********************************************
 OM_NEW
*********************************************/
STATIC ULONG AddressEntryList_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct AddressEntryList_Data *data;

	if (!(obj=(Object *)DoSuperNew(cl,obj,
					MUIA_Draggable, TRUE,
					TAG_MORE,msg->ops_AttrList)))
		return 0;

	data = (struct AddressEntryList_Data*)INST_DATA(cl,obj);

  data->type = GetTagData(MUIA_AddressEntryList_Type,MUIV_AddressEntryList_Type_Addressbook,msg->ops_AttrList);

	init_hook(&data->construct_hook,(HOOKFUNC)addressentry_construct);
	init_hook(&data->compare_hook,(HOOKFUNC)addressentry_compare);
	init_hook(&data->destruct_hook,(HOOKFUNC)addressentry_destruct);
	init_hook_with_data(&data->display_hook,(HOOKFUNC)addressentry_display,data);

	SetAttrs(obj,
						MUIA_NList_ConstructHook2, &data->construct_hook,
						MUIA_NList_CompareHook2, &data->compare_hook,
						MUIA_NList_DestructHook2, &data->destruct_hook,
						MUIA_NList_DisplayHook2, &data->display_hook,
						MUIA_NList_Title, TRUE,
						MUIA_NList_Format, ",,,",
						TAG_DONE);

	if (data->type == MUIV_AddressEntryList_Type_Main)
	{
		data->title_menu = MenustripObject,
			Child, MenuObjectT(_("Addresslist Settings")),
				Child, data->show_realname_item = MenuitemObject, MUIA_ObjectID, MAKE_ID('A','L','S','R'),MUIA_Menuitem_Title, _("Show Realname?"), MUIA_UserData, 1, MUIA_Menuitem_Checked, TRUE, MUIA_Menuitem_Checkit, TRUE, MUIA_Menuitem_Toggle, TRUE, End,
				Child, data->show_nickname_item = MenuitemObject, MUIA_ObjectID, MAKE_ID('A','L','S','N'),MUIA_Menuitem_Title, _("Show Nickname?"), MUIA_UserData, 2, MUIA_Menuitem_Checked, TRUE, MUIA_Menuitem_Checkit, TRUE, MUIA_Menuitem_Toggle, TRUE, End,
				Child, data->show_description_item = MenuitemObject, MUIA_ObjectID, MAKE_ID('A','L','S','D'),MUIA_Menuitem_Title, _("Show Description?"), MUIA_UserData, 3, MUIA_Menuitem_Checked, TRUE, MUIA_Menuitem_Checkit, TRUE, MUIA_Menuitem_Toggle, TRUE, End,
				Child, data->show_email_item = MenuitemObject, MUIA_ObjectID, MAKE_ID('A','L','S','A'),MUIA_Menuitem_Title, _("Show E-Mail Address?"), MUIA_UserData, 4, MUIA_Menuitem_Checked, TRUE, MUIA_Menuitem_Checkit, TRUE, MUIA_Menuitem_Toggle, TRUE, End,
				Child, MenuitemObject, MUIA_Menuitem_Title, -1, End,
				Child, MenuitemObject, MUIA_Menuitem_Title, _("Default Width: this"), MUIA_UserData, MUIV_NList_Menu_DefWidth_This, End,
				Child, MenuitemObject, MUIA_Menuitem_Title, _("Default Width: all"), MUIA_UserData, MUIV_NList_Menu_DefWidth_All, End,
				Child, MenuitemObject, MUIA_Menuitem_Title, _("Default Order: this"), MUIA_UserData, MUIV_NList_Menu_DefOrder_This, End,
				Child, MenuitemObject, MUIA_Menuitem_Title, _("Default Order: all"), MUIA_UserData, MUIV_NList_Menu_DefOrder_All, End,
				End,
			End;

			AddressEntryList_UpdateFormat(cl,obj);
	}

	return (ULONG)obj;
}

/********************************************
 OM_DISPOSE
*********************************************/
STATIC ULONG AddressEntryList_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
	struct AddressEntryList_Data *data = (struct AddressEntryList_Data*)INST_DATA(cl,obj);
	free(data->pattern);
	return DoSuperMethodA(cl,obj,msg);
}

/********************************************
 MUIM_AddressEntryList_Refresh
*********************************************/
STATIC ULONG AddressEntryList_Refresh(struct IClass *cl, Object *obj, struct MUIP_AddressEntryList_Refresh *msg)
{
	struct AddressEntryList_Data *data = (struct AddressEntryList_Data*)INST_DATA(cl,obj);
	struct addressbook_entry_new *entry;
	char *pattern;

	if (data->type == MUIV_AddressEntryList_Type_Match)
		pattern = msg->pattern;
	else pattern = NULL;

	free(data->pattern);
	data->pattern = mystrdup(pattern);

	set(obj, MUIA_NList_Quiet, TRUE);
	DoMethod(obj, MUIM_NList_Clear);

	entry = addressbook_first_entry();
	while (entry)
	{
		if (!msg->pattern || (msg->pattern && addressbook_get_entry_completing_part(entry,msg->pattern,NULL)))
			DoMethod(obj, MUIM_NList_InsertSingle, entry, MUIV_NList_Insert_Sorted);

		entry = addressbook_next_entry(entry);
	}
	set(obj, MUIA_NList_Quiet, FALSE);

	return 0;
}

/********************************************
 MUIM_ContextMenuChoice
*********************************************/
STATIC ULONG AddressEntryList_ContextMenuChoice(struct IClass *cl, Object *obj, struct MUIP_ContextMenuChoice *msg)
{
	switch (xget(msg->item,MUIA_UserData))
	{
		case	1:
		case	2:
		case	3:
		case	4:
					AddressEntryList_UpdateFormat(cl,obj);
					break;

		default: 
					return DoSuperMethodA(cl,obj,(Msg)msg);
	}
  return 0;
}

/********************************************
 MUIM_NList_ContextMenuBuild
*********************************************/
STATIC ULONG AddressEntryList_ContextMenuBuild(struct IClass *cl, Object * obj, struct MUIP_NList_ContextMenuBuild *msg)
{
	struct AddressEntryList_Data *data = (struct AddressEntryList_Data*)INST_DATA(cl,obj);

	if (data->type != MUIV_AddressEntryList_Type_Main) return DoSuperMethodA(cl,obj,(Msg)msg);

	if (msg->ontop) return (ULONG)data->title_menu;
	return 0;
}

/********************************************
 MUIM_DragQuery
*********************************************/
STATIC ULONG AddressEntryList_DragQuery(struct IClass *cl, Object *obj, struct MUIP_DragQuery *msg)
{
/*	struct AddressTreelist_Data *data = (struct AddressTreelist_Data*)INST_DATA(cl,obj);*/

	if (OCLASS(msg->obj) == CL_MailTreelist->mcc_Class) return MUIV_DragQuery_Accept;
	return MUIV_DragQuery_Refuse;
}

/********************************************
 MUIM_DragDrop
*********************************************/
STATIC ULONG AddressEntryList_DragDrop(struct IClass *cl, Object *obj, struct MUIP_DragDrop *msg)
{
	if (OCLASS(msg->obj) != CL_MailTreelist->mcc_Class) return DoSuperMethodA(cl,obj,(Msg)msg);
	callback_get_address();
	return 0;
}

/********************************************
 Boopsi Dispatcher
*********************************************/
STATIC BOOPSI_DISPATCHER(ULONG,AddressEntryList_Dispatcher,cl,obj,msg)
{
	switch(msg->MethodID)
	{
		case	OM_NEW: return AddressEntryList_New(cl,obj,(struct opSet*)msg);
		case	OM_DISPOSE: return AddressEntryList_Dispose(cl,obj,msg);
		case	MUIM_ContextMenuChoice: return AddressEntryList_ContextMenuChoice(cl, obj, (struct MUIP_ContextMenuChoice *)msg);
		case	MUIM_NList_ContextMenuBuild: return AddressEntryList_ContextMenuBuild(cl,obj,(struct MUIP_NList_ContextMenuBuild *)msg);
		case	MUIM_DragQuery: return AddressEntryList_DragQuery(cl,obj,(struct MUIP_DragQuery *)msg);
		case	MUIM_DragDrop:  return AddressEntryList_DragDrop(cl,obj,(struct MUIP_DragDrop *)msg);
		case	MUIM_AddressEntryList_Refresh: return AddressEntryList_Refresh(cl,obj,(struct MUIP_AddressEntryList_Refresh *)msg);
		default: return DoSuperMethodA(cl,obj,msg);
	}
}

struct MUI_CustomClass *CL_AddressEntryList;

int create_addressentrylist_class(void)
{
	SM_ENTER;
	if ((CL_AddressEntryList = CreateMCC(MUIC_NList,NULL,sizeof(struct AddressEntryList_Data),AddressEntryList_Dispatcher)))
	{
		SM_DEBUGF(15,("Create CL_AddressEntryList: 0x%lx\n",CL_AddressEntryList));
		SM_RETURN(1,"%ld");
	}
	SM_DEBUGF(5,("FAILED! Create CL_AddressEntryList\n"));
	SM_RETURN(0,"%ld");
}

void delete_addressentrylist_class(void)
{
	SM_ENTER;
	if (CL_AddressEntryList)
	{
		if (MUI_DeleteCustomClass(CL_AddressEntryList))
		{
			SM_DEBUGF(15,("Deleted CL_AddressEntryList: 0x%lx\n",CL_AddressEntryList));
			CL_AddressEntryList = NULL;
		} else
		{
			SM_DEBUGF(5,("FAILED! Delete CL_AddressEntryList: 0x%lx\n",CL_AddressEntryList));
		}
	}
	SM_LEAVE;
}
