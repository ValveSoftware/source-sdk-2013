# Versioning

Releases follow the [semantic versioning](https://semver.org/) scheme with additional guarantees:

* Releases from 0.4.0 to 0.8.x are stable
* If 1.0.0 will introduce breaking changes then 0.8.x will be maintained as a separate stable branch

Currently 1.0.0 is planned to be [compatible](https://github.com/randy408/libspng/issues/3).

# Macros

`SPNG_VERSION_MAJOR`

libspng version's major number

`SPNG_VERSION_MINOR`

libspng version's minor number

`SPNG_VERSION_PATCH`

libspng version's patch number


# Functions

# spng_version_string()

```c
const char *spng_version_string(void)
```

Returns the library version as a string.
