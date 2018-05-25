/*
 * Kernel Object
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
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

#pragma once

#include "mdb.hpp"
#include "refptr.hpp"

class Kobject : public Mdb
{
    public:
        // Also define the permission bit for that object in PD capability.
        enum Type : uint8 {
            PD = 0,
            EC,
            SC,
            PT,
            SM,
        };

        ALWAYS_INLINE
        inline Type type() const { return Type (objtype); }

    private:
        uint8 objtype;

        static void free (Rcu_elem *) {}

    protected:
        Spinlock lock;

        explicit Kobject (Type t, Space *s, mword b = 0, mword a = 0) : Mdb (s, reinterpret_cast<mword>(this), b, a, free), objtype (t) {}
};
