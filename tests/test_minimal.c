#include <stdio.h>
#include <assert.h>
#include "datum.h"

int main(void) {
    printf("Minimal Datum test\n");

    Datum_T d = Datum_new();
    if (!d) {
        printf("Feil: Kunne ikke allokere\n");
        return 1;
    }

    assert(Datum_isDatum(d));
    printf("Datum opprettet OK\n");

    Datum_free(&d);
    assert(d == NULL);
    printf("Datum frigjort OK\n");

    printf("Test OK!\n");
    return 0;
}