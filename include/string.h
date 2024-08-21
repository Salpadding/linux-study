#ifndef _STRING_H
#define _STRING_H

#define memset(s, c, count)                                                    \
  ({                                                                           \
    __asm__ __volatile__("cld\n\t"                                             \
                         "rep\n\t"                                             \
                         "stosb" ::"a"((unsigned long)(c)),                    \
                         "D"((unsigned long)(s)),                              \
                         "c"((unsigned long)(count)));                         \
    s;                                                                         \
  })

#define memcpy(dest, src, n)                                                   \
  ({                                                                           \
    __asm__("cld\n\t"                                                          \
            "rep\n\t"                                                          \
            "movsb" ::"c"(n),                                                  \
            "S"((unsigned long)(src)), "D"((unsigned long)(dest))              \
            :);                                                                \
    dest;                                                                      \
  })

int strlen(const char *s);

int strncmp(const char *cs, const char *ct, int count);

/*
 * This string-include defines all string functions as inline
 * functions. Use gcc. It also assumes ds=es=data space, this should be
 * normal. Most of the string-functions are rather heavily hand-optimized,
 * see especially strtok,strstr,str[c]spn. They should work, but are not
 * very easy to understand. Everything is done entirely within the register
 * set, making the functions fast and clean. String instructions have been
 * used through-out, making for "slightly" unclear code :-)
 *
 *		(C) 1991 Linus Torvalds
 */

char *strcpy(char *dest, const char *src);

int strcmp(const char *cs, const char *ct);

char *strncpy(char *dest, const char *src, int count);

char *strchr(const char *s, char c);
#endif
