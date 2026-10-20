// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define STEAL_PAGE_NUMBER 128

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock[NCPU];
  struct run *freelist[NCPU];
} kmem;

void 
getlockname(char *buf, int size, int n){
  snprintf(buf, size, "kmem%d", n);  
}

static char lockname[NCPU][8];

void
kinit()
{

  for(int i = 0; i < NCPU; i ++) {
    getlockname(lockname[i], sizeof(lockname[i]), i);
    initlock(&kmem.lock[i], lockname[i]);
  }
  
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
  
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int cid = cpuid();
  acquire(&kmem.lock[cid]);
  r->next = kmem.freelist[cid];
  kmem.freelist[cid] = r;
  release(&kmem.lock[cid]);
  pop_off();

  // acquire(&kmem.lock);
  // r->next = kmem.freelist;
  // kmem.freelist = r;
  // release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int cid = cpuid();
  int mycid = cid;

  int count = 0;

  while(count < NCPU) {
    acquire(&kmem.lock[cid]);

    if (kmem.freelist[cid]) {
      r = kmem.freelist[cid];

      if (mycid == cid) {
        kmem.freelist[cid] = r->next;
        release(&kmem.lock[cid]);
        pop_off();  
        memset((char*)r, 5, PGSIZE); // fill with junk
        return (void*)r;
      }

      struct run* head = r;

      for (int i = 0; i < STEAL_PAGE_NUMBER; i ++) {
        if(head->next) {
          head = head->next;
        } else {
          break;
        }
      }

      kmem.freelist[cid] = head->next;

      if (head == r) {
        release(&kmem.lock[cid]);
        memset((char*)head, 5, PGSIZE);
        pop_off();
        return (void*) head;
      } 

      head->next = 0;
      release(&kmem.lock[cid]);
      acquire(&kmem.lock[mycid]);
      kmem.freelist[mycid] = r->next;
      release(&kmem.lock[mycid]);
      memset((char*)r, 5, PGSIZE);
      pop_off();
      return (void*) r;
    }

    release(&kmem.lock[cid]);
    cid = (cid + 1) % NCPU;
    count++;
  }

  pop_off();
  return 0;
  
  // acquire(&kmem.lock);
  // r = kmem.freelist;
  // if(r)
  //   kmem.freelist = r->next;
  // release(&kmem.lock);

  // if(r)
  //   memset((char*)r, 5, PGSIZE); // fill with junk
  // return (void*)r;
}
