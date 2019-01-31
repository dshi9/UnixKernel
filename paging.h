#include "types.h"

#ifndef _PAGING_H
#define _PAGING_H
extern uint32_t page_dir[1024] __attribute__((aligned(4096)));
extern uint32_t page_table_1[1024] __attribute__((aligned(4096)));
/*defined in page-linkage.S*/
extern void enablePaging();
extern void enableExPaging();

void init_paging();

void
load_page_dir(uint32_t* page_directory);
void page_map(uint32_t virtual_addr, uint32_t phys_addr, uint32_t flags);
void video_page_map(uint32_t virtual_addr);
/*
void
enable_paging();

void
enable_extended_page();
*/
#endif
