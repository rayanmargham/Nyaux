#include <stddef.h>
#include <main.h>


#define UACPI_PRIx64 "lx"
#define UACPI_PRIX64 "lX"
#define UACPI_PRIu64 "lu"

#define uacpi_memcpy memcpy
#define uacpi_memset memset
#define uacpi_memcmp memcmp
#define uacpi_strncmp strncmp
#define uacpi_strcmp strcmp
#define uacpi_memmove memmove
#define uacpi_strnlen strnlen
#define uacpi_strlen strlen
#define uacpi_snprintf npf_snprintf

#define uacpi_offsetof offsetof