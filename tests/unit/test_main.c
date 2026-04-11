#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"
#include "test_suites.h"

/*
 * test_main is the single entrypoint of the unit test binary. It builds a
 * root CuSuite, merges every module level suite into it, runs everything
 * and prints the aggregated summary and details to stdout.
 *
 * CuSuiteAddSuite transfers ownership of each CuTest pointer from the
 * sub-suite into the root suite and sets the original slots to NULL, so it
 * is safe to CuSuiteDelete the empty sub-suite shells right after merging
 * them. CuSuiteDelete only frees non-NULL entries, therefore no test case
 * is double freed.
 */
int32_t main(void) {
    CuSuite *root = CuSuiteNew();

    CuSuite *parser_suite = parser_get_suite();
    CuSuiteAddSuite(root, parser_suite);
    CuSuiteDelete(parser_suite);

    CuSuite *error_management_suite = error_management_get_suite();
    CuSuiteAddSuite(root, error_management_suite);
    CuSuiteDelete(error_management_suite);

    CuSuiteRun(root);

    CuString *summary = CuStringNew();
    CuString *details = CuStringNew();

    CuSuiteSummary(root, summary);
    CuSuiteDetails(root, details);

    printf("%s\n", summary->buffer);
    printf("%s\n", details->buffer);

    int32_t exit_code = root->failCount == 0 ? 0 : 1;

    CuStringDelete(summary);
    CuStringDelete(details);
    CuSuiteDelete(root);

    return exit_code;
}
