#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <float.h>
#include <uchar.h>
// #include <common/utils.h>
#include <datum.h>
// #include <common/converters.h>

#define DATUM_STRUCTID 20260117
const size_t THIS_DATUM_TP = 0xe3eceee64a2b360; // sha1 hash from git
static const char *DatumTypeName = "datum";

struct Datum {
    size_t thisTp;
    size_t structId;
    union Value {
        double r;           /* value as double */
        long long i;        /* integer value */
        char *z;            /* string or BLOB value */
        uintptr_t *uptr;    /* value is an universal pointer */
        wchar_t *zW;        /* value as widecharacter string */
    } value;
    size_t n;               /* Number of characters in string value, excluding '\0' */
    size_t sz;              /* number of bytes occupied by string */
    short dec;              /* number of digits after decimalpoint */
    size_t flags;           /* Some combination of DATUM_Null, DATUM_Str, etc. */
    dtm_encoding_t enc;     /* DT_UTF8, DT_UTF16BE, DT_UTF16LE */
    short type;             /* One of DT_NULL, DT_TEXT, DT_INTEGER, etc */
    short isLocked;         /* the value can not be changed */
    unsigned long hash;     /* hashed version of value when char */
};

// Returns the number of characters in an UTF-8 encoded string.
// (Does not check for encoding validity)
int utf8_strlen(const char *s)
{
    int len = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) len++;
        s++;
    }
    return len;
}

/*
Bits of CP First Last Sequence Byte 1 Byte 2 Byte3 Byte4
7 U+0000 U+007F 1 0xxxxxxx - - -
11 U+0080 U+07FF 2 110xxxxx 10xxxxxx - -
16 U+0800 U+FFFF 3 1110xxxx 10xxxxxx 10xxxxxx -
21 U+10000 U+1FFFFF 4 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
size_t utf8_charlen(uint8_t c)
{
    if (c < 0x80) return 1;                        /* 0xxxxxxx */
    else if ((c & 0xe0) == 0xc0) return 2;         /* 110xxxxx */
    else if ((c & 0xf0) == 0xe0) return 3;         /* 1110xxxx */
    else if ((c & 0xf8) == 0xf0 && (c <= 0xf4)) return 4; /* 11110xxx */
    else return 0;                                 /* invalid UTF8 */
}

/**
 * @brief checks if the given utf8 character is valid
 */
size_t utf8_valid(const uint8_t *c)
{
    size_t clen = utf8_charlen(*c);
    if (clen == 0) return 0;

    // Sjekk alle trailing bytes (starter fra indeks 1)
    for (size_t i = 1; i < clen; i++) {
        if ((c[i] & 0xc0) != 0x80) {
            return 0;
        }
    }
    return clen;
}

/**
 * @brief converts utf8 character to utf32 
 */
uint32_t utf8_to_32(const uint8_t *c)
{
    switch (utf8_valid(c))
    {
        case 0: return 0;                           /* invalid utf8 */
        case 1: return *c;                          /* no work, just promote size */
        case 2: return ((c[0] & 0x1f) << 6) | (c[1] & 0x3f);
        case 3: return ((c[0] & 0x0f) << 12) | ((c[1] & 0x3f) << 6) | (c[2] & 0x3f);
        case 4: return ((c[0] & 0x07) << 18) | ((c[1] & 0x3f) << 12) | ((c[2] & 0x3f) << 6) | (c[3] & 0x3f);
    }
    return 0;                                       /* no complaints gcc */
}

/**
 * @brief returns the typeof datum
 *
 * @param val pointer to a datum instance
 * @return char * if given and ok typeof fs, otherwise NULL
 */
const char* Datum_typeof(void *val)
{
    return (val && Datum_isDatum(val)) ? DatumTypeName : NULL;
}

/**
 * @brief validates if the given value is a datum struct
 *
 * @param val pointer to value
 * @return true the value is a datum struct
 * @return false the value is not a datum struct
 */
bool Datum_isDatum(void *val)
{
    return (val && ((Datum_T)val)->thisTp == THIS_DATUM_TP) ? true : false;
}

/**
 * @brief Creates a new Datum "object"
 *
 * @return Datum_T
 */
Datum_T Datum_new(void)
{
    Datum_T datum = calloc(1, sizeof(struct Datum));
    if (datum)
    {
        datum->thisTp = THIS_DATUM_TP;
        datum->structId = DATUM_STRUCTID;
        datum->flags |= DATUM_Dyn;
        return datum;
    }
    else
        return NULL;
}

/**
 * @brief
 *
 * @param datum
 */
