#ifndef WLP_FILTER_CORE_H
#define WLP_FILTER_CORE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define WLP_MAX_IDS 300u
#define WLP_HEADER_BYTES 44u
#define WLP_MAX_MANIFEST_BYTES (WLP_HEADER_BYTES+WLP_MAX_IDS*8u+4u)
#define WLP_GAME_APP_TYPE UINT32_C(0x80000000)
#define WLP_WII_APP_TYPE UINT32_C(0x8000002e)
typedef struct {uint64_t ids[WLP_MAX_IDS];uint8_t digest[32];uint32_t count;} wlp_manifest;
static inline uint32_t wlp_be32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static inline uint64_t wlp_be64(const uint8_t*p){return ((uint64_t)wlp_be32(p)<<32)|wlp_be32(p+4);}
static inline uint32_t wlp_crc32(const uint8_t*p,size_t n){uint32_t c=~UINT32_C(0);for(size_t i=0;i<n;i++){c^=p[i];for(int j=0;j<8;j++)c=(c>>1)^(UINT32_C(0xedb88320)&(uint32_t)(-(int32_t)(c&1)));}return ~c;}
static inline int wlp_manifest_read(const uint8_t *p,size_t n,wlp_manifest *out){
 if(!p||!out||n<WLP_HEADER_BYTES+4||memcmp(p,"WLPA",4)!=0 || p[4]!=0 || p[5]!=1||p[6]!=0||p[7]!=0)return 0;
 uint32_t count=wlp_be32(p+8);
 if(!count||count>300||n!=44+(size_t)count*8+4||wlp_crc32(p,n-4)!=wlp_be32(p+n-4))return 0;
 wlp_manifest tmp={0};tmp.count=count;memcpy(tmp.digest,p+12,32);
 uint64_t prev=0;
 for(uint32_t i=0;i<count;i++){
   uint64_t tid=wlp_be64(p+44+(size_t)i*8);
   if(!tid||tid<=prev)return 0;
   tmp.ids[i]=prev=tid;
 }
 *out=tmp;return 1;
}
static inline int wlp_has(const wlp_manifest*p,uint64_t id){
 uint32_t l=0,r=p->count;
 while(l<r){uint32_t mid=l+(r-l)/2;if(p->ids[mid]<id)l=mid+1;else r=mid;}
 return l<p->count && p->ids[l]==id;
}
static inline int wlp_is_homebrew(uint64_t tid){return (uint32_t)(tid>>32)==UINT32_C(0x0005000f);}
static inline int wlp_is_game(uint64_t tid,uint32_t app_type){return (uint32_t)(tid>>32)==UINT32_C(0x00050000)&&(app_type==WLP_GAME_APP_TYPE||app_type==WLP_WII_APP_TYPE);}
static inline int wlp_would_hide(const wlp_manifest*p,uint64_t tid,uint32_t app_type){return (wlp_is_game(tid,app_type)||wlp_is_homebrew(tid))&&!wlp_has(p,tid);}
typedef struct {uint32_t required_games,required_homebrew,found_games,found_homebrew,hide_games,hide_homebrew;} wlp_scan;
typedef struct {uint64_t id;uint32_t app_type;} wlp_record;
static inline wlp_scan wlp_scan_titles(const wlp_manifest *m,const wlp_record*records,size_t count){
 wlp_scan s={0};
 for(uint32_t i=0;i<m->count;i++){
   uint32_t high=(uint32_t)(m->ids[i]>>32);
   if(high==0x00050000u)s.required_games++;
   else if(high==0x0005000fu)s.required_homebrew++;
 }
 for(size_t i=0;i<count;i++){
  if(wlp_is_game(records[i].id,records[i].app_type)){
    if(wlp_has(m,records[i].id))s.found_games++;
    else s.hide_games++;
  }else if(wlp_is_homebrew(records[i].id)){
    if(wlp_has(m,records[i].id))s.found_homebrew++;
    else s.hide_homebrew++;
  }
 }
 return s;
}
static inline int wlp_ready(const wlp_scan*s){return s->found_games==s->required_games && s->found_homebrew==s->required_homebrew && s->required_games>0 && s->required_homebrew>0;}
#endif
