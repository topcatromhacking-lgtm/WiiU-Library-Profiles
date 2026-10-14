#ifndef WLP_BARISTA_PROBE_CORE_H
#define WLP_BARISTA_PROBE_CORE_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define WLP_BARISTA_BYTES 130116u
#define WLP_ROOT_RECORDS 600u
#define WLP_BARISTA_RECORD_BYTES 16u
#define WLP_BARISTA_FOLDER_OFFSET 0x2d24u
#define WLP_BARISTA_FOLDER_STRIDE 1976u
#define WLP_BARISTA_FOLDER_COUNT 60u
typedef struct {uint32_t fixed_icons,usb_games,folders_empty;uint64_t fixed_ids[15];} wlp_barista_inspection;
static inline uint32_t wlp_barista_be32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static inline uint64_t wlp_barista_be64(const uint8_t*p){return ((uint64_t)wlp_barista_be32(p)<<32)|wlp_barista_be32(p+4);}
static inline int wlp_barista_inspect(const uint8_t *buf,size_t n,wlp_barista_inspection *out){
 if(!buf||!out||n!=WLP_BARISTA_BYTES)return 0;
 wlp_barista_inspection found={0};
 for(uint32_t i=0;i<WLP_ROOT_RECORDS;i++){
  const uint8_t *p=buf+(size_t)i*16;
  uint64_t tid=wlp_barista_be64(p);
  uint32_t type=wlp_barista_be32(p+8),flags=wlp_barista_be32(p+12);
  if(flags)return 0;
  if(i<300){
   if(type!=1 && !(i<15 && (type==5||type==9)))return 0;
   if(i<15){
    if(!tid)return 0;
    uint32_t hi=(uint32_t)(tid>>32);
    if(i<8 && hi!=0x00050010u&&hi!=0x0005001bu&&hi!=0x00050030u)return 0;
    if(i>=8&&hi!=0x0005000fu)return 0;
    for(uint32_t j=0;j<i;j++)if(found.fixed_ids[j]==tid)return 0;
    found.fixed_ids[i]=tid;found.fixed_icons++;
   } else if(tid)return 0;
  }else{
   if(type!=2)return 0;
   if(i<315){if(tid)return 0;}
   else if(tid){
    if((uint32_t)(tid>>32)!=0x00050000u)return 0;
    for(uint32_t j=315;j<i;j++)if(wlp_barista_be64(buf+(size_t)j*16)==tid)return 0;
    found.usb_games++;
   }
  }
 }
 if(WLP_BARISTA_FOLDER_OFFSET+WLP_BARISTA_FOLDER_COUNT*WLP_BARISTA_FOLDER_STRIDE!=WLP_BARISTA_BYTES)return 0;
 for(uint32_t folder=0;folder<WLP_BARISTA_FOLDER_COUNT;folder++){
  const uint8_t *base=buf+WLP_BARISTA_FOLDER_OFFSET+(size_t)folder*WLP_BARISTA_FOLDER_STRIDE;
  for(uint32_t slot=0;slot<120;slot++){
   const uint8_t *p=base+(size_t)slot*16;
   if(wlp_barista_be64(p)||wlp_barista_be32(p+12)||wlp_barista_be32(p+8)!=(slot<60?1u:2u))return 0;
  }
  for(uint32_t j=1920;j<1976;j++)if(base[j])return 0;
  found.folders_empty++;
 }
 *out=found;return 1;
}
#endif
