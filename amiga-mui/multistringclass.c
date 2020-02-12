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
** multistringclass.c
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <intuition/intuitionbase.h>
#include <libraries/mui.h>
#include <mui/BetterString_mcc.h>
#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "lists.h"
#include "support_indep.h"
#include "debug.h"

#include "amigasupport.h"
#include "configuration.h"
#include "compiler.h"
#include "muistuff.h"
#include "multistringclass.h"
#include "utf8stringclass.h"

/* This class is used privatly */
struct SingleString_Data
{
  struct MUI_EventHandlerNode ehnode;
  int event;
};


STATIC ULONG SingleString_Set(struct IClass *cl,Object *obj, struct opSet *msg)
{
	struct SingleString_Data *data = (struct SingleString_Data*)INST_DATA(cl,obj);
	struct TagItem *ti = FindTagItem(MUIA_SingleString_Event,msg->ops_AttrList);
	if (ti)
		data->event = ti->ti_Data;

	return DoSuperMethodA(cl,obj,(Msg)msg);
}

STATIC ULONG SingleString_Get(struct IClass *cl,Object *obj, struct opGet *msg)
{
	struct SingleString_Data *data = (struct SingleString_Data*)INST_DATA(cl,obj);
	if (msg->opg_AttrID == MUIA_SingleString_Event)
	{
		*msg->opg_Storage = data->event;
	}
	return DoSuperMethodA(cl,obj,(Msg)msg);
}

STATIC ULONG SingleString_Setup(struct IClass *cl, Object *obj,struct MUIP_Setup *msg)
{
	struct SingleString_Data *data = (struct SingleString_Data*)INST_DATA(cl,obj);
	if (!DoSuperMethodA(cl, obj, (Msg)msg)) return FALSE;

	data->ehnode.ehn_Priority = 1;
	data->ehnode.ehn_Flags    = 0;
	data->ehnode.ehn_Object   = obj;
	data->ehnode.ehn_Class    = cl;
	data->ehnode.ehn_Events   = IDCMP_RAWKEY;

	return 1;
}

STATIC ULONG SingleString_GoActive(struct IClass *cl, Object *obj,Msg msg)
{
	struct SingleString_Data *data = (struct SingleString_Data*)INST_DATA(cl,obj);
	DoMethod(_win(obj), MUIM_Window_AddEventHandler, (IPTR)&data->ehnode);
	return DoSuperMethodA(cl, obj, msg);
}

STATIC ULONG SingleString_GoInactive(struct IClass *cl, Object *obj,Msg msg)
{
	struct SingleString_Data *data = (struct SingleString_Data*)INST_DATA(cl,obj);
	DoMethod(_win(obj), MUIM_Window_RemEventHandler, (IPTR)&data->ehnode);
	set(obj, MUIA_BetterString_SelectSize, 0);
	return DoSuperMethodA(cl, obj, msg);
}

STATIC ULONG SingleString_HandleEvent(struct IClass *cl, Object *obj, struct MUIP_HandleEvent *msg)
{
	if (msg->imsg && msg->imsg->Class == IDCMP_RAWKEY)
	{
		UWORD code;
		if (msg->imsg->Code == CURSORUP)
		{
			set(obj,MUIA_SingleString_Event, MUIV_SingleString_Event_CursorUp);
			return MUI_EventHandlerRC_Eat;
		}

		if (msg->imsg->Code == CURSORDOWN)
		{
			set(obj,MUIA_SingleString_Event, MUIV_SingleString_Event_CursorDown);
			return MUI_EventHandlerRC_Eat;
		}

		code = ConvertKey(msg->imsg);
		if (code == '\b')
		{
			if (xget(obj,MUIA_String_BufferPos) == 0)
			{
				set(obj,MUIA_SingleString_Event, MUIV_SingleString_Event_ContentsToPrevLine);
				return MUI_EventHandlerRC_Eat;
			}
		}
	}
	return 0;
}

