#pragma once
// WULP148 - separately signed 278-title alphabetical class profiles.
// Console verification is read-only and uses the proven 277 parent + 141 original.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "wulp_v138_gate.h"
#include "wulp_v146_filter.h"
#define W148_WONDER UINT64_C(0x000500001012DC00)
#define W148_GAMES 278u
#define W148_ORIGINAL 141u
#define W148_PARENT 277u
#define W148_HB 7u
#define W148_MAX_WLPA (44u + (W148_GAMES+W148_HB)*8u + 4u)
#define W148_ORDER_LEN (12u+W148_GAMES*8u+4u)
#define W148_META_BEGIN 9600u
#define W148_META_END 11556u
#define W148_ALLOWED_DRIFT 32u
#define W148_MARKER_MAX 600u

static inline unsigned w148_mask_count(unsigned mask){
    return ((mask&1u)?141u:0u)+((mask&2u)?32u:0u)+
           ((mask&4u)?31u:0u)+((mask&8u)?74u:0u);
}
static inline int w148_source_gate(const uint8_t *base,size_t bn,
                     const uint8_t *original,size_t on,
                     const uint8_t *oldCat,size_t oldLen,
                     const W146Entry *newEntries,unsigned newN){
    if(!base||!original||!oldCat||!newEntries||bn!=W138_BARISTA||on!=W138_BARISTA||
       memcmp(base,original,315u*16u)!=0||newN!=W148_GAMES||
       oldLen!=16u+W148_PARENT*W146_ENTRY_BYTES)return 0;
    W146Entry previous[W148_PARENT];unsigned oldN=0;
    if(!w146_parse_catalog(oldCat,oldLen,previous,&oldN)||oldN!=W148_PARENT)return 0;
    unsigned parentN=0,originalN=0,cls[4]={};int extra=0;
    for(unsigned i=315u;i<600u;i++){
        uint64_t a=w138_u64(base+i*16u),b=w138_u64(original+i*16u);
        if(a){ if((a>>32u)!=0x00050000u || ++parentN>W148_PARENT ||
                 !w146_classify(previous,oldN,a))return 0; }
        if(b){ if((b>>32u)!=0x00050000u || ++originalN>W148_ORIGINAL)return 0; }
    }
    if(parentN!=W148_PARENT||originalN!=W148_ORIGINAL)return 0;
    for(unsigned i=0;i<newN;i++){
        uint64_t id=newEntries[i].id;
        uint8_t kind=newEntries[i].platform;
        if(kind!=1u && kind!=2u && kind!=4u && kind!=8u)return 0;
        cls[kind==1u?0:kind==2u?1:kind==4u?2:3]++;
        if(id==W148_WONDER){
            if(kind!=1u||w146_classify(previous,oldN,id)||extra++)return 0;
            unsigned found=0;
            for(unsigned j=315;j<600;j++)found+=w138_u64(original+j*16u)==id;
            if(found!=1)return 0;
        }else{
            if(w146_classify(previous,oldN,id)!=kind)return 0;
        }
    }
    if(extra!=1||cls[0]!=141u||cls[1]!=32u||cls[2]!=31u||cls[3]!=74u)return 0;
    for(unsigned i=0;i<oldN;i++)
        if(w146_classify(newEntries,newN,previous[i].id)!=previous[i].platform)return 0;
    return 1;
}
static inline int w148_order_gate(const uint8_t *order,size_t len,
                                  const W146Entry *entries,unsigned n,
                                  uint64_t out[W148_GAMES]){
    if(!order||!entries||!out||n!=W148_GAMES||len!=W148_ORDER_LEN||
       memcmp(order,"WLO1",4)||w138_u32(order+4)!=W148_GAMES||
       w138_u32(order+8)||w138_crc(order,len-4)!=w138_u32(order+len-4))return 0;
    unsigned prevRank=0;
    for(unsigned i=0;i<W148_GAMES;i++){
        uint64_t id=w138_u64(order+12+i*8u);
        uint8_t group=w146_classify(entries,n,id);
        if(!group ||(i && group<prevRank))return 0;
        for(unsigned j=0;j<i;j++)if(out[j]==id)return 0;
        out[i]=id;prevRank=group;
    }
    return 1;
}
static inline int w148_pair_gate(const uint8_t *base,size_t bn,
                   const uint8_t *original,size_t on,
                   const uint8_t *oldCat,size_t oldLen,
                   const uint8_t *newCat,size_t catLen,
                   const uint8_t *order,size_t orderLen,
                   const char *marker,size_t mn,unsigned mask,
                   const uint8_t *gold,size_t gn,
                   const uint8_t *wlpa,size_t wn,
                   uint64_t allowed[W148_GAMES+W148_HB],unsigned *outCount){
    if(outCount)*outCount=0;
    if(!base||!original||!oldCat||!newCat||!order||!marker||!gold||
       !wlpa||!allowed||!outCount||mask>15u||gn!=W138_BARISTA||
       mn>W148_MARKER_MAX||catLen!=16u+W148_GAMES*W146_ENTRY_BYTES||
       orderLen!=W148_ORDER_LEN)return 0;
    W146Entry entries[W148_GAMES];unsigned entriesN=0;
    if(!w146_parse_catalog(newCat,catLen,entries,&entriesN)||
       !w148_source_gate(base,bn,original,on,oldCat,oldLen,entries,entriesN))return 0;
    uint64_t orderIds[W148_GAMES]={};
    if(!w148_order_gate(order,orderLen,entries,entriesN,orderIds))return 0;
    const unsigned games=w148_mask_count(mask),count=games+W148_HB;
    if(wn!=44u+count*8u+4u||wn>W148_MAX_WLPA)return 0;
    uint8_t bh[32],oh[32],ch[32],ih[32],gh[32],wh[32];
    char bs[65],os[65],cs[65],is[65],gs[65],ws[65],expected[W148_MARKER_MAX+1];
    w138_sha(base,bn,bh);w138_sha(original,on,oh);
    w138_sha(newCat,catLen,ch);w138_sha(order,orderLen,ih);
    w138_sha(gold,gn,gh);w138_sha(wlpa,wn,wh);
    w138_hex(bh,bs);w138_hex(oh,os);w138_hex(ch,cs);
    w138_hex(ih,is);w138_hex(gh,gs);w138_hex(wh,ws);
    const int got=snprintf(expected,sizeof(expected),
        "WULP148:1\nmask=%02X\nparent_sha256=%s\noriginal_sha256=%s\n"
        "catalog_sha256=%s\norder_sha256=%s\nbarista_sha256=%s\nwlpa_sha256=%s\n",
        mask,bs,os,cs,is,gs,ws);
    if(got<0||(size_t)got>=sizeof(expected)||(size_t)got!=mn||memcmp(expected,marker,mn))return 0;
    if(memcmp(base,gold,315u*16u)||
       memcmp(base+W148_META_BEGIN,gold+W148_META_BEGIN,W138_BARISTA-W148_META_BEGIN))return 0;
    for(unsigned i=0;i<600u;i++)
        if(memcmp(base+i*16u+8u,gold+i*16u+8u,8u))return 0;
    unsigned at=0;
    for(unsigned i=0;i<W148_GAMES;i++){
        uint64_t id=orderIds[i];
        if(!(w146_classify(entries,entriesN,id)&mask))continue;
        if(at>=games||w138_u64(gold+(315u+at)*16u)!=id)return 0;
        ++at;
    }
    if(at!=games)return 0;
    for(unsigned i=315u+games;i<600u;i++)if(w138_u64(gold+i*16u))return 0;
    if(memcmp(wlpa,"WLPA",4)||wlpa[4]||wlpa[5]!=1u||wlpa[6]||wlpa[7]||
       w138_u32(wlpa+8)!=count||memcmp(wlpa+12,gh,32)||
       w138_crc(wlpa,wn-4)!=w138_u32(wlpa+wn-4))return 0;
    unsigned wg=0,hb=0;uint64_t prev=0;
    for(unsigned i=0;i<count;i++){
       uint64_t id=w138_u64(wlpa+44+i*8u);
       if(!id||(i&&id<=prev))return 0;
       prev=id;int found=0;
       if((id>>32u)==0x00050000u){
           for(unsigned j=315u;j<315u+games;j++)
               if(w138_u64(gold+j*16u)==id){found=1;break;}
           if(found)wg++;
       }else if((id>>32u)==0x0005000fu){
           for(unsigned j=8u;j<15u;j++)
               if(w138_u64(base+j*16u)==id){found=1;break;}
           if(found)hb++;
       }
       if(!found)return 0;
       allowed[i]=id;
    }
    if(wg!=games||hb!=W148_HB)return 0;
    *outCount=count;return 1;
}
static inline int w148_working_gate(const uint8_t *gold,size_t gn,
                  const uint8_t *working,size_t wn){
    if(!gold||!working||gn!=W138_BARISTA||wn!=W138_BARISTA||
       memcmp(gold,working,W148_META_BEGIN)||
       memcmp(gold+W148_META_END,working+W148_META_END,
              W138_BARISTA-W148_META_END))return 0;
    unsigned changes=0;
    for(unsigned i=W148_META_BEGIN;i<W148_META_END;i++)
       if(gold[i]!=working[i]&&++changes>W148_ALLOWED_DRIFT)return 0;
    return 1;
}
