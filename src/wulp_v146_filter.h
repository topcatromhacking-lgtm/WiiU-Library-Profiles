#pragma once
// WULP146 portable, read-only library filter. Binary per-title classification
// supplied from user's own installed catalog. Never copies Nintendo saves.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "wulp_v138_gate.h"
#define W146_MAX_CLASSIFIED 2048u
#define W146_ENTRY_BYTES 12u
#define W146_MAX_BYTES (16u + W146_MAX_CLASSIFIED*W146_ENTRY_BYTES)
#define W146_WIIU  0x01u
#define W146_WII   0x02u
#define W146_NDS   0x04u
#define W146_GBA   0x08u
#define W146_NES   0x10u
#define W146_SNES  0x20u
#define W146_N64   0x40u
#define W146_OTHER 0x80u
struct W146Entry{uint64_t id; uint8_t platform;};
static inline int w146_parse_catalog(const uint8_t *p,size_t n,
                                   W146Entry *out,unsigned *count){
    if(count)*count=0;
    if(!p||!out||!count||n<16u||n>W146_MAX_BYTES||
       memcmp(p,"WLPF",4)||p[4]!=1u||p[5]||p[6]||p[7])return 0;
    uint32_t c=w138_u32(p+8u);
    if(!c||c>W146_MAX_CLASSIFIED||n!=16u+(size_t)c*W146_ENTRY_BYTES||
       w138_crc(p,n-4u)!=w138_u32(p+n-4u))return 0;
    uint64_t prev=0;
    for(uint32_t i=0;i<c;++i){
       const uint8_t *e=p+12u+i*W146_ENTRY_BYTES;
       uint64_t id=w138_u64(e);
       uint8_t type=e[8];
       // Exactly one platform per game; sorted, no duplicate IDs.
       if(!id||(id>>32u)!=0x00050000u||(i&&id<=prev)||
          !type||(type&(type-1u))||e[9]||e[10]||e[11])return 0;
       out[i].id=id;out[i].platform=type;prev=id;
    }
    *count=c;return 1;
}
static inline uint8_t w146_classify(const W146Entry *entries,unsigned n,uint64_t id){
    unsigned lo=0,hi=n;
    while(lo<hi){unsigned mid=lo+(hi-lo)/2;
       if(entries[mid].id<id)lo=mid+1;else hi=mid;}
    return lo<n&&entries[lo].id==id?entries[lo].platform:0;
}
static inline int w146_parse_mask(const uint8_t *p,size_t n,uint8_t *mask){
    // Exactly "WULP146:1\nmask=0xAB\n" (no BOM, CRLF or hidden controls).
    static const char front[]="WULP146:1\nmask=0x";
    if(!p||!mask||n!=sizeof(front)-1u+3u||
       memcmp(p,front,sizeof(front)-1u)||p[n-1u]!='\n')return 0;
    unsigned acc=0;
    for(size_t i=sizeof(front)-1u;i<n-1u;i++){
        uint8_t c=p[i];unsigned v;
        if(c>='0'&&c<='9')v=c-'0';
        else if(c>='A'&&c<='F')v=c-'A'+10u;
        else return 0;
        acc=(acc<<4u)|v;
    }
    if(!acc)return 0;
    *mask=(uint8_t)acc;return 1;
}
// The filter file must classify every allowed *game* in either profile.
// Homebrew synthetic titles are protected and intentionally not catalogued.
static inline int w146_covers_profile(const W146Entry *entries,unsigned n,
                                      const uint64_t *allow,unsigned allowCount,
                                      unsigned *games){
    if(games)*games=0;
    if(!entries||!allow||!games||!n||!allowCount)return 0;
    for(unsigned i=0;i<allowCount;++i){
       uint32_t kind=(uint32_t)(allow[i]>>32u);
       if(kind==0x0005000fu)continue;
       if(kind!=0x00050000u||!w146_classify(entries,n,allow[i]))return 0;
       ++*games;
    }
    return 1;
}
static inline int w146_pass(uint64_t id,uint8_t mask,
                            const W146Entry *entries,unsigned n){
    const uint32_t kind=(uint32_t)(id>>32u);
    if(kind!=0x00050000u)return 1; // updates, system and HB unchanged
    const uint8_t group=w146_classify(entries,n,id);
    return group && (group&mask)!=0u;
}
static inline unsigned w146_visible_games(const uint64_t *allowed,unsigned allowedN,
                                  uint8_t mask,const W146Entry *entries,unsigned n){
    unsigned count=0;
    for(unsigned i=0;i<allowedN;i++)
       if((allowed[i]>>32u)==0x00050000u && w146_pass(allowed[i],mask,entries,n))++count;
    return count;
}
