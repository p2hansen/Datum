#pragma once
/*
 * datum.h
 *
 * Datum: A versatile data container for multiple data types and encodings.
 *
 * This header defines the Datum structure and associated functions for
 * creating, manipulating, and querying Datum objects. Datum can hold
 * various data types including integers, doubles, strings (in multiple
 * encodings), blobs, arrays of datums, and void pointers.
 *
 * Supported string encodings include UTF-8, UTF-16 (LE/BE), ASCII,
 * ISO-8859 variants, and Windows-1252, with internal processing using
 * UTF-16 for consistency across platforms.
 *
 * Created by: p2hansen
 */

#include <stdbool.h>   // bool, true, false
#include <stddef.h>    // size_t
#include <stdint.h>    // uint32_t, uint8_t, ..

#define DATUM_STR_MAXSIZE 32764

#define DATUM_UTF8          1
#define DATUM_UTF16LE       2	    /* standard ms */
#define DATUM_UTF16BE       3
#define DATUM_UTF16         4       /* Use native byte order */
#define DATUM_ASCII         5 
#define DATUM_UTF32         6       /* Use native byte order */
#define DATUM_UTF32LE       7     
#define DATUM_UTF32BE       8   

/* https://no.wikipedia.org/wiki/ISO_8859 */
#define DATUM_ISO8859_1     11      /* Latin-1 Vesteuropeisk            */
#define DATUM_ISO8859_2     12      /* Latin-2 Sentraluropeisk          */
#define DATUM_ISO8859_15    25      /* Latin-9, Latin_1 + fr,fi,est,€   */ 

/* https://zims-en.kiwix.campusafrica.gos.orange.com/wikipedia_en_all_nopic/A/ISO-IR-197 */
#define DATUM_ISO_IR_197    28
#define DATUM_ISO_IR_197WIN 29

#define DATUM_CH_1252       30


/*
 * Encoding support in Datum
 * -------------------------
 * Datum supports a range of legacy and modern text encodings, with special
 * attention to Nordic and Sami characters (ISO-IR-197, ISO-8859-15, Windows-1252).
 *
 * Key platform notes:
 * - char*          : Narrow strings (8-bit), used for UTF-8, ASCII, ISO-8859-*, Windows-1252
 * - wchar_t*       : Wide strings, 16-bit on Windows (UTF-16LE native), 32-bit on Linux/macOS
 * - uint32_t*      : UTF-32 (used internally or for portable wide strings)
 *
 * All internal string processing uses UTF-16 as canonical form to simplify
 * conversion and ensure consistent behavior across platforms.
 *
 * References:
 * - Microsoft UCRT / Win32 wide-char API uses UTF-16LE
 * - POSIX/Linux prefers UTF-8 and wchar_t = UTF-32
 */
typedef enum {
    DTM_ENC_NONE        = 0         /* either utf-8 or ISO_8859-15      */
  , DTM_ENC_UTF8        = DATUM_UTF8
  , DTM_ENC_UTF16       = DATUM_UTF16
  , DTM_ENC_ASCII       = DATUM_ASCII
  , DTM_ENC_UTF32       = DATUM_UTF32 
  , DTM_ENC_ISO8859_1   = DATUM_ISO8859_1
  , DTM_ENC_ISO8859_2   = DATUM_ISO8859_2
  , DTM_ENC_ISO8859_15  = DATUM_ISO8859_15
  , DTM_ENC_ISO_IR_197  = DATUM_ISO_IR_197
  , DTM_ENC_ISO_IR_197W = DATUM_ISO_IR_197WIN
  , DTM_ENC_CH1252      = DATUM_CH_1252
} dtm_encoding_t;

#define DATUM_Null      0x0001      /* Value is NULL */
#define DATUM_Int       0x0002      /* Value is an integer */
#define DATUM_Double    0x0004      /* Value is a double */
#define DATUM_Bool      0x0008      /* Value is a bool */
#define DATUM_Str       0x0010      /* Value is a string */
#define DATUM_StrW      0x0020      /* Value is wide character string */
#define DATUM_Blob      0x0040      /* Value is a BLOB */
#define DATUM_Datums    0x2000      /* Value is an array of datums */
#define DATUM_Array     0x4000      /* Value is an array */
#define DATUM_UINTPTR	0x8000	    /* value is an universal void ptr */
#define DATUM_StrU      0x0010000   /* Value is string on utf32 */

#define DATUM_Invalid   0x00800000  /* Value is undefined */

/* Whenever Datum contains a valid string or blob representation, one of
** the following flags must be set to determine the memory management
** policy for Datum.z.  The DATUM_Term flag tells us whether or not the
** string is \000 or \u0000 terminated
*/
#define DATUM_Term      0x01000000   /* String rep is nul terminated */
#define DATUM_Dyn       0x02000000   /* Need to call sqliteFree() on Mem.z */
#define DATUM_Static    0x04000000   /* Mem.z points to a static string */
#define DATUM_Ephem     0x08000000   /* Mem.z points to an ephemeral string */
#define DATUM_Agg       0x10000000   /* Mem.z points to an agg function context */
#define DATUM_Zero      0x20000000   /* Mem.i contains count of 0s appended to blob */

typedef struct Datum *Datum_T;

extern Datum_T Datum_new(void);

extern const char* Datum_typeof(void *dtm);

extern Datum_T Datum_asString(const char *str, int len, dtm_encoding_t encoding);
extern Datum_T Datum_asStringW(const wchar_t *str, int len);
extern Datum_T Datum_asStringU(const unsigned int *str, int len);
extern Datum_T Datum_asInteger(long long val);
extern Datum_T Datum_asDouble(double val);
extern Datum_T Datum_asVoidPtr(void *val);
extern Datum_T Datum_asBLOB(void *str, int len, short enc);
extern Datum_T Datum_newAsTimestamp();
extern Datum_T Datum_asArray(void *arr, int len);
extern Datum_T Datum_asDatums(Datum_T *datums, int len);

extern long Datum_getType(Datum_T datum);

extern bool Datum_isDatum(void *datum);
extern bool Datum_isLocked(Datum_T datum);
extern short Datum_toggleLocked(Datum_T datum);
extern bool Datum_isString(Datum_T datum);
extern bool Datum_isStringW(Datum_T datum);
extern bool Datum_isInteger(Datum_T datum);
extern bool Datum_isBlob(Datum_T datum);
extern bool Datum_isDouble(Datum_T datum);
extern bool Datum_isDatums(Datum_T datum);    // new

extern bool Datum_isNull(Datum_T datum);
extern bool Datum_isEqual(Datum_T datum_1, Datum_T datum_2);

extern unsigned char *Datum_getAsString(Datum_T datum, dtm_encoding_t encoding);
extern wchar_t *Datum_getAsStringW(Datum_T datum);
extern uint32_t *Datum_getAsStringU(Datum_T datum);
extern void *Datum_getAsBlob(Datum_T datum);
extern long long  Datum_getAsInteger(Datum_T datum);
extern double Datum_getAsDouble(Datum_T datum);
extern Datum_T *Datum_getAsDatums(Datum_T datum); // new

extern long Datum_getSize(Datum_T datum);               //!new number of bytes 
extern long Datum_getLength(Datum_T datum);             //!new number of characters
extern dtm_encoding_t Datum_getEncoding(Datum_T datum); //!new
extern long Datum_getDatatype(Datum_T datum);           //!new

extern Datum_T Datum_copy(Datum_T datum);

extern void Datum_free(Datum_T *datum);