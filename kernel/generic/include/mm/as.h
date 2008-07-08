/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup genericmm
 * @{
 */
/** @file
 */

#ifndef KERN_AS_H_
#define KERN_AS_H_

/** Address space area flags. */
#define AS_AREA_READ		1
#define AS_AREA_WRITE		2
#define AS_AREA_EXEC		4
#define AS_AREA_CACHEABLE	8

#ifdef KERNEL

#include <arch/mm/page.h>
#include <arch/mm/as.h>
#include <arch/mm/asid.h>
#include <arch/types.h>
#include <synch/spinlock.h>
#include <synch/mutex.h>
#include <adt/list.h>
#include <adt/btree.h>
#include <lib/elf.h>

#ifdef __OBJC__
#include <lib/objc.h>
#endif

/**
 * Defined to be true if user address space and kernel address space shadow each
 * other.
 */
#define KERNEL_ADDRESS_SPACE_SHADOWED	KERNEL_ADDRESS_SPACE_SHADOWED_ARCH

#define KERNEL_ADDRESS_SPACE_START	KERNEL_ADDRESS_SPACE_START_ARCH
#define KERNEL_ADDRESS_SPACE_END	KERNEL_ADDRESS_SPACE_END_ARCH
#define USER_ADDRESS_SPACE_START	USER_ADDRESS_SPACE_START_ARCH
#define USER_ADDRESS_SPACE_END		USER_ADDRESS_SPACE_END_ARCH

#define USTACK_ADDRESS			USTACK_ADDRESS_ARCH

/** Kernel address space. */
#define FLAG_AS_KERNEL			(1 << 0)	

/* Address space area attributes. */
#define AS_AREA_ATTR_NONE	0
#define AS_AREA_ATTR_PARTIAL	1	/**< Not fully initialized area. */

/** The page fault was not resolved by as_page_fault(). */
#define AS_PF_FAULT		0
/** The page fault was resolved by as_page_fault(). */
#define AS_PF_OK		1
/** The page fault was caused by memcpy_from_uspace() or memcpy_to_uspace(). */
#define AS_PF_DEFER		2

#ifdef __OBJC__
@interface as_t : base_t {
	@public
		/** Protected by asidlock. */
		link_t inactive_as_with_asid_link;
		/**
		 * Number of processors on wich is this address space active.
		 * Protected by asidlock.
		 */
		count_t cpu_refcount;
		/**
		 * Address space identifier.
		 * Constant on architectures that do not support ASIDs.
		 * Protected by asidlock.  
		 */
		asid_t asid;
		
		/** Number of references (i.e tasks that reference this as). */
		atomic_t refcount;

		mutex_t lock;
		
		/** B+tree of address space areas. */
		btree_t as_area_btree;
		
		/** Non-generic content. */
		as_genarch_t genarch;
		
		/** Architecture specific content. */
		as_arch_t arch;
}

+ (pte_t *) page_table_create: (int) flags;
+ (void) page_table_destroy: (pte_t *) page_table;
- (void) page_table_lock: (bool) _lock;
- (void) page_table_unlock: (bool) unlock;

@end

#else

/** Address space structure.
 *
 * as_t contains the list of as_areas of userspace accessible
 * pages for one or more tasks. Ranges of kernel memory pages are not
 * supposed to figure in the list as they are shared by all tasks and
 * set up during system initialization.
 */
typedef struct as {
	/** Protected by asidlock. */
	link_t inactive_as_with_asid_link;
	/**
	 * Number of processors on wich is this address space active.
	 * Protected by asidlock.
	 */
	count_t cpu_refcount;
	/**
	 * Address space identifier.
	 * Constant on architectures that do not support ASIDs.
	 * Protected by asidlock.
	 */
	asid_t asid;

	/** Number of references (i.e tasks that reference this as). */
	atomic_t refcount;

	mutex_t lock;

	/** B+tree of address space areas. */
	btree_t as_area_btree;
	
	/** Non-generic content. */
	as_genarch_t genarch;

	/** Architecture specific content. */
	as_arch_t arch;
} as_t;

typedef struct {
	pte_t *(* page_table_create)(int flags);
	void (* page_table_destroy)(pte_t *page_table);
	void (* page_table_lock)(as_t *as, bool lock);
	void (* page_table_unlock)(as_t *as, bool unlock);
} as_operations_t;
#endif

/**
 * This structure contains information associated with the shared address space
 * area.
 */
typedef struct {
	/** This lock must be acquired only when the as_area lock is held. */
	mutex_t lock;		
	/** This structure can be deallocated if refcount drops to 0. */
	count_t refcount;
	/**
	 * B+tree containing complete map of anonymous pages of the shared area.
	 */
	btree_t pagemap;
} share_info_t;

/** Page fault access type. */
typedef enum {
	PF_ACCESS_READ,
	PF_ACCESS_WRITE,
	PF_ACCESS_EXEC
} pf_access_t;

struct mem_backend;

/** Backend data stored in address space area. */
typedef union mem_backend_data {
	struct {	/**< elf_backend members */
		elf_header_t *elf;
		elf_segment_header_t *segment;
	};
	struct {	/**< phys_backend members */
		uintptr_t base;
		count_t frames;
	};
} mem_backend_data_t;

/** Address space area structure.
 *
 * Each as_area_t structure describes one contiguous area of virtual memory.
 */
typedef struct {
	mutex_t lock;
	/** Containing address space. */
	as_t *as;		
	/**
	 * Flags related to the memory represented by the address space area.
	 */
	int flags;
	/** Attributes related to the address space area itself. */
	int attributes;
	/** Size of this area in multiples of PAGE_SIZE. */
	count_t pages;
	/** Base address of this area. */
	uintptr_t base;
	/** Map of used space. */
	btree_t used_space;

	/**
	 * If the address space area has been shared, this pointer will
	 * reference the share info structure.
	 */
	share_info_t *sh_info;

	/** Memory backend backing this address space area. */
	struct mem_backend *backend;

	/** Data to be used by the backend. */
	mem_backend_data_t backend_data;
} as_area_t;

/** Address space area backend structure. */
typedef struct mem_backend {
	int (* page_fault)(as_area_t *area, uintptr_t addr, pf_access_t access);
	void (* frame_free)(as_area_t *area, uintptr_t page, uintptr_t frame);
	void (* share)(as_area_t *area);
} mem_backend_t;

extern as_t *AS_KERNEL;

#ifndef __OBJC__
extern as_operations_t *as_operations;
#endif

extern link_t inactive_as_with_asid_head;

extern void as_init(void);

extern as_t *as_create(int flags);
extern void as_destroy(as_t *as);
extern void as_switch(as_t *old_as, as_t *new_as);
extern int as_page_fault(uintptr_t page, pf_access_t access, istate_t *istate);

extern as_area_t *as_area_create(as_t *as, int flags, size_t size,
    uintptr_t base, int attrs, mem_backend_t *backend,
    mem_backend_data_t *backend_data);
extern int as_area_destroy(as_t *as, uintptr_t address);	
extern int as_area_resize(as_t *as, uintptr_t address, size_t size, int flags);
int as_area_share(as_t *src_as, uintptr_t src_base, size_t acc_size,
    as_t *dst_as, uintptr_t dst_base, int dst_flags_mask);
extern int as_area_change_flags(as_t *as, int flags, uintptr_t address);

extern int as_area_get_flags(as_area_t *area);
extern bool as_area_check_access(as_area_t *area, pf_access_t access);
extern size_t as_area_get_size(uintptr_t base);
extern int used_space_insert(as_area_t *a, uintptr_t page, count_t count);
extern int used_space_remove(as_area_t *a, uintptr_t page, count_t count);


/* Interface to be implemented by architectures. */
#ifndef as_constructor_arch
extern int as_constructor_arch(as_t *as, int flags);
#endif /* !def as_constructor_arch */
#ifndef as_destructor_arch
extern int as_destructor_arch(as_t *as);
#endif /* !def as_destructor_arch */
#ifndef as_create_arch
extern int as_create_arch(as_t *as, int flags);
#endif /* !def as_create_arch */
#ifndef as_install_arch
extern void as_install_arch(as_t *as);
#endif /* !def as_install_arch */
#ifndef as_deinstall_arch
extern void as_deinstall_arch(as_t *as);
#endif /* !def as_deinstall_arch */

/* Backend declarations and functions. */
extern mem_backend_t anon_backend;
extern mem_backend_t elf_backend;
extern mem_backend_t phys_backend;

/** 
 * This flags is passed when running the loader, otherwise elf_load()
 * would return with a EE_LOADER error code.
 */
#define ELD_F_NONE	0
#define ELD_F_LOADER	1

extern unsigned int elf_load(elf_header_t *header, as_t *as, int flags);

/* Address space area related syscalls. */
extern unative_t sys_as_area_create(uintptr_t address, size_t size, int flags);
extern unative_t sys_as_area_resize(uintptr_t address, size_t size, int flags);
extern unative_t sys_as_area_change_flags(uintptr_t address, int flags);
extern unative_t sys_as_area_destroy(uintptr_t address);

/* Introspection functions. */
extern void as_print(as_t *as);

#endif /* KERNEL */

#endif

/** @}
 */
