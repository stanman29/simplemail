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
#ifndef SM__FILTERRULECLASS_H
#define SM__FILTERRULECLASS_H

#define FilterRuleObject (Object*)MyNewObject(CL_FilterRule->mcc_Class, NULL

extern struct MUI_CustomClass *CL_FilterRule;

#define MUIA_FilterRule_Type			   (TAG_USER+0x3000101) /* I.. LONG */
#define MUIA_FilterRule_Data				 (TAG_USER+0x3000102) /* I.. struct filter_rule * */

int create_filterrule_class(void);
void delete_filterrule_class(void);

#endif  /* FC__AUTOGROUPCLASS_H */

