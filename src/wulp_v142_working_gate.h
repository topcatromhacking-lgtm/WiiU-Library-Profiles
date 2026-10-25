#pragma once
// WULP142 portable working-save validator.
// Keeps every protected root slot, title type/flags and all 60 folder banks
// unchanged; permits ONE observed class of Menu mutation: one byte in the
// 1956-byte opaque root metadata area. The actual offset is deliberately
// not assumed from the prior 141-game validation report.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "wulp_v140_working_gate.h"

#define W142_OPAQUE_START 9600u
#define W142_OPAQUE_END   11556u

// 0 = not trusted, 1 = exact metadata, 2 = exactly one opaque byte drift.
static inline int w142_working_gate(const uint8_t *golden,size_t gn,
                                   const uint8_t *working,size_t wn,
                                   const uint64_t allowed[284]) {
    if (!golden || !working || !allowed || gn!=W140_BARISTA_SIZE ||
        wn!=W140_BARISTA_SIZE) return 0;
    // Root slots 0..314 protect original icons (including the first 15).
    if (memcmp(golden,working,315u*16u)) return 0;
    // Folder tables are NOT inferred from white/empty UI pages; protect them.
    if (memcmp(golden+W142_OPAQUE_END,working+W142_OPAQUE_END,
               W140_BARISTA_SIZE-W142_OPAQUE_END)) return 0;
    unsigned drift=0;
    for (size_t i=W142_OPAQUE_START;i<W142_OPAQUE_END;i++) {
        if (golden[i]!=working[i] && ++drift>1u) return 0;
    }
    unsigned count=0;
    for (unsigned i=315u;i<600u;i++) {
        const uint8_t *record=working+i*16u;
        if (memcmp(record+8u,golden+i*16u+8u,8u)) return 0;
        const uint64_t id=w140_u64(record);
        if (!id) continue;
        if ((id>>32u)!=0x00050000u || !w140_in_sorted_allowlist(allowed,id))
            return 0;
        for (unsigned j=315u;j<i;j++)
            if (id==w140_u64(working+j*16u)) return 0;
        ++count;
    }
    return count==277u ? (drift?2:1) : 0;
}
