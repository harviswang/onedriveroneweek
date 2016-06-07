struct page *mem_map;
/*
 * A number of key systems in x86 including ioremap() rely on the assumption
 * that high_memory defines the upper bound on direct map memory, then end
 * of ZONE_NORMAL.  Under CONFIG_DISCONTIG this means that max_low_pfn and
 * highstart_pfn must be the same; there must be no gap between ZONE_NORMAL
 * and ZONE_HIGHMEM.
 */
void * high_memory;

#define _CACHE_SHIFT        (9)
#define _CACHE_UNCACHED         (2<<_CACHE_SHIFT)  /* R4[0246]00      */

typedef unsigned long phys_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;

# define cpu_has_64bit_addresses    0
# define __iomem
#define _ATYPE_     __PTRDIFF_TYPE__
#define _ATYPE32_   int
#define _ACAST32_       (_ATYPE_)(_ATYPE32_)

/*
 * Returns the physical address of a CKSEGx / XKPHYS address
 */
#define CPHYSADDR(a)        ((_ACAST32_(a)) & 0x1fffffff)
#define KSEG1           0xa0000000
#define CKSEG1ADDR(a)       (CPHYSADDR(a) | KSEG1)

#define __AC(X,Y)   (X##Y)
#define _AC(X,Y)    __AC(X,Y)
#define CAC_BASE        _AC(0x80000000, UL)
#define PHYS_OFFSET     _AC(0, UL)
#define PAGE_OFFSET     (CAC_BASE + PHYS_OFFSET)
#define PAGE_SIZE   (_AC(1,UL) << PAGE_SHIFT)
#define PAGE_MASK   (~((1 << PAGE_SHIFT) - 1))
#define __pa(x)                             \
    ((unsigned long)(x) - PAGE_OFFSET + PHYS_OFFSET)
#define __va(x)     ((void *)((unsigned long)(x) + PAGE_OFFSET - PHYS_OFFSET))

/*
 * It's normally defined only for FLATMEM config but it's
 * used in our early mem init code for all memory models.
 * So always define it.
 */
#define ARCH_PFN_OFFSET     PFN_UP(PHYS_OFFSET)
#define __pfn_to_page(pfn)  (mem_map + ((pfn) - ARCH_PFN_OFFSET))
#define pfn_to_page __pfn_to_page
#define PAGE_SHIFT  12
#define PFN_UP(x)   (((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x) ((x) >> PAGE_SHIFT)
#define virt_to_page(kaddr) pfn_to_page(PFN_DOWN(virt_to_phys(kaddr)))

/*
 * 'kernel.h' contains some often-used function prototypes etc
 */
#define __ALIGN_KERNEL(x, a)        __ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)    (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)     __ALIGN_KERNEL((x), (a))
/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr) ALIGN(addr, PAGE_SIZE)

/*
 * Macros to create function definitions for page flags
 */