void Datum_free(Datum_T *datum)
{
    if (datum && Datum_isDatum(*datum))
    {
        /* ok */
    }
    else
        return; //-- return()

    if ((*datum)->flags & DATUM_Str)
    {
        free((*datum)->value.z);
        (*datum)->value.z = NULL;
    }
    else if ((*datum)->flags & DATUM_StrW)
    {
        free((*datum)->value.zW);
        (*datum)->value.zW = NULL;
    }
    else if ((*datum)->flags & DATUM_StrU)
    {
        uint32_t *ustr = (uint32_t *)(*datum)->value.uptr;
        if (ustr)
            free(ustr);
        (*datum)->value.uptr = NULL;
    }
    else if ((*datum)->flags & DATUM_Blob)
    {
        free((*datum)->value.z);
        (*datum)->value.z = NULL;
    };

    free(*datum);
    *datum = NULL;
    return;
}

bool Datum_isLocked(Datum_T datum)
{
    return (Datum_isDatum(datum) && datum->isLocked) ? true : false;
}

bool Datum_isString(Datum_T datum)
{
    return (Datum_isDatum(datum) && datum->flags & DATUM_Str) ? true : false;
}

bool Datum_isStringW(Datum_T datum)
{
    return (Datum_isDatum(datum) && datum->flags & DATUM_StrW) ? true : false;
}

bool Datum_isInteger(Datum_T datum)
{
    return (Datum_isDatum(datum) && datum->flags & DATUM_Int) ? true : false;
}

bool Datum_isDouble(Datum_T datum)
{
    return (Datum_isDatum(datum) && datum->flags & DATUM_Double) ? true : false;
}

bool Datum_isNull(Datum_T datum)
{
    return (Datum_isDatum(datum) && datum->flags & DATUM_Null) ? true : false;
}

/**
 * @brief Creates a new Datum as an integer and sets its value to val
 * @param val The integer value to store
 * @return New Datum_T or NULL on allocation failure
 */
Datum_T Datum_asInteger(long long val)
{
    Datum_T datum = Datum_new();
    if (!datum) {
        return NULL;
    }

    datum->value.i = val;
    datum->flags |= DATUM_Int | DATUM_Dyn;  // Dyn fordi vi allokerte med calloc
    // type-feltet ditt kan også settes her hvis du bruker det: datum->type = DATUM_Int;

    return datum;
}

/**
 * @brief Creates a new Datum as a double and sets its value to val
 * @param val The double value to store
 * @return New Datum_T or NULL on allocation failure
 */
Datum_T Datum_asDouble(double val)
{
    Datum_T datum = Datum_new();
    if (!datum) {
        return NULL;
    }

    datum->value.r = val;
    datum->flags |= DATUM_Double | DATUM_Dyn;
    // type-feltet ditt kan også settes her hvis du bruker det: datum->type = DATUM_Double;

    return datum;
}

/**
 * @brief Returns the value as double.
 *
 * If the datum is a double, returns it directly.
 * If the datum is an integer, converts it to double.
 * Otherwise (not numeric, NULL, or invalid datum), returns DBL_MAX as error indicator.
 *
 * @param datum Valid Datum pointer
 * @return The double value, or DBL_MAX on error/invalid type
 */
double Datum_getAsDouble(Datum_T datum)
{
    if (!datum || !Datum_isDatum(datum)) {
        return DBL_MAX;  // Tidlig retur ved ugyldig input
    }

    if (datum->flags & DATUM_Double) {
        return datum->value.r;
    }

    if (datum->flags & DATUM_Int) {
        return (double)datum->value.i;  // Konverter int til double
    }

    return DBL_MAX;  // Ikke numerisk type
}

/**
 * @brief Returns the value as long long.
 *
 * - If the datum is an integer, returns it directly.
 * - If the datum is a double, converts it to long long (truncates decimal part).
 * - Otherwise (not numeric, NULL, or invalid datum), returns LONG_MAX as error indicator.
 *
 * @note Converting from double may lose precision or cause overflow (no range check).
 * @param datum Valid Datum pointer
 * @return The long long value, or LONG_MAX on error/invalid type
 */
long long Datum_getAsInteger(Datum_T datum)
{
    if (!datum || !Datum_isDatum(datum)) {
        return LONG_MAX;
    }

    if (datum->flags & DATUM_Int) {
        return datum->value.i;
    }

    if (datum->flags & DATUM_Double) {
        return (long long)datum->value.r;  // Truncates toward zero
    }

    return LONG_MAX;
}
