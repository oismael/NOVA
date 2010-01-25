/*
 * Memory Space
 *
 * Copyright (C) 2007-2010, Udo Steinberg <udo@hypervisor.org>
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#include "memory.h"
#include "mtrr.h"
#include "pd.h"
#include "regs.h"
#include "space_mem.h"
#include "vma.h"

unsigned Space_mem::did_ctr;

void Space_mem::init (unsigned cpu)
{
    if (!percpu[cpu]) {

        // Create ptab for this CPU
        percpu[cpu] = new Ptab;

        // Sync cpu-local range
        percpu[cpu]->sync_from (Pd::kern.cpu_ptab (cpu), LOCAL_SADDR);

        // Sync kernel code and data
        percpu[cpu]->sync_master_range (LINK_ADDR, LOCAL_SADDR);

        trace (TRACE_MEMORY, "PD:%p PTAB[%u]:%#lx", static_cast<Pd *>(this), cpu, Buddy::ptr_to_phys (percpu[cpu]));
    }
}

void Space_mem::insert_root (mword base, size_t size)
{
    for (size_t frag; size; size -= frag, base += frag) {

        unsigned type = Mtrr::memtype (base);

        for (frag = 0; frag < size; frag += PAGE_SIZE)
            if (Mtrr::memtype (base + frag) != type)
                break;

        insert_root (base, frag, type, 7);
    }
}

void Space_mem::insert_root (mword b, size_t s, unsigned t, unsigned a)
{
    for (long int o; s; s -= 1ul << o, b += 1ul << o) {

        o = bit_scan_reverse (s);
        if (b)
            o = min (bit_scan_forward (b), o);

        vma_head.create_child (&vma_head, 0, b, o, t, a);
    }
}

bool Space_mem::insert (Vma *vma, Paddr phys)
{
    unsigned o = vma->order - PAGE_BITS;

    if (dpt) {
        unsigned ord = min (o, Dpt::ord);
        for (unsigned i = 0; i < 1u << (o - ord); i++)
            dpt->insert (vma->base + i * (1ul << (Dpt::ord + PAGE_BITS)), ord, phys + i * (1ul << (Dpt::ord + PAGE_BITS)), vma->attr);
    }

    if (ept) {
        unsigned ord = min (o, Ept::ord);
        for (unsigned i = 0; i < 1u << (o - ord); i++)
            ept->insert (vma->base + i * (1ul << (Ept::ord + PAGE_BITS)), ord, phys + i * (1ul << (Ept::ord + PAGE_BITS)), vma->attr, vma->type);
    }

    Ptab::Attribute a = Ptab::Attribute (Ptab::ATTR_USER |
                      (vma->attr & 0x4 ? Ptab::ATTR_NONE     : Ptab::ATTR_NOEXEC) |
                      (vma->attr & 0x2 ? Ptab::ATTR_WRITABLE : Ptab::ATTR_NONE));

    // Whoever owns a VMA struct in the VMA list owns the respective PT slots
    mst->insert (vma->base, o, a, phys);

    return true;
}
