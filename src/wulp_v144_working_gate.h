#pragma once
// WULP144: narrow, bounded metadata drift across multiple cold boots.
// This is NOT a Barista repair routine: validates a read-only snapshot.
// All 277 game IDs and all layout records must still validate exactly.
// Only an accumulating small number of mutations in an opaque metadata
// byte range is tolerated. The threshold is deliberately finite.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "wulp_v140_working_gate.h"

#define W144_OPAQUE_START 9600u
#define W144_OPAQUE_END 11556u
#define W144_MAX_OPAQUE_CHANGED_BYTES 32u

// 0 = rejected; 1 = no metadata drift; 2 = bounded drift.
static inline int w144_working_gate(const uint8_t *golden,size_t gn,
                                   const uint8_t *working,size_t wn,
                                   const uint64_t allowed[284],
                                   unsigned *changed_opaque) {
    if (changed_opaque) *changed_opaque=0;
    if (!golden || !working || !allowed || gn!=W140_BARISTA_SIZE ||
        wn!=W140_BARISTA_SIZE) return 0;
    // Protect all original NAND and 15 first-page HB icons.
    if (memcmp(golden,working,315u*16u)) return 0;
    // Do not accept changed folder contents, page/folder tables or footer.
    if (memcmp(golden+W144_OPAQUE_END,working+W144_OPAQUE_END,
               W140_BARISTA_SIZE-W144_OPAQUE_END)) return 0;
    unsigned changes=0;
    for(size_t i=W144_OPAQUE_START;i<W144_OPAQUE_END;i++) {
        if (golden[i]!=working[i] && ++changes>W144_MAX_OPAQUE_CHANGED_BYTES)
            return 0;
    }
    unsigned found=0;
    for(unsigned slot=315u;slot<600u;slot++) {
        const uint8_t *record=working+slot*16u;
        if (memcmp(record+8u,golden+slot*16u+8u,8u)) return 0;
        uint64_t id=w140_u64(record);
        if(!id) continue;
        if ((id>>32u)!=0x00050000u ||
            !w140_in_sorted_allowlist(allowed,id)) return 0;
        for(unsigned previous=315u;previous<slot;previous++)
            if (id==w140_u64(working+previous*16u)) return 0;
        found++;
    }
    if(found!=277u) return 0;
    if(changed_opaque) *changed_opaque=changes;
    return changes ? 2 : 1;
}
