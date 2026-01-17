#include "acutest.h"
#include "datum.h"

static void test_new_and_free(void) {
    Datum_T d = Datum_new();
    TEST_CHECK(d != NULL);
    TEST_CHECK(Datum_isDatum(d));

    Datum_free(&d);
    TEST_CHECK(d == NULL);
}

static void test_invalid_pointer(void) {
    TEST_CHECK(!Datum_isDatum(NULL));
    Datum_free(NULL);  // Skal ikke kræsje
}

TEST_LIST = {
    { "new_and_free", test_new_and_free },
    { "invalid_pointer", test_invalid_pointer },
    { NULL, NULL }
};