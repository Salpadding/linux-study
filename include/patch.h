#ifndef _PATCH_H
#define _PATCH_H

#include <sys/types.h>

#define inode m_inode

void rw_swap_page(int rw, unsigned int nr, char * buf);

#define ll_rw_swap_file(x1, x2, x3, x4, x5)
#define i_rdev i_dev

#endif