#define TESTPAGEFLAG(uname, lname)                  \
static inline int Page##uname(const struct page *page)          \
            { return test_bit(PG_##lname, &page->flags); }
#define SETPAGEFLAG(uname, lname)                   \
static inline void SetPage##uname(struct page *page)            \
            { set_bit(PG_##lname, &page->flags); }

#define CLEARPAGEFLAG(uname, lname)                 \
static inline void ClearPage##uname(struct page *page)          \
            { clear_bit(PG_##lname, &page->flags); }
#define __CLEARPAGEFLAG(uname, lname)                   \
static inline void __ClearPage##uname(struct page *page)        \
            { __clear_bit(PG_##lname, &page->flags); }
#define PAGEFLAG(uname, lname) TESTPAGEFLAG(uname, lname)       \
    SETPAGEFLAG(uname, lname) CLEARPAGEFLAG(uname, lname)
PAGEFLAG(Reserved, reserved) __CLEARPAGEFLAG(Reserved, reserved)

/*
 *     virt_to_phys    -       map virtual addresses to physical
 *     @address: address to remap
 *
 *     The returned physical address is the physical (CPU) mapping for
 *     the memory address given. It is only valid to use this function on
 *     addresses directly mapped or allocated via kmalloc.
 *
 *     This function does not give bus mappings for DMA transfers. In
 *     almost all conceivable cases a device driver should not be using
 *     this function
 */
static inline unsigned long virt_to_phys(volatile const void *address)
{
    return __pa(address);
}

/*
 *     phys_to_virt    -       map physical address to virtual
 *     @address: address to remap
 *
 *     The returned virtual address is a current CPU mapping for
 *     the memory address given. It is only valid to use this function on
 *     addresses that have a kernel mapping
 *
 *     This function does not handle bus mappings for DMA transfers. In
 *     almost all conceivable cases a device driver should not be using
 *     this function
 */
static inline void * phys_to_virt(unsigned long address)
{
    return (void *)(address + PAGE_OFFSET - PHYS_OFFSET);
}

static inline void __iomem *plat_ioremap(phys_t offset, unsigned long size,
    unsigned long flags)
{
    return NULL;
}

/*
 * Remap an arbitrary physical address space into the kernel virtual
 * address space. Needed when the kernel wants to access high addresses
 * directly.
 *
 * NOTE! We need to allow non-page-aligned mappings too: we will obviously
 * have to convert them into an offset in a page-aligned mapping, but the
 * caller shouldn't need to know that small detail.
 */

#define IS_LOW512(addr) (!((phys_t)(addr) & (phys_t) ~0x1fffffffULL))

void __iomem * __ioremap(phys_t phys_addr, phys_t size, unsigned long flags)
{
    struct vm_struct * area;
    unsigned long offset;
    phys_t last_addr;
    void * addr;

    phys_addr = fixup_bigphys_addr(phys_addr, size);

    /* Don't allow wraparound or zero size */
    last_addr = phys_addr + size - 1;
    if (!size || last_addr < phys_addr)
        return NULL;

    /*
     * Map uncached objects in the low 512mb of address space using KSEG1,
     * otherwise map using page tables.
     */
    if (IS_LOW512(phys_addr) && IS_LOW512(last_addr) &&
        flags == _CACHE_UNCACHED)
        return (void __iomem *) CKSEG1ADDR(phys_addr);

    /*
     * Don't allow anybody to remap normal RAM that we're using..
     */
    if (phys_addr < virt_to_phys(high_memory)) {
        char *t_addr, *t_end;
        struct page *page;

        t_addr = __va(phys_addr);
        t_end = t_addr + (size - 1);

        for(page = virt_to_page(t_addr); page <= virt_to_page(t_end); page++)
            if(!PageReserved(page))
                return NULL;
    }

    /*
     * Mappings have to be page-aligned
     */
    offset = phys_addr & ~PAGE_MASK;
    phys_addr &= PAGE_MASK;
    size = PAGE_ALIGN(last_addr + 1) - phys_addr;

    /*
     * Ok, go for it..
     */
    area = get_vm_area(size, VM_IOREMAP);
    if (!area)
        return NULL;
    addr = area->addr;
    if (remap_area_pages((unsigned long) addr, phys_addr, size, flags)) {
        vunmap(addr);
        return NULL;
    }

    return (void __iomem *) (offset + (char *)addr);
}

/*
 * Allow physical addresses to be fixed up to help peripherals located
 * outside the low 32-bit range -- generic pass-through version.
 */
static inline phys_t fixup_bigphys_addr(phys_t phys_addr, phys_t size)
{
    return phys_addr;
}

static inline void __iomem * __ioremap_mode(phys_t offset, unsigned long size,
    unsigned long flags)
{
    void __iomem *addr = plat_ioremap(offset, size, flags);

    if (addr)
        return addr;

#define __IS_LOW512(addr) (!((phys_t)(addr) & (phys_t) ~0x1fffffffULL))

    if (cpu_has_64bit_addresses) {
        u64 base = UNCAC_BASE;

        /*
         * R10000 supports a 2 bit uncached attribute therefore
         * UNCAC_BASE may not equal IO_BASE.
         */
        if (flags == _CACHE_UNCACHED)
            base = (u64) IO_BASE;
        return (void __iomem *) (unsigned long) (base + offset);
    } else if (__builtin_constant_p(offset) &&
           __builtin_constant_p(size) && __builtin_constant_p(flags)) {
        phys_t phys_addr, last_addr;

        phys_addr = fixup_bigphys_addr(offset, size);

        /* Don't allow wraparound or zero size. */
        last_addr = phys_addr + size - 1;
        if (!size || last_addr < phys_addr)
            return NULL;

        /*
         * Map uncached objects in the low 512MB of address
         * space using KSEG1.
         */
        if (__IS_LOW512(phys_addr) && __IS_LOW512(last_addr) &&
            flags == _CACHE_UNCACHED)
            return (void __iomem *)
                (unsigned long)CKSEG1ADDR(phys_addr);
    }

    return __ioremap(offset, size, flags);

#undef __IS_LOW512
}

/*
 * ioremap     -   map bus memory into CPU space
 * @offset:    bus address of the memory
 * @size:      size of the resource to map
 *
 * ioremap performs a platform specific sequence of operations to
 * make bus memory CPU accessible via the readb/readw/readl/writeb/
 * writew/writel functions and the other mmio helpers. The returned
 * address is not guaranteed to be usable directly as a virtual
 * address.
 */
#define ioremap(offset, size)                       \
    __ioremap_mode((offset), (size), _CACHE_UNCACHED)