STATIC MY_BOOPSI_DISPATCHER(ULONG, SingleString_Dispatcher, cl, obj, msg)
{
	switch(msg->MethodID)
	{
		case OM_SET: return SingleString_Set(cl, obj, (struct opSet*)msg);
		case OM_GET: return SingleString_Get(cl, obj, (struct opGet*)msg);
		case MUIM_Setup: return SingleString_Setup(cl, obj, (struct MUIP_Setup*)msg);
		case MUIM_GoActive: return SingleString_GoActive(cl, obj, msg);
		case MUIM_GoInactive: return SingleString_GoInactive(cl, obj, msg);
		case MUIM_HandleEvent: return SingleString_HandleEvent(cl, obj, (struct MUIP_HandleEvent*)msg);
		default: return DoSuperMethodA(cl, obj, msg);
	}
}

struct MUI_CustomClass *CL_SingleString;

/* This is the multistring class */

struct object_node { struct node node; Object *obj; };

struct MultiString_Data
{
	struct list object_list;
	char **contents_array;
};

STATIC ULONG MultiString_Set(struct IClass *cl,Object *obj, struct opSet *msg, int construct);
STATIC Object *MultiString_AddStringField(struct IClass *cl,Object *obj, struct MUIP_MultiString_AddStringField *msg,struct object_node *prev);

STATIC VOID MultiString_Acknowledge(void **msg)
{
	struct IClass *cl = (struct IClass*)msg[0];
	Object *obj = (Object*)msg[1];
	Object *new_obj;
	struct object_node *obj_node = (struct object_node*)msg[2];
	struct MUIP_MultiString_AddStringField asf_msg;

	asf_msg.MethodID = MUIM_MultiString_AddStringField;
	asf_msg.contents = "";

	if ((new_obj = MultiString_AddStringField(cl,obj,&asf_msg,obj_node)))
	{
		set((Object*)xget(obj,MUIA_WindowObject), MUIA_Window_ActiveObject, new_obj);
	}
}

STATIC VOID MultiString_Event(void **msg)
{
/*	struct IClass *cl = (struct IClass*)msg[0];*/
	Object *obj = (Object*)msg[1];
	struct object_node *obj_node = (struct object_node*)msg[2];
	Object *window = (Object*)xget(obj,MUIA_WindowObject);
	int event = (IPTR)msg[3];

	if (event == MUIV_SingleString_Event_CursorUp && node_prev(&obj_node->node))
	{
		set(window, MUIA_Window_ActiveObject, ((struct object_node*)node_prev(&obj_node->node))->obj);
		return;
	}

	if (event == MUIV_SingleString_Event_CursorDown && node_next(&obj_node->node))
	{
		set(window, MUIA_Window_ActiveObject, ((struct object_node*)node_next(&obj_node->node))->obj);
		return;
	}

	if (event == MUIV_SingleString_Event_ContentsToPrevLine && node_prev(&obj_node->node))
	{
		struct object_node *prev_node = (struct object_node*)node_prev(&obj_node->node);
		char *contents = (char*)xget(obj_node->obj, MUIA_UTF8String_Contents);
		int new_cursor_pos = strlen((char*)xget(prev_node->obj, MUIA_String_Contents)); /* is Okay */

		DoMethod(prev_node->obj, MUIM_UTF8String_Insert, (IPTR)contents, MUIV_BetterString_Insert_EndOfString);
		set(prev_node-> obj,MUIA_String_BufferPos, new_cursor_pos);
		set(window, MUIA_Window_ActiveObject, prev_node->obj);

		node_remove(&obj_node->node);
		DoMethod(obj, MUIM_Group_InitChange);
		DoMethod(obj, OM_REMMEMBER, (IPTR)obj_node->obj);
		MUI_DisposeObject(obj_node->obj);
		free(obj_node);
		DoMethod(obj, MUIM_Group_ExitChange);
		return;
	}
}

STATIC ULONG MultiString_New(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct MultiString_Data *data;

	if (!(obj=(Object *)DoSuperNew(cl,obj,
					MUIA_Group_Horiz, FALSE,
					TAG_MORE, msg->ops_AttrList)))
	        return 0;

	data = (struct MultiString_Data*)INST_DATA(cl,obj);
	list_init(&data->object_list);
	MultiString_Set(cl,obj,msg,1);
	return (IPTR)obj;
}

