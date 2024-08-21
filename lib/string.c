#include <string.h>

__attribute__((noinline)) int strlen(const char *s) {
  unsigned long __res = 0xffffffff;
  __asm__ __volatile__("cld\n\t"
                       "repne\n\t"
                       "scasb\n\t"
                       "notl %0\n\t"
                       "decl %0"
                       : "+c"(__res)
                       : "D"(s), "a"(0));
  return __res;
}

__attribute__((noinline)) int strncmp(const char *cs, const char *ct,
                                      int count) {
  int __res;
  __asm__ __volatile__("cld\n"
                       "1:\tdecl %3\n\t" // while(--ecx >= 0)
                       "js 2f\n\t"       // check if ecx < 0
                       "lodsb\n\t"       // al = [si++]
                       "scasb\n\t"       // cmp al, [di++]
                       "jne 3f\n\t"
                       "testb %%al,%%al\n\t" // \0 found
                       "jne 1b\n"
                       "2:\txorl %%eax,%%eax\n\t"
                       "jmp 4f\n"
                       "3:\tmovl $1,%%eax\n\t"
                       "jl 4f\n\t" // jl [si] < [di] si=ct, di=cs
                       "negl %%eax\n"
                       "4:"
                       : "=a"(__res)
                       : "D"(cs), "S"(ct), "c"(count)
                       :);
  return __res;
}

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

char *strcpy(char *dest, const char *src) {
  __asm__("cld\n"
          "1:\tlodsb\n\t"
          "stosb\n\t"
          "testb %%al,%%al\n\t"
          "jne 1b" ::"S"(src),
          "D"(dest)
          :);
  return dest;
}

/**
 * char c tmp1, tmp2;
 * while((tmp2 = *cs++) != (tmp1 = *ct++) && tmp1);
 * if(tmp1 != tmp2) return SIGNUM(tmp2 - tmp1);
 * if(!tmp1) return 0;
 *
 *
 */
int strcmp(const char *cs, const char *ct) {
  register int __res;
  __asm__("cld\n"
          "1:\tlodsb\n\t"
          "scasb\n\t"
          "jne 2f\n\t"
          "testb %%al,%%al\n\t"
          "jne 1b\n\t"
          "xorl %%eax,%%eax\n\t"
          "jmp 3f\n"
          "2:\tmovl $1,%%eax\n\t"
          "jl 3f\n\t"
          "negl %%eax\n"
          "3:"
          : "=a"(__res)
          : "D"(cs), "S"(ct)
          :);
  return __res;
}

/*
 * while(--count >= 0) if(!(*(dest++) = *(src++)) ) break
 * while(count--) *(dest++) = 0;
 * */
__attribute__((noinline)) char *strncpy(char *dest, const char *src,
                                        int count) {
  __asm__("cld\n"
          "1:\tdecl %2\n\t" // while(--count >= 0)
          "js 2f\n\t"
          "lodsb\n\t"
          "stosb\n\t"           // if(!(*(dest++) = *(src++)) ) break;
          "testb %%al,%%al\n\t" //
          "jne 1b\n\t"
          "rep\n\t" // while(count--) *(dest++) = 0;
          "stosb\n"
          "2:" ::"S"(src),
          "D"(dest), "c"(count)
          :);
  return dest;
}

/**
 * find first char in string
 * return 0 if not found
 * */
char *strchr(const char *s, char c) {
  char *__res;
  __asm__("cld\n\t"
          "movb %%al,%%ah\n"
          "1:\tlodsb\n\t"
          "cmpb %%ah,%%al\n\t"
          "je 2f\n\t"
          "testb %%al,%%al\n\t"
          "jne 1b\n\t"
          "movl $1,%1\n"
          "2:\tmovl %1,%0\n\t"
          "decl %0"
          : "=a"(__res)
          : "S"(s), "0"(c)
          :);
  return __res;
}
