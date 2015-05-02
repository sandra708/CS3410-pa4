#include "kernel.h"

static int ram_start_page, ram_end_page, ram_pages;

void *physical_to_virtual(unsigned int paddr)
{
  // By default, virtual address 0xC0000000 + N is mapped to physical address N.
  // There may other virtual addresses that also map to physical address N, but
  // for this function any one will do, so we use the 0xC0000000 + N mapping.
  return (void *)(0xC0000000 + paddr);
}

unsigned int virtual_to_physical(void *vptr)
{
  // If the virtual address is of the form 0xC0000000 + N, then the physical
  // address is just N (as long as we don't modify any of these mappings).
  if (vptr >= (void *)0xC0000000)
    return (vptr - (void *)0xC0000000);

  // If the virtual address is not of this form, then the physical address could
  // be anything, and we have to traverse the pagetables to figure it out.
  unsigned int vaddr = (unsigned int)vptr;
  unsigned int pdi = vaddr >> 22;
  unsigned int pti = (vaddr >> 12) & 0x3ff;
  unsigned int off = vaddr & 0xfff;

  unsigned int context = current_cpu_context();
  unsigned int ppn = context >> 12;
  if (ppn < ram_start_page || ppn >= ram_end_page) {
    printf("context register seems to point to non-RAM\n");
    return NOPAGE;
  }

  unsigned int *pd = physical_to_virtual(ppn << 12);
  unsigned int pde = pd[pdi];
  if (!(pde & 0x1)) {
    printf("PDE is invalid for virtual address %p\n", vptr);
    return NOPAGE;
  }
  
  unsigned int *pt = physical_to_virtual(pde & ~0xFFF);
  unsigned int pte = pt[pti];
  if (!(pte & 0x1)) {
    printf("PTE is invalid for virtual address %p\n", vptr);
    return NOPAGE;
  }

  return (pte & ~0xFFF) | off;
}

// some useful memory helper routines
void *memset(void *s, unsigned int c, unsigned int len)
{
  unsigned char *p = s;
  for (int i = 0; i < len; i++)
    p[i] = (unsigned char)c;
  return s;
}

void *memcpy(void *dest, const void *src, unsigned int len)
{
  unsigned char *d = dest;
  const unsigned char *s = src;
  for (int i = 0; i < len; i++)
    d[i] = s[i];
  return dest;
}


// set the ith bit in an array to given value
static void bitmap_set(unsigned char *bitmap, int i, int value) {
  // for i=0, use the LSB of bitmap[0], for i=7, use the MSB of bitmap[0], etc.
  if (value) bitmap[i/8] |= (1 << (i % 8));
  else bitmap[i/8] &= ~(1 << (i % 8));
}

// get the value of the ith bit in an array
static int bitmap_get(unsigned char *bitmap, int i) {
  return (bitmap[i/8] >> (i % 8)) & 0x1;
}

// The following is very simple page allocator.
//
// It can only allocate or free whole RAM pages at a time. It tracks which pages
// are in use using a simple bitmap with 1 bit of state per page. It does not
// attempt to track what each page allocation was for, or anything more
// sophisticated than a single "free/busy" bit. It only works for RAM pages.

static void *page_alloc_bitmap;
static unsigned int page_alloc_hint;
static unsigned int pages_reserved;

static void page_alloc_init()
{
  // how many pages do we need for our free/busy bitmap?
  int bits_per_page = 8*PAGE_SIZE;
  int n = (ram_pages + bits_per_page - 1) / bits_per_page;
  // we assume that the bootpages were taken sequentially, starting at
  // ram_start_page, so we can just take the next n pages for our bitmap.
  page_alloc_bitmap = physical_to_virtual((ram_start_page + bootparams->bootpages) << 12);
  memset(page_alloc_bitmap, 0, n * PAGE_SIZE);
  // we forbid anything lower than pages_reserved from ever being freed, so we
  // don't even keep it in the bitmap.
  pages_reserved = bootparams->bootpages + n;
  // for allocation, start the search near page_alloc_hint
  page_alloc_hint = 0;
}

