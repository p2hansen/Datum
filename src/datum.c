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
const static char *DatumTypeName = "datum";

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

size_t utf8_valid(const uint8_t *c)
{
    size_t clen = utf8_charlen(*c);
    switch (clen)
    {
        case 4: if ((c[3] & 0xc0) != 0x80) return 0;
        case 3: if ((c[2] & 0xc0) != 0x80) return 0;
        case 2: if ((c[1] & 0xc0) != 0x80) return 0;
        case 1: return clen;                        /* no trailing bytes to validate */
        case 0: return 0;                           /* invalid utf8 */
    }
    return clen;                                    /* don't complain, gcc */
}

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