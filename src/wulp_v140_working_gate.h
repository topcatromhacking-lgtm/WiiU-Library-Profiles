#pragma once
/* WULP140: validate a writable Barista against its immutable golden
 * candidate. Only reordering within the 285 USB root slots is currently
 * supported. The immutable candidate and WLPA must first pass w138_gate.
 * No filesystem calls, no writes, no owner data.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define W140_BARISTA_SIZE 130116u
#define W140_USB_START 315u
#define W140_ROOT_END 600u
static inline uint64_t w140_u64(const uint8_t *p) {
    uint64_t n=0;
    for(unsigned j=0;j<8u;j++)n=(n<<8u)|p[j];
    return n;
}
static inline bool w140_in_sorted_allowlist(const uint64_t allowed[284],uint64_t id) {
    unsigned lo=0,hi=284;
    while(lo<hi){
        unsigned mid=lo+(hi-lo)/2u;
        if(allowed[mid]<id)lo=mid+1u;else hi=mid;
    }
    return lo<284u && allowed[lo]==id;
}
static inline bool w140_working_gate(const uint8_t *golden,size_t gn,
                                     const uint8_t *working,size_t wn,
                                     const uint64_t allowed[284]) {
    if(!golden||!working||!allowed||gn!=W140_BARISTA_SIZE||
       wn!=W140_BARISTA_SIZE)return false;
    // Root slots 0..314 include the fifteen protected icons.
    if(memcmp(golden,working,W140_USB_START*16u))return false;
    // Opaque root metadata and all 60 folder banks are read-only in beta.
    if(memcmp(golden+W140_ROOT_END*16u,working+W140_ROOT_END*16u,
              W140_BARISTA_SIZE-W140_ROOT_END*16u))return false;
    unsigned found=0;
    for(unsigned i=W140_USB_START;i<W140_ROOT_END;i++){
        const uint8_t *now=working+i*16u;
        const uint8_t *old=golden+i*16u;
        if(memcmp(now+8u,old+8u,8u))return false;
        uint64_t id=w140_u64(now);
        if(!id)continue;
        if((id>>32u)!=0x00050000u||!w140_in_sorted_allowlist(allowed,id))return false;
        for(unsigned j=W140_USB_START;j<i;j++)
            if(id==w140_u64(working+j*16u))return false;
        found++;
    }
    return found==277u;
}
