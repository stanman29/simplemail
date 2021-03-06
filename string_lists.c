/**
 * @file string_lists.c
 */

#include "string_lists.h"

#include <stdlib.h>

#include "support_indep.h"

/*****************************************************************************/

void string_list_init(struct string_list *list)
{
	list_init(&list->l);
}

/*****************************************************************************/

struct string_node *string_list_first(const struct string_list *list)
{
	return (struct string_node*)list_first(&list->l);
}

/*****************************************************************************/

struct string_node *string_list_last(const struct string_list *list)
{
	return (struct string_node*)list_last(&list->l);
}

/*****************************************************************************/

struct string_node *string_node_next(const struct string_node *node)
{
	return (struct string_node*)node_next(&node->node);
}

/*****************************************************************************/

void string_list_insert_tail_node(struct string_list *list, struct string_node *node)
{
	list_insert_tail(&list->l, &node->node);
}

/*****************************************************************************/

void string_list_insert_after(struct string_list *list, struct string_node *newnode, struct string_node *prednode)
{
	list_insert(&list->l, &newnode->node, &prednode->node);
}

/*****************************************************************************/

struct string_node *string_list_remove_head(struct string_list *list)
{
	return (struct string_node *)list_remove_head(&list->l);
}

/*****************************************************************************/

struct string_node *string_list_remove_tail(struct string_list *list)
{
	return (struct string_node *)list_remove_tail(&list->l);
}

/*****************************************************************************/

struct string_node *string_list_insert_tail(struct string_list *list, const char *string)
{
	struct string_node *node = (struct string_node*)malloc(sizeof(struct string_node));
	if (node)
	{
		if ((node->string = mystrdup(string)))
		{
			list_insert_tail(&list->l,&node->node);
			return node;
		}
		free(node);
	}
	return NULL;
}

/*****************************************************************************/

struct string_node *string_list_insert_tail_always(struct string_list *list, const char *string)
{
	struct string_node *node = (struct string_node*)malloc(sizeof(struct string_node));
	if (node)
	{
		if ((node->string = malloc(strlen(string) + 1)))
		{
			strcpy(node->string, string);
			list_insert_tail(&list->l,&node->node);
			return node;
		}
	}
	return NULL;
}

/*****************************************************************************/

struct string_node *string_list_insert_tail_always_len(struct string_list *list, const char *string, int len)
{
	struct string_node *node;

	if (!(node = (struct string_node*)malloc(sizeof(struct string_node))))
	{
		return NULL;
	}

	if (!(node->string = (char *)malloc(len + 1)))
	{
		free(node);
		return NULL;
	}
	strncpy(node->string, string, len);
	node->string[len] = 0;
	string_list_insert_tail_node(list, node);

	return node;
}

/*****************************************************************************/

void string_list_clear(struct string_list *list)
{
	struct string_node *node;
	while ((node = string_list_remove_tail(list)))
	{
		if (node->string) free(node->string);
		free(node);
	}
}

/*****************************************************************************/

void string_list_exchange(struct string_list *a, struct string_list *b)
{
	struct string_list t;

	t = *a;
	*a = *b;
	*b = t;
}

/*****************************************************************************/

void string_list_free(struct string_list *list)
{
	if (!list) return;
	string_list_clear(list);
	free(list);
}

/*****************************************************************************/

struct string_node *string_list_find(struct string_list *list, const char *str)
{
	struct string_node *node = (struct string_node*)list_first(&list->l);
	while (node)
	{
		if (!mystricmp(str,node->string)) return node;
		node = (struct string_node*)node_next(&node->node);
	}
	return NULL;
}

/*****************************************************************************/

struct string_node *string_list_find_by_index(struct string_list *list, int index)
{
	struct string_node *node = (struct string_node*)list_first(&list->l);
	while (node && index--)
	{
		node = (struct string_node*)node_next(&node->node);
	}
	return node;
}

/*****************************************************************************/

int string_list_length(const struct string_list *l)
{
	return list_length(&l->l);
}
