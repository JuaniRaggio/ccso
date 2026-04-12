#pragma once

#include "CuTest.h"

/*
 * One factory function per module under test. Each returns a freshly
 * allocated CuSuite containing the tests defined in that translation unit.
 * The caller is responsible for CuSuiteDelete.
 */
CuSuite *parser_get_suite(void);
CuSuite *error_management_get_suite(void);
CuSuite *game_sync_get_suite(void);
CuSuite *game_admin_get_suite(void);