void *alloc_pages(unsigned int count)
{
  if (count == 0 || count > ram_pages - pages_reserved) {
    printf("alloc_pages: sorry, can't allocate %d pages (only %d RAM pages available)\n",
	count, ram_pages - pages_reserved);
    shutdown();
  }
  // look for count free pages in a row
  int seen = 0;
  for (int i = 0; i < ram_pages - pages_reserved; i++) {
    int end = (page_alloc_hint + i) % (ram_pages - pages_reserved);
    if (end == 0)
      seen = 0; // just wrapped around to the start of the bitmap
    if (bitmap_get(page_alloc_bitmap, end) == 0)
      seen++;
    if (seen == count) {
      int start = end - count + 1;
      // start through end (inclusive) are all free
      for (i = start; i <= end; i++)
	bitmap_set(page_alloc_bitmap, i, 1);
      page_alloc_hint = (end + 1) % (ram_pages - pages_reserved);
      return physical_to_virtual((ram_start_page + pages_reserved + start) << 12);
    }
  }
  printf("alloc_pages: no free pages left, sorry\n");
  shutdown();
}

void *calloc_pages(unsigned int count)
{
  void *pages = alloc_pages(count);
  memset(pages, 0, count * PAGE_SIZE);
  return pages;
}

void free_pages(void *page, unsigned int count)
{
  if (count == 0 || count > ram_pages - pages_reserved) {
    printf("free_pages: sorry, can't free %d pages (only %d RAM pages available)\n",
	count, ram_pages - pages_reserved);
    shutdown();
  }
  void *end = page + count*PAGE_SIZE - 1;
  if (page < (void *)0xC0000000 || end < (void *)0xC0000000) {
    printf("free_pages: virtual address %p through %p is too low to have come from alloc_pages\n", page, end);
    shutdown();
  }
  unsigned int paddr = virtual_to_physical(page);
  if (paddr & 0xfff) {
    printf("free_pages: virtual address %p is not aligned properly\n", page);
    shutdown();
  }
  unsigned int ppn = paddr / PAGE_SIZE;
  if (ppn < ram_start_page || ppn + count > ram_end_page) {
    printf("free_pages: virtual address %p is not mapped to RAM\n", page);
    shutdown();
  }
  if (ppn < ram_start_page + pages_reserved) {
    printf("free_pages: virtual address %p is reserved and should never be freed\n", page);
    shutdown();
  }
  while (count > 0) {
    int i = (ppn - ram_start_page - pages_reserved);
    if (bitmap_get(page_alloc_bitmap, i) == 0) {
      printf("free_pages: virtual address %p is already free\n", page);
      shutdown();
    }
    bitmap_set(page_alloc_bitmap, i, 0);
    count--;
    page += PAGE_SIZE;
    ppn++;
  }
}

// The following is fairly simple malloc implementation.
//
// It can allocate or free arbitrary size RAM blocks. It rounds the block sizes
// upwards to make accounting simpler, as follows:
//
// For small allocations (half a page or less), it rounds up to the nearest
// power of two, and groups allocations into buckets by size. For instance, all
// allocations at least 65 bytes but no more than 128 bytes are all rounded up
// to 128 bytes. The allocations are then packed into pages, e.g. for 128 byte
// allocations, 16 allocations fit on a single page, minus accounting overhead.
//
// For large allocations (more than half a page), it rounds up to the nearest
// multiple of the page size and just uses alloc_pages(). This implementation
// doesn't make any effort to deal with fragmentation. For instance,
// alloc_pages(100) will fail if there are not 100 physically contiguous pages
// available, even if there are plenty of individual pages available.  In
// principle, we could use virtual memory to make 100 individual pages appear
// virtually contiguous, but this implementation doesn't even try.

#define MIN_BLOCKSIZE2 5   // 2^5 = 32 bytes
#define MAX_BLOCKSIZE2 11  // 2^11 = 2048 bytes

// For the smallest possible allocation, 32 bytes, the page can hold at most
// 4096/32 = 128 blocks. In this case, the bitmap needs 128/8 = 16 bytes, so the
// smallblock_info will take up exactly 32 bytes, or one block. So in all cases, we
// will reserve the first block (however big it happens to be) of each page for
// the smallblock_info accounting data.
struct smallblock_info {
  unsigned int magic; // 0xfeedface, for debugging and sanity checks
  unsigned int blocksize; // size of blocks on this page
  struct smallblock_info *prev, *next; // doubly-linked list with other pages of same blocksize
  unsigned char bitmap[16]; // bitmap of busy blocks within this page
};

// For large allocations, there is no bitmap and no linked lists. We just keep
// track of the number of pages, so that we can call free_pages() with the
// proper argument.
struct bigblock_info {
  unsigned int magic; // 0xf00dface, for debugging and sanity checks
  unsigned int pagecount;
};

#define NUM_BLOCKSIZES (MAX_BLOCKSIZE2 - MIN_BLOCKSIZE2 + 1) // 5 though 11 inclusive
struct smallblock_info smallblock[NUM_BLOCKSIZES];

