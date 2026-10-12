#ifndef WLP_ALLOWLIST_CORE_H
#define WLP_ALLOWLIST_CORE_H
/* Portable, read-only parser/predicate. No console, FS, or Barista writes. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define WLP_MAX_IDS 300u
#define WLP_HEADER_BYTES 44u
#define WLP_MAX_MANIFEST_BYTES (WLP_HEADER_BYTES+WLP_MAX_IDS*8u+4u)
#define WLP_GAME_APP_TYPE UINT32_C(0x80000000)
#define WLP_WII_APP_TYPE UINT32_C(0x8000002e)
typedef struct {
  uint64_t ids[WLP_MAX_IDS];
  unsigned char barista_digest[32];
  uint32_t count;
} wlp_allowlist;
static inline uint16_t wlp_be16(const unsigned char *p){
  return (uint16_t)(((uint16_t)p[0]<<8)|p[1]);
}
static inline uint32_t wlp_be32(const unsigned char *p){
  return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static inline uint64_t wlp_be64(const unsigned char *p){
  return ((uint64_t)wlp_be32(p)<<32)|wlp_be32(p+4);
}
static inline uint32_t wlp_crc32(const unsigned char *p,size_t n){
  uint32_t c=UINT32_C(0xffffffff);
  for(size_t i=0;i<n;i++){
    c^=p[i];
    for(unsigned j=0;j<8;j++){
      c=(c>>1)^(UINT32_C(0xedb88320)&(uint32_t)(-(int32_t)(c&1u)));
    }
  }
  return ~c;
}
static inline int wlp_parse_allowlist(const unsigned char *data,size_t n,wlp_allowlist *out){
  if(!data||!out||n<WLP_HEADER_BYTES+4u||memcmp(data,"WLPA",4)!=0||
     wlp_be16(data+4)!=1||wlp_be16(data+6)!=0) return 0;
  uint32_t count=wlp_be32(data+8);
  if(count==0||count>WLP_MAX_IDS||
     n!=WLP_HEADER_BYTES+(size_t)count*8u+4u||
     wlp_crc32(data,n-4u)!=wlp_be32(data+n-4u)) return 0;
  wlp_allowlist tmp={0};
  tmp.count=count;
  memcpy(tmp.barista_digest,data+12,32);
  uint64_t previous=0;
  for(uint32_t i=0;i<count;i++){
    uint64_t tid=wlp_be64(data+WLP_HEADER_BYTES+(size_t)i*8u);
    if(tid==0||tid<=previous)return 0;
    tmp.ids[i]=previous=tid;
  }
  *out=tmp;
  return 1;
}
static inline int wlp_contains(const wlp_allowlist *p,uint64_t tid){
  uint32_t left=0,right=p->count;
  while(left<right){
    uint32_t mid=left+(right-left)/2;
    if(p->ids[mid]<tid) left=mid+1;
    else right=mid;
  }
  return left<p->count && p->ids[left]==tid;
}
static inline int wlp_is_base_game(uint64_t tid,uint32_t app_type){
  return (uint32_t)(tid>>32)==UINT32_C(0x00050000) &&
       (app_type==WLP_GAME_APP_TYPE||app_type==WLP_WII_APP_TYPE);
}
static inline int wlp_is_synthetic(uint64_t tid){
  return (uint32_t)(tid>>32)==UINT32_C(0x0005000f);
}
static inline int wlp_hide_from_menu(const wlp_allowlist *p,uint64_t tid,uint32_t app_type){
  return (wlp_is_synthetic(tid)||wlp_is_base_game(tid,app_type)) &&
         !wlp_contains(p,tid);
}
typedef struct {
  uint32_t expected_game,expected_homebrew,expected_other;
  uint32_t present_game,present_homebrew,present_other;
  uint32_t missing_game,missing_homebrew,missing_other;
  uint32_t would_hide_game,would_hide_homebrew;
  uint32_t game_records,homebrew_records;
} wlp_dry_stats;
typedef struct { uint64_t tid; uint32_t app_type; } wlp_observed;
/* Exact ID set comparison, explicitly distinguishes absent manifest entries.
 * "other" includes non-game first-page system entries that may not be
 * returned in MCP_TitleList. They are diagnostic, NOT a failed profile. */
static inline wlp_dry_stats wlp_dry_evaluate(
    const wlp_allowlist *p,const wlp_observed *records,uint32_t count){
  wlp_dry_stats s={0};
  for(uint32_t j=0;j<p->count;j++){
    uint32_t upper=(uint32_t)(p->ids[j]>>32);
    if(upper==UINT32_C(0x0005000f))s.expected_homebrew++;
    else if(upper==UINT32_C(0x00050000))s.expected_game++;
    else s.expected_other++;
    int found=0;
    for(uint32_t i=0;i<count;i++){
      if(records[i].tid==p->ids[j]){found=1;break;}
    }
    if(upper==UINT32_C(0x0005000f))found?s.present_homebrew++:s.missing_homebrew++;
    else if(upper==UINT32_C(0x00050000))found?s.present_game++:s.missing_game++;
    else found?s.present_other++:s.missing_other++;
  }
  for(uint32_t i=0;i<count;i++){
    uint64_t tid=records[i].tid;
    if(wlp_is_synthetic(tid)){
      s.homebrew_records++;
      if(wlp_hide_from_menu(p,tid,records[i].app_type))s.would_hide_homebrew++;
    }else if(wlp_is_base_game(tid,records[i].app_type)){
      s.game_records++;
      if(wlp_hide_from_menu(p,tid,records[i].app_type))s.would_hide_game++;
    }
  }
  return s;
}
#endif
