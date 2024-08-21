#ifndef _STRING_H
#define _STRING_H

#define memset(s, c, count)                                                    \
  ({                                                                           \
    __asm__ __volatile__("movl %0, %%edi\n\t"                                  \
                         "movl %1, %%ecx\n\t"                                  \
                         "cld\n\t"                                             \
                         "rep\n\t"                                             \
                         "stosb"                                               \
                         :                                                     \
                         : "g"((unsigned long)(s)),                            \
                           "g"((unsigned long)(count)),                        \
                           "a"((unsigned long)(c))                             \
                         : "edi", "ecx");                                      \
    s;                                                                         \
  })

#define memcpy(dest, src, n)                                                   \
  ({                                                                           \
    __asm__("movl %0, %%ecx\n\t"                                               \
            "movl %1, %%esi\n\t"                                               \
            "movl %2, %%edi\n\t"                                               \
            "cld\n\t"                                                          \
            "rep\n\t"                                                          \
            "movsb" ::"g"(n),                                                  \
            "g"((unsigned long)(src)), "g"((unsigned long)(dest))              \
            : "ecx", "esi", "edi");                                            \
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
