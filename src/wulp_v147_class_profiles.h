#pragma once
// WULP147. Validated platform subsets of the existing 277 game profile.
// Pure checking logic; never writes console saves.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "wulp_v138_gate.h"
#include "wulp_v146_filter.h"
#define W147_ROOT_START 315u
#define W147_ROOT_END 600u
#define W147_GAMES 277u
#define W147_HOMEBREW 7u
#define W147_MAX_DRIFT 32u
#define W147_META_START 9600u
#define W147_META_END 11556u
#define W147_MAX_WLPA 2320u
#define W147_PAIR_MAX 440u
static inline unsigned w147_count_for_mask(unsigned mask){
    return ((mask&1u)?140u:0u)+((mask&2u)?32u:0u)+
           ((mask&4u)?31u:0u)+((mask&8u)?74u:0u);
}
static inline bool w147_catalog_matches_parent(const uint8_t *base,size_t bn,
                const W146Entry *entries,unsigned count){
    if(!base||bn!=W138_BARISTA||!entries||count!=W147_GAMES)return false;
    unsigned seen=0,classes[4]={};
    for(unsigned slot=W147_ROOT_START;slot<W147_ROOT_END;slot++){
        uint64_t id=w138_u64(base+slot*16u);
        if(!id)continue;
        if((id>>32u)!=0x00050000u||++seen>W147_GAMES)return false;
        uint8_t klass=w146_classify(entries,count,id);
        if(klass!=1u && klass!=2u && klass!=4u && klass!=8u)return false;
        classes[(klass==1u)?0u:(klass==2u)?1u:(klass==4u)?2u:3u]++;
    }
    if(seen!=W147_GAMES || classes[0]!=140u||classes[1]!=32u||
       classes[2]!=31u||classes[3]!=74u)return false;
    for(unsigned i=0;i<count;i++){
        bool found=false;
        for(unsigned slot=W147_ROOT_START;slot<W147_ROOT_END;slot++)
            if(w138_u64(base+slot*16u)==entries[i].id){found=true;break;}
        if(!found)return false;
    }
    return true;
}
static inline bool w147_pair_gate(const uint8_t *base,size_t bn,
                 const uint8_t *catalogBytes,size_t catalogLen,
                 const char *marker,size_t mn,unsigned mask,
                 const uint8_t *gold,size_t gn,const uint8_t *wlpa,size_t wn,
                 uint64_t output[284],unsigned *outCount){
    if(outCount)*outCount=0;
    if(!base||!catalogBytes||!marker||!gold||!wlpa||!output||!outCount||
       bn!=W138_BARISTA||gn!=W138_BARISTA||mask>14u||mn>W147_PAIR_MAX||
       wn>W147_MAX_WLPA || wn<48u) return false;
    if(catalogLen!=16u+W147_GAMES*W146_ENTRY_BYTES)return false;
    W146Entry entries[W147_GAMES];unsigned nc=0;
    if(!w146_parse_catalog(catalogBytes,catalogLen,entries,&nc)||
       !w147_catalog_matches_parent(base,bn,entries,nc))return false;
    const unsigned games=w147_count_for_mask(mask);
    const unsigned n=games+W147_HOMEBREW;
    if(wn!=44u+n*8u+4u)return false;
    uint8_t ah[32],ch[32],gh[32],wh[32];
    char as[65],cs[65],gs[65],ws[65],expected[W147_PAIR_MAX+1];
    w138_sha(base,bn,ah);w138_sha(catalogBytes,catalogLen,ch);
    w138_sha(gold,gn,gh);w138_sha(wlpa,wn,wh);
    w138_hex(ah,as);w138_hex(ch,cs);w138_hex(gh,gs);w138_hex(wh,ws);
    const int length=snprintf(expected,sizeof(expected),
      "WULP147:1\nmask=%02X\nparent_sha256=%s\ncatalog_sha256=%s\nbarista_sha256=%s\nwlpa_sha256=%s\n",
      mask,as,cs,gs,ws);
    if(length<0||(size_t)length>=sizeof(expected)||mn!=(size_t)length||
       memcmp(marker,expected,mn))return false;
    if(memcmp(base,gold,W147_ROOT_START*16u)||
       memcmp(base+W147_META_START,gold+W147_META_START,W138_BARISTA-W147_META_START))return false;
    for(unsigned i=0;i<W147_ROOT_END;i++)
        if(memcmp(base+i*16u+8u,gold+i*16u+8u,8u))return false;
    unsigned position=0;
    for(unsigned slot=W147_ROOT_START;slot<W147_ROOT_END;slot++){
        uint64_t id=w138_u64(base+slot*16u);
        if(!id)continue;
        const uint8_t cls=w146_classify(entries,nc,id);
        if(!(cls&mask))continue;
        if(position>=games||w138_u64(gold+(W147_ROOT_START+position)*16u)!=id)return false;
        ++position;
    }
    if(position!=games)return false;
    for(unsigned slot=W147_ROOT_START+games;slot<W147_ROOT_END;slot++)
        if(w138_u64(gold+slot*16u))return false;
    if(memcmp(wlpa,"WLPA",4)||wlpa[4]||wlpa[5]!=1u||wlpa[6]||wlpa[7]||
       w138_u32(wlpa+8u)!=n||memcmp(wlpa+12u,gh,32)||
       w138_crc(wlpa,wn-4u)!=w138_u32(wlpa+wn-4u))return false;
    uint64_t previous=0;unsigned matchedGames=0,matchedHb=0;
    for(unsigned i=0;i<n;i++){
       uint64_t id=w138_u64(wlpa+44u+i*8u);
       if(!id||(i&&id<=previous))return false;
       previous=id;bool ok=false;
       if((id>>32u)==0x00050000u){
           for(unsigned k=0;k<games;k++)
               if(id==w138_u64(gold+(W147_ROOT_START+k)*16u)){ok=true;break;}
           if(ok)matchedGames++;
       }else if((id>>32u)==0x0005000fu){
           for(unsigned k=8;k<15;k++)
               if(id==w138_u64(base+k*16u)){ok=true;break;}
           if(ok)matchedHb++;
       }
       if(!ok)return false;
       output[i]=id;
    }
    if(matchedGames!=games||matchedHb!=W147_HOMEBREW)return false;
    *outCount=n;return true;
}
static inline bool w147_working_gate(const uint8_t *gold,size_t gn,
                                     const uint8_t *working,size_t wn){
    if(!gold||!working||gn!=W138_BARISTA||wn!=W138_BARISTA)return false;
    if(memcmp(gold,working,W147_META_START)||
       memcmp(gold+W147_META_END,working+W147_META_END,W138_BARISTA-W147_META_END))return false;
    unsigned drift=0;
    for(unsigned i=W147_META_START;i<W147_META_END;i++)
        if(gold[i]!=working[i]&&++drift>W147_MAX_DRIFT)return false;
    return true;
}
