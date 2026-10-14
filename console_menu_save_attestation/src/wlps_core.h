#ifndef WLP_V12_SELECTION_CORE_H
#define WLP_V12_SELECTION_CORE_H
/* Strict WLPS v1 parser and deterministic read-only platform/region planner.
 * C11-compatible; no SD, MCP, WUPS, file or Barista operations. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "wlpdb_core.h"
#define WLPS_HEADER_BYTES 32u
#define WLPS_MAX_IDS 512u
#define WLPS_ICON_LIMIT 285u
#define WLPS_PLATFORM_MASK 0x01ffu
typedef struct { const uint8_t *raw; uint16_t platform_mask,includes,excludes; uint8_t regions,policy; uint32_t crc; } wlps_view;
typedef struct {uint64_t id; uint8_t platform,region,manual,unused;} wlps_item;
typedef struct {uint32_t games,joined,confirmed,review,unknown,excluded,requested_missing,eligible,shown,overflow,blocked;} wlps_stats;
static inline uint64_t wlps_item_id(const uint8_t*raw,uint16_t idx){return wlpdb_u64(raw+WLPS_HEADER_BYTES+(size_t)idx*8);}
static inline int wlps_contains(const wlps_view *v,uint64_t id,int exclusion){
 uint16_t n=exclusion?v->excludes:v->includes;
 uint16_t base=exclusion?v->includes:0,lo=0,hi=n;
 while(lo<hi){uint16_t mid=lo+(uint16_t)((hi-lo)/2);uint64_t x=wlps_item_id(v->raw,base+mid);if(x<id)lo=mid+1;else hi=mid;}
 return lo<n && wlps_item_id(v->raw,base+lo)==id;
}
static inline int wlps_open(wlps_view *v,const uint8_t *b,size_t len){
 if(!v)return 0;
 memset(v,0,sizeof(*v));
 if(!b||len<WLPS_HEADER_BYTES||memcmp(b,"WLPS",4)!=0)return 0;
 uint16_t platforms=wlpdb_u16(b+8),inc=wlpdb_u16(b+12),exc=wlpdb_u16(b+14);
 uint32_t crc=wlpdb_u32(b+16);
 if(wlpdb_u16(b+4)!=1||wlpdb_u16(b+6)!=WLPS_HEADER_BYTES||!platforms||(platforms&~WLPS_PLATFORM_MASK)||
    !b[10]||b[10]>7||b[11]>2||inc>WLPS_MAX_IDS||exc>WLPS_MAX_IDS||
    wlpdb_u32(b+20)!=len||wlpdb_u32(b+24)||wlpdb_u32(b+28)||
    len!=WLPS_HEADER_BYTES+(size_t)(inc+exc)*8 || wlpdb_crc32(b+WLPS_HEADER_BYTES,len-WLPS_HEADER_BYTES)!=crc)return 0;
 for(uint32_t i=0;i<(uint32_t)inc+exc;i++){
   uint64_t x=wlps_item_id(b,(uint16_t)i);
   if(i && ((i!=inc && x<=wlps_item_id(b,(uint16_t)(i-1)))))return 0;
 }
 v->raw=b;v->platform_mask=platforms;v->regions=b[10];v->policy=b[11];v->includes=inc;v->excludes=exc;v->crc=crc;
 for(uint16_t i=0;i<inc;i++)if(wlps_contains(v,wlps_item_id(b,i),1)){memset(v,0,sizeof(*v));return 0;}
 return 1;
}
static inline int wlps_item_cmp(const void *a,const void *b){
 const wlps_item *x=(const wlps_item*)a,*y=(const wlps_item*)b;
 if(x->platform!=y->platform)return x->platform<y->platform?-1:1;
 return (x->id>y->id)-(x->id<y->id);
}
/* 'installed' must contain strictly ascending, unique GAME Title IDs.
 * out_capacity >= installed_count; output is sorted by platform then Title ID. */
static inline int wlps_plan(const wlps_view *profile,const wlpdb_view *db,
                            const uint64_t *installed,size_t installed_count,
                            wlps_item *out,size_t out_capacity,wlps_stats *stats){
 if(!profile||!profile->raw||!db||!db->bytes||!stats||
    (installed_count&&(!installed||!out))||out_capacity<installed_count||installed_count>4096u)return 0;
 memset(stats,0,sizeof(*stats));stats->games=(uint32_t)installed_count;
 for(size_t i=1;i<installed_count;i++)if(installed[i-1]>=installed[i])return 0;
 for(size_t i=0;i<installed_count;i++){
  uint64_t tid=installed[i];wlpdb_result entry={0};
  if(!wlpdb_find(db,tid,&entry)){stats->unknown++;continue;}
  stats->joined++;
  if(!entry.confirmed||!entry.platform||!entry.region_mask){stats->review++;continue;}
  stats->confirmed++;
  if(wlps_contains(profile,tid,1)){stats->excluded++;continue;}
  int manual=wlps_contains(profile,tid,0);
  if(!manual && (!(profile->platform_mask&(1u<<(entry.platform-1)))||!(entry.region_mask&profile->regions)))continue;
  out[stats->eligible].id=tid;
  out[stats->eligible].platform=entry.platform;
  out[stats->eligible].region=entry.region_mask;
  out[stats->eligible].manual=(uint8_t)(manual!=0);
  out[stats->eligible].unused=0;
  stats->eligible++;
 }
 for(uint16_t j=0;j<profile->includes;j++){
  uint64_t id=wlps_item_id(profile->raw,j);
  size_t lo=0,hi=installed_count;
  while(lo<hi){size_t mid=lo+(hi-lo)/2;if(installed[mid]<id)lo=mid+1;else hi=mid;}
  if(lo==installed_count||installed[lo]!=id)stats->requested_missing++;
 }
 qsort(out,stats->eligible,sizeof(*out),wlps_item_cmp);
 stats->shown=stats->eligible<WLPS_ICON_LIMIT?stats->eligible:WLPS_ICON_LIMIT;
 stats->overflow=stats->eligible-stats->shown;
 stats->blocked=stats->overflow && profile->policy==0;
 return 1;
}
#endif
