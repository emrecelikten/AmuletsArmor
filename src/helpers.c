#include <ctype.h>

/**
 * Contains code for missing extension functions.
 */
/**
 * Since ANSI does not provide itoa, we have to roll out our own implementation
 * Written by Lukás Chmela
 * Released under GPLv3.
 */
char *
itoa(int value, char *result, int base)
{
    // check that the base if valid
    if (base < 2 || base > 36)
    {
        *result = '\0';
        return result;
    }

    char *ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do
    {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35
            + (tmp_value
                - value * base)];
    }
    while (value);

    // Apply negative sign
    if (tmp_value < 0)
        *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr)
    {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

/* Platform specific string comparison functions */
#ifdef TARGET_UNIX
// Unsafe hacky implementation of a case insensitive string comparison, found from stack overflow and modified
int
strnicmp(char const *a, char const *b, int num)
{
    int i;
    int d;
    for (i = 0; i < num; a++, b++)
    {
        d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a)
            return d;
    }
}

int
stricmp(char const *a, char const *b)
{
    int d;
    for (;; a++, b++)
    {
        d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a)
            return d;
    }
}
#endif