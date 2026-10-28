#pragma once
// WULP149: nine console classes, stable priority, 285 games + 15 protected icons.
// Pure validator and counts; no filesystem accesses and no console data writes.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "wulp_v148_sorted.h"
#define W149_CLASSES 9u
#define W149_MASKS 512u
#define W149_TOTAL 511u
#define W149_MAX_GAMES 285u
#define W149_HB 7u
#define W149_MAX_ALLOWED 292u
#define W149_ENTRY_SIZE 12u
#define W149_CATALOG_BYTES (16u+W149_TOTAL*W149_ENTRY_SIZE)
#define W149_ORDER_BYTES (16u+W149_TOTAL*8u)
#define W149_WLPA_MAX (44u+W149_MAX_ALLOWED*8u+4u)
#define W149_MARKER_MAX 560u
#define W149_META_START 9600u
#define W149_META_END 11556u
#define W149_DRIFT 32u

struct W149Entry {uint64_t id;uint16_t klass;};
static const unsigned w149Counts[W149_CLASSES]={141,32,31,74,21,54,95,40,23};
static inline unsigned w149_total(unsigned mask){
  unsigned count=0;
  for(unsigned i=0;i<W149_CLASSES;i++){
    if(mask&(1u<<i))count+=w149Counts[i];
  }
  return count;
}
static inline unsigned w149_shown(unsigned mask){
  const unsigned n=w149_total(mask);return n>W149_MAX_GAMES?W149_MAX_GAMES:n;
}
static inline unsigned w149_visible_class(unsigned mask,unsigned index){
  if(index>=W149_CLASSES||!(mask&(1u<<index)))return 0;
  unsigned ahead=0;
  for(unsigned i=0;i<index;i++)if(mask&(1u<<i))ahead+=w149Counts[i];
  if(ahead>=W149_MAX_GAMES)return 0;
  const unsigned room=W149_MAX_GAMES-ahead;
  return room<w149Counts[index]?room:w149Counts[index];
}
static inline uint16_t w149_lookup(const W149Entry *e,unsigned n,uint64_t id){
  unsigned lo=0,hi=n;
  while(lo<hi){const unsigned md=lo+(hi-lo)/2u;
    if(e[md].id<id)lo=md+1u;else hi=md;}
  return (lo<n && e[lo].id==id)?e[lo].klass:0u;
}
static inline int w149_catalog(const uint8_t *b,size_t len,W149Entry out[W149_TOTAL]){
  if(!b||!out||len!=W149_CATALOG_BYTES||memcmp(b,"WLP9",4)||
     b[4]!=1u||b[5]||b[6]||b[7]||w138_u32(b+8u)!=W149_TOTAL||
     w138_crc(b,len-4u)!=w138_u32(b+len-4u))return 0;
  unsigned counts[W149_CLASSES]={};uint64_t prev=0;
  for(unsigned i=0;i<W149_TOTAL;i++){
    const uint8_t *p=b+12u+W149_ENTRY_SIZE*i;
    uint64_t id=w138_u64(p);
    uint16_t kind=(uint16_t)(((unsigned)p[8]<<8)|p[9]);
    if(!id||(id>>32)!=0x00050000u||(i&&id<=prev)||!kind||
       (kind&(kind-1u))||kind>256u||p[10]||p[11])return 0;
    unsigned idx=0;while(idx<9 && kind!=(1u<<idx))idx++;
    if(idx==9)return 0;
    counts[idx]++;out[i].id=id;out[i].klass=kind;prev=id;
  }
  for(unsigned i=0;i<9;i++)if(counts[i]!=w149Counts[i])return 0;
  return 1;
}
static inline int w149_order(const uint8_t *b,size_t len,
                             const W149Entry *e,uint64_t out[W149_TOTAL]){
  if(!b||!e||!out||len!=W149_ORDER_BYTES||memcmp(b,"WLO9",4)||
     w138_u32(b+4)!=W149_TOTAL||w138_u32(b+8u)||
     w138_crc(b,len-4u)!=w138_u32(b+len-4u))return 0;
  uint16_t prev=0;
  for(unsigned i=0;i<W149_TOTAL;i++){
    uint64_t id=w138_u64(b+12u+i*8u);
    uint16_t kind=w149_lookup(e,W149_TOTAL,id);
    if(!kind||(i&&kind<prev))return 0;
    for(unsigned j=0;j<i;j++)if(out[j]==id)return 0;
    out[i]=id;prev=kind;
  }
  return 1;
}
static inline int w149_parent_check(const uint8_t *gold,size_t gn,
                                    const W149Entry *e,const uint64_t *order){
  if(!gold||gn!=W138_BARISTA||!e||!order)return 0;
  unsigned seen=0;
  for(unsigned i=315;i<600;i++){
    uint64_t id=w138_u64(gold+i*16u);
    if(!id)continue;
    if(++seen>278 || (id>>32)!=0x00050000u||id!=order[seen-1u] ||
       !(w149_lookup(e,W149_TOTAL,id)&15u))return 0;
  }
  return seen==278;
}
static inline int w149_pair_gate(const uint8_t *parent,size_t pn,
                 const uint8_t *cat,size_t cn,
                 const uint8_t *orderBytes,size_t on,
                 const char *marker,size_t mn,unsigned mask,
                 const uint8_t *gold,size_t gn,
                 const uint8_t *wlpa,size_t wn,
                 uint64_t out[W149_MAX_ALLOWED],unsigned *outputCount){
  if(outputCount)*outputCount=0;
  if(!parent||!cat||!orderBytes||!marker||!gold||!wlpa||!out||!outputCount||
     pn!=W138_BARISTA||gn!=W138_BARISTA||mask>=W149_MASKS||mn>W149_MARKER_MAX||
     cn!=W149_CATALOG_BYTES||on!=W149_ORDER_BYTES)return 0;
  W149Entry entries[W149_TOTAL];uint64_t ids[W149_TOTAL];
  if(!w149_catalog(cat,cn,entries)||!w149_order(orderBytes,on,entries,ids)||
     !w149_parent_check(parent,pn,entries,ids))return 0;
  const unsigned display=w149_shown(mask), expected=display+W149_HB;
  if(wn!=44u+expected*8u+4u||wn>W149_WLPA_MAX)return 0;
  uint8_t bh[32],ch[32],oh[32],gh[32],wh[32];
  char bs[65],cs[65],os[65],gs[65],ws[65],ref[W149_MARKER_MAX+1];
  w138_sha(parent,pn,bh);w138_sha(cat,cn,ch);w138_sha(orderBytes,on,oh);
  w138_sha(gold,gn,gh);w138_sha(wlpa,wn,wh);
  w138_hex(bh,bs);w138_hex(ch,cs);w138_hex(oh,os);
  w138_hex(gh,gs);w138_hex(wh,ws);
  const int n=snprintf(ref,sizeof(ref),
      "WULP149:1\nmask=%03X\nparent_sha256=%s\ncatalog_sha256=%s\n"
      "order_sha256=%s\nbarista_sha256=%s\nwlpa_sha256=%s\n",mask,bs,cs,os,gs,ws);
  if(n<0||(size_t)n>=sizeof(ref)||(size_t)n!=mn||memcmp(ref,marker,mn))return 0;
  if(memcmp(parent,gold,315u*16u)||
     memcmp(parent+W149_META_START,gold+W149_META_START,
            W138_BARISTA-W149_META_START))return 0;
  for(unsigned i=0;i<600;i++)
    if(memcmp(parent+i*16u+8u,gold+i*16u+8u,8u))return 0;
  unsigned pos=0;
  for(unsigned i=0;i<W149_TOTAL;i++){
    if(!(w149_lookup(entries,W149_TOTAL,ids[i])&mask))continue;
    if(pos>=display)break;
    if(w138_u64(gold+(315u+pos)*16u)!=ids[i])return 0;
    pos++;
  }
  if(pos!=display)return 0;
  for(unsigned i=315u+display;i<600u;i++)if(w138_u64(gold+i*16u))return 0;
  if(memcmp(wlpa,"WLPA",4)||wlpa[4]||wlpa[5]!=1u||wlpa[6]||wlpa[7]||
     w138_u32(wlpa+8u)!=expected||memcmp(wlpa+12u,gh,32)||
     w138_crc(wlpa,wn-4u)!=w138_u32(wlpa+wn-4u))return 0;
  unsigned games=0,home=0;uint64_t prev=0;
  for(unsigned i=0;i<expected;i++){
    uint64_t id=w138_u64(wlpa+44u+i*8u);
    if(!id||(i&&id<=prev))return 0;
    prev=id;int found=0;
    if((id>>32u)==0x00050000u){
      for(unsigned j=315u;j<315u+display;j++)
        if(id==w138_u64(gold+j*16u)){found=1;break;}
      if(found)games++;
    }else if((id>>32u)==0x0005000fu){
      for(unsigned j=8u;j<15u;j++)
        if(id==w138_u64(parent+j*16u)){found=1;break;}
      if(found)home++;
    }
    if(!found)return 0;
    out[i]=id;
  }
  if(games!=display||home!=W149_HB)return 0;
  *outputCount=expected;return 1;
}
static inline int w149_working_gate(const uint8_t *gold,size_t gn,
                             const uint8_t *working,size_t wn){
  if(!gold||!working||gn!=W138_BARISTA||wn!=W138_BARISTA||
     memcmp(gold,working,W149_META_START)||
     memcmp(gold+W149_META_END,working+W149_META_END,
            W138_BARISTA-W149_META_END))return 0;
  unsigned diff=0;
  for(unsigned i=W149_META_START;i<W149_META_END;i++)
    if(gold[i]!=working[i]&&++diff>W149_DRIFT)return 0;
  return 1;
}
