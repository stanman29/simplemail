/**
 * index_unittest - a simple test for the simple string index interface for SimpleMail.
 * Copyright (C) 2012  Sebastian Bauer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <CUnit/Basic.h>

#include "string_pools.h"

/*******************************************************/

/* @Test */
void test_string_pool(void)
{
	struct string_pool *p;
	char *dup_hallo_0 = strdup("Hallo0");

	p = string_pool_create();

	CU_ASSERT_PTR_NOT_NULL(p);

	int hallo_id_0 = string_pool_ref(p, "Hallo0");
	int hallo_id_1 = string_pool_ref(p, "Hallo1");

	CU_ASSERT_EQUAL(hallo_id_0, 0);
	CU_ASSERT_EQUAL(hallo_id_1, 1);

	CU_ASSERT_EQUAL(string_pool_ref(p, "Hallo0"), 0);
	CU_ASSERT_STRING_EQUAL(string_pool_get(p, 0), "Hallo0");
	CU_ASSERT_STRING_EQUAL(string_pool_get(p, 1), "Hallo1");

	string_pool_delete(p);
	free(dup_hallo_0);
}