STATIC ULONG MultiString_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
	struct MultiString_Data *data = (struct MultiString_Data*)INST_DATA(cl,obj);
	free(data->contents_array);
	return DoSuperMethodA(cl,obj,msg);
}


STATIC ULONG MultiString_Set(struct IClass *cl,Object *group, struct opSet *msg, int construct)
{
	struct MultiString_Data *data = (struct MultiString_Data*)INST_DATA(cl, group);
	struct TagItem *ti = FindTagItem(MUIA_MultiString_ContentsArray, msg->ops_AttrList);
	if (ti || construct)
	{
		struct object_node *obj = (struct object_node*)list_first(&data->object_list);
		static const char *dummy_array[] = {"",NULL};
		const char **array = ti?((const char**)ti->ti_Data):dummy_array;
		int i = 0;
		int group_changed = 0;

		if (!array || !*array) array = dummy_array;

		while (obj)
		{
			struct object_node *next_obj = (struct object_node*)node_next(&obj->node);
			if (array[i])
			{
				set(obj->obj, MUIA_UTF8String_Contents, array[i]);
				i++;
			} else
			{
				if (!group_changed)
				{
					group_changed = 1;
					DoMethod(group, MUIM_Group_InitChange);
				}
				DoMethod(group,OM_REMMEMBER, (IPTR)obj->obj);
				MUI_DisposeObject(obj->obj);
				node_remove(&obj->node);
			}
			obj = next_obj;
		}


		if (array[i])
		{
			struct MUIP_MultiString_AddStringField asf_msg;

			asf_msg.MethodID = MUIM_MultiString_AddStringField;

			if (!group_changed)
			{
				group_changed = 1;
				DoMethod(group,MUIM_Group_InitChange);
			}

			for (;array[i];i++)
			{
				asf_msg.contents = array[i];
				MultiString_AddStringField(cl, group, &asf_msg, (struct object_node*)list_last(&data->object_list));
			}
		}

		if (group_changed)
			DoMethod(group,MUIM_Group_ExitChange);
	}
	if (!construct) return DoSuperMethodA(cl,group,(Msg)msg);
	return 0;
}

STATIC ULONG MultiString_Get(struct IClass *cl,Object *obj, struct opGet *msg)
{
	struct MultiString_Data *data = (struct MultiString_Data*)INST_DATA(cl,obj);
	if (msg->opg_AttrID == MUIA_MultiString_ContentsArray)
	{
		free(data->contents_array);
		if ((data->contents_array = (char**)malloc(sizeof(char*)*(list_length(&data->object_list)+1))))
		{
			struct object_node *obj_node;
			int i = 0;
			obj_node = (struct object_node*)list_first(&data->object_list);
			while (obj_node)
			{
				data->contents_array[i++] = (char*)xget(obj_node->obj,MUIA_UTF8String_Contents);
				obj_node = (struct object_node*)node_next(&obj_node->node);
			}
			data->contents_array[i] = NULL;
			*msg->opg_Storage = (IPTR)data->contents_array;
		}
	}
	return DoSuperMethodA(cl,obj,(Msg)msg);
}

