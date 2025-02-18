# Error handling

All functions return zero on success and non-zero on error.

Some errors such as integer overflow, OOM, decoding errors may lead to a
non-recoverable state, in this case all subsequent function calls will
return `SPNG_EBADSTATE`.

See also: [Decoder error handling](decode.md#error-handling)

# Functions

# spng_strerror()

```c
const char *spng_strerror(int err)
```

Return the error message for the given error code.
