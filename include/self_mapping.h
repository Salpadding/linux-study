#ifndef _SELF_MAPPING_H
#define _SELF_MAPPING_H

#define PG_ENTRY_N(x, n, prefix)                                               \
  ((unsigned long *)(((((unsigned long)(x)) >> n) | prefix) & (~(3UL))))

#define PG_TBL_ENTRY(x) PG_ENTRY_N(x, 10, 0xffc00000UL)
#define PG_DIR_ENTRY(x) PG_ENTRY_N(x, 20, 0xfffff000UL)

#endif
