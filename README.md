# Datum

**Datum** is a lightweight, immutable variant/union type library written in pure C.

It provides a safe and consistent way to handle different data types (integers, doubles, strings, blobs, arrays) with built-in support for text encodings commonly found in Nordic and legacy systems:

- UTF-8, UTF-16LE/BE
- ISO-8859-1/2/15
- Windows-1252
- ISO-IR-197 (Sami/Nordic support)
- ASCII

Designed for use in test data masking, synthetic data generation, data migration, and legacy system integration where encoding issues are common.

Features:
- Immutable values (no accidental modifications)
- Memory ownership policies (dynamic/static/ephemeral)
- Encoding detection and conversion
- Type-safe accessors
- Simple and dependency-free core

Currently focused on numeric types; string and encoding support coming soon.

## Status
Early development – not yet production-ready, but actively worked on.

## License
MIT

## Building
```bash
make
make test