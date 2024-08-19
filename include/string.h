#ifndef _STRING_H
#define _STRING_H

int strlen(const char *s);

void *memset(void *s, char c, int count);

int strncmp(const char *cs, const char *ct, int count);

void *memcpy(void *dest, const void *src, int n);

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


char *strncpy(char *dest, const char *src,
                                        int count);

char *strchr(const char *s, char c);
#endif