static void malloc_init()
{
  for (int i = 0; i < NUM_BLOCKSIZES; i++) {
    smallblock[i].next = smallblock[i].prev = &smallblock[i];
    smallblock[i].blocksize = 1 << (MIN_BLOCKSIZE2+i);
  }
}

void *malloc(unsigned int size)
{
  // try small allocation first
  for (int i = 0; i < NUM_BLOCKSIZES; i++) {
    if (size <= smallblock[i].blocksize) {
      struct smallblock_info *elt, *head = &smallblock[i];
      int pieces_per_page = PAGE_SIZE / head->blocksize;

      // for each existing page of this blocksize
      for (elt = head->next; elt != head; elt = elt->next) {

	// for each of the blocks on this page (note: block 0 is reserved)
	for (int j = 1; j < pieces_per_page; j++) {

	  // if the block is free
	  if (bitmap_get(elt->bitmap, j) == 0) {

	    // then use that block
	    bitmap_set(elt->bitmap, j, 1);
	    void *pointer = (void *)elt + j * elt->blocksize;
	    return pointer;

	  }
	}
      }

      // there were no existing pages with free blocks
      void *p = alloc_pages(1);
      // first part of page is for accounting
      elt = p;
      elt->magic = 0xfeedface;
      elt->blocksize = head->blocksize;
      memset(elt->bitmap, 0, 16);
      // add to existing list
      elt->next = head;
      elt->prev = head->prev;
      elt->next->prev = elt;
      elt->prev->next = elt;
      // remainder of block is the actual data
      bitmap_set(elt->bitmap, 1, 1);
      void *pointer = p + 1 * elt->blocksize;
      return pointer;
    }
  }

  // resort to large allocation if it wasn't small
  size += sizeof(struct bigblock_info); // accounting overhead
  int n = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  void *p = alloc_pages(n);
  // first part of page is for accounting
  struct bigblock_info *elt = p;
  elt->magic = 0xf00dface;
  elt->pagecount = n;
  // remainder of page is the actual data
  void *pointer = (p + sizeof(struct bigblock_info));
  return pointer;
}

void *calloc(unsigned int count, unsigned int size)
{
  // round size up to multiples of 4, for compatibility with standard code
  size = (size + 3) & ~3;
  void *p = malloc(size * count);
  memset(p, 0, size * count);
  return p;
}

void free(void *pointer)
{
  // round down to nearest page boundary
  void *page = (void *)((unsigned int)pointer & ~(PAGE_SIZE-1));
  // some trivial sanity checks 
  if (page < (void *)0xC0000000) {
    printf("free: virtual address %p is too low to have come from malloc\n", pointer);
    shutdown();
  }
  // first word on page should be a magic number, second should be blocksize
  if (pointer - page < 8) {
    printf("free: virtual address %p is not aligned properly to have come from malloc\n", pointer);
    shutdown();
  }

  unsigned int *magic = page;
  if (*magic == 0xfeedface) {
    // small block
    struct smallblock_info *elt = page;
    // calculate index into blocks of page
    int idx = (pointer - page) / elt->blocksize;
    if (idx < 1 || pointer != (page + idx * elt->blocksize)) {
      printf("free: virtual address %p is not aligned properly to have come from malloc\n", pointer);
      shutdown();
    }
    if (bitmap_get(elt->bitmap, idx) == 0) {
      printf("free: virtual address %p was already freed, or has not been allocated\n", pointer);
      shutdown();
    }
    bitmap_set(elt->bitmap, idx, 0);
    // we could, if we want, free the whole page if all the blocks on the page
    // are empty, but we won't bother
  } else if (*magic == 0xf00dface) {
    // big block
    struct bigblock_info *elt = page;
    elt->magic = 0xdeadf00d; // erase the magic number
    free_pages(page, elt->pagecount);
  } else {
    printf("free: virtual address %p has bad magic (0x%x), either didn't come from malloc, was freed, or is corrupted\n", pointer, *magic);
    shutdown();
  }
}

void mem_init()
{
  for (int i = 0; i < 16; i++) {
    if (bootparams->devtable[i].type == DEV_TYPE_RAM) {
      ram_start_page = bootparams->devtable[i].start / PAGE_SIZE;
      ram_end_page = bootparams->devtable[i].end / PAGE_SIZE;
      ram_pages = ram_end_page - ram_start_page;
      page_alloc_init();
      malloc_init();
      return;
    }
  }
  puts("No RAM found?!?!");
  shutdown();
}