STATIC Object *MultiString_AddStringField(struct IClass *cl,Object *obj, struct MUIP_MultiString_AddStringField *msg, struct object_node *prev_node)
{
	struct MultiString_Data *data = (struct MultiString_Data*)INST_DATA(cl,obj);
	struct object_node *obj_node;

	if ((obj_node = (struct object_node*)malloc(sizeof(struct object_node))))
	{
		obj_node->obj = SingleStringObject,
			MUIA_CycleChain, 1,
			MUIA_UTF8String_Contents, msg->contents,
			MUIA_UTF8String_Charset, user.config.default_codeset,
			End;

		if (obj_node->obj)
		{
			Object **sort_array = (Object**)malloc(sizeof(Object*)*(list_length(&data->object_list)+3));
			list_insert(&data->object_list, &obj_node->node, &prev_node->node);
			DoMethod(obj, MUIM_Group_InitChange);
			DoMethod(obj, OM_ADDMEMBER, (IPTR)obj_node->obj);

			if (sort_array)
			{
				int i = 1;
				struct object_node *cursor = (struct object_node *)list_first(&data->object_list);

				sort_array[0] = (Object*)MUIM_Group_Sort;

				while (cursor)
				{
					sort_array[i] = cursor->obj;
					cursor = (struct object_node*)node_next(&cursor->node);
					i++;
				}
				sort_array[i] = NULL;
				DoMethodA(obj, (Msg)sort_array);

				free(sort_array);
			}

			DoMethod(obj,MUIM_Group_ExitChange);
			DoMethod(obj_node->obj, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, (IPTR)obj, 6, MUIM_CallHook, (IPTR)&hook_standard, (IPTR)MultiString_Acknowledge, (IPTR)cl, (IPTR)obj, (IPTR)obj_node);
			DoMethod(obj_node->obj, MUIM_Notify, MUIA_SingleString_Event, MUIV_EveryTime, (IPTR)App, 10, MUIM_Application_PushMethod, (IPTR)App, 7, MUIM_CallHook, (IPTR)&hook_standard, (IPTR)MultiString_Event, (IPTR)cl, (IPTR)obj, (IPTR)obj_node, MUIV_TriggerValue);
			return obj_node->obj;
		}
	}
	return NULL;
}

STATIC MY_BOOPSI_DISPATCHER(ULONG, MultiString_Dispatcher, cl, obj, msg)
{
	switch (msg->MethodID)
	{
		case  OM_NEW: return MultiString_New(cl,obj,(struct opSet*)msg);
		case  OM_DISPOSE: return MultiString_Dispose(cl,obj,msg);
		case  OM_SET: return MultiString_Set(cl,obj,(struct opSet*)msg,0);
		case  OM_GET: return MultiString_Get(cl,obj,(struct opGet*)msg);
		case  MUIM_MultiString_AddStringField:
					{
						struct MultiString_Data *data = (struct MultiString_Data*)INST_DATA(cl,obj);
						MultiString_AddStringField(cl,obj,(struct MUIP_MultiString_AddStringField*)msg,(struct object_node*)list_last(&data->object_list));
						return 0; /* return value is private */
					}
	}

	return DoSuperMethodA(cl,obj,msg);
}

struct MUI_CustomClass *CL_MultiString;

int create_multistring_class(void)
{
	SM_ENTER;
	if ((CL_SingleString = CreateMCC(NULL, CL_UTF8String, sizeof(struct SingleString_Data), SingleString_Dispatcher)))
	{
		SM_DEBUGF(15,("Create CL_SingleString: 0x%lx\n",CL_SingleString));
		if ((CL_MultiString = CreateMCC(MUIC_Group, NULL, sizeof(struct MultiString_Data), MultiString_Dispatcher)))
		{
			SM_DEBUGF(15,("Create CL_MultiString: 0x%lx\n",CL_MultiString));
			SM_RETURN(TRUE,"%ld");
		}
		SM_DEBUGF(5,("FAILED! Create CL_MultiString\n"));
		SM_RETURN(FALSE,"%ld");
	}
	SM_DEBUGF(5,("FAILED! Create CL_SingleString\n"));
	SM_RETURN(FALSE,"%ld");
}

void delete_multistring_class(void)
{
	SM_ENTER;
	if (CL_MultiString)
	{
		if (MUI_DeleteCustomClass(CL_MultiString))
		{
			SM_DEBUGF(15,("Deleted CL_MultiString: 0x%lx\n",CL_MultiString));
			CL_MultiString = NULL;
		} else
		{
			SM_DEBUGF(5,("FAILED! Delete CL_MultiString: 0x%lx\n",CL_MultiString));
		}
	}

	if (CL_SingleString)
	{
		if (MUI_DeleteCustomClass(CL_SingleString))
		{
			SM_DEBUGF(15,("Deleted CL_SingleString: 0x%lx\n",CL_SingleString));
			CL_SingleString = NULL;
		} else
		{
			SM_DEBUGF(5,("FAILED! Delete CL_SingleString: 0x%lx\n",CL_SingleString));
		}
	}
	SM_LEAVE;
}
