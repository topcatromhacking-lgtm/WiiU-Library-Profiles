#pragma once
// WULP145 - independent original141 verification; zero filesystem calls.
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "wulp_v138_gate.h"
#include "wulp_v144_working_gate.h"
#define W145_LEGACY_GAMES 141u
#define W145_LEGACY_HOME 7u
#define W145_LEGACY_ALLOW 148u
#define W145_LEGACY_WLPA (44u+W145_LEGACY_ALLOW*8u+4u)

static inline bool w145_legacy_pair_gate(const char *marker,size_t mn,
 const uint8_t *golden,size_t gn,const uint8_t *wlpa,size_t wn,uint64_t outIds[284]){
 if(!marker||!golden||!wlpa||!outIds||mn>220||gn!=W138_BARISTA||wn!=W145_LEGACY_WLPA)return false;
 uint8_t gh[32],wh[32];char gs[65],ws[65],expected[220];
 w138_sha(golden,gn,gh);w138_sha(wlpa,wn,wh);w138_hex(gh,gs);w138_hex(wh,ws);
 int n=snprintf(expected,sizeof(expected),"WULP145:1\nprofile=original141\nbarista_sha256=%s\nwlpa_sha256=%s\n",gs,ws);
 if(n<0||(size_t)n>=sizeof(expected)||mn!=(size_t)n||memcmp(marker,expected,mn))return false;
 if(memcmp(wlpa,"WLPA",4)||wlpa[4]||wlpa[5]!=1||wlpa[6]||wlpa[7]||
    w138_u32(wlpa+8)!=W145_LEGACY_ALLOW||memcmp(wlpa+12,gh,32)||
    w138_crc(wlpa,wn-4)!=w138_u32(wlpa+wn-4))return false;
 for(unsigned i=0;i<15;i++)if(!w138_u64(golden+i*16u))return false;
 unsigned games=0,hb=0;uint64_t buf[W145_LEGACY_ALLOW]={};
 for(unsigned i=315;i<600;i++){
    const uint64_t id=w138_u64(golden+i*16u);
    if(!id)continue;
    if((id>>32)!=0x00050000u||++games>W145_LEGACY_GAMES)return false;
    for(unsigned j=315;j<i;j++)if(id==w138_u64(golden+j*16u))return false;
 }
 if(games!=W145_LEGACY_GAMES)return false;
 for(unsigned i=0;i<W145_LEGACY_ALLOW;i++){
    const uint64_t id=w138_u64(wlpa+44u+i*8u);
    if(i&&id<=buf[i-1])return false;
    buf[i]=id;const uint32_t kind=(uint32_t)(id>>32);bool found=false;
    if(kind==0x0005000fu){
      for(unsigned j=8;j<15;j++)if(id==w138_u64(golden+j*16u)){found=true;break;}
      if(found)hb++;
    }else if(kind==0x00050000u){
      for(unsigned j=315;j<600;j++)if(id==w138_u64(golden+j*16u)){found=true;break;}
    }
    if(!found)return false;
 }
 if(hb!=W145_LEGACY_HOME)return false;
 for(unsigned i=0;i<W145_LEGACY_ALLOW;i++)outIds[i]=buf[i];
 return true;
}
static inline int w145_legacy_working_gate(const uint8_t *golden,size_t gn,
 const uint8_t *working,size_t wn,const uint64_t allowed[284],unsigned *changedOpaque){
 if(changedOpaque)*changedOpaque=0;
 if(!golden||!working||!allowed||gn!=W138_BARISTA||wn!=W138_BARISTA)return 0;
 if(memcmp(golden,working,315u*16u))return 0;
 if(memcmp(golden+W144_OPAQUE_END,working+W144_OPAQUE_END,W138_BARISTA-W144_OPAQUE_END))return 0;
 unsigned diff=0;
 for(unsigned i=W144_OPAQUE_START;i<W144_OPAQUE_END;i++)
    if(golden[i]!=working[i]&&++diff>W144_MAX_OPAQUE_CHANGED_BYTES)return 0;
 unsigned count=0;
 for(unsigned i=315;i<600;i++){
    const uint8_t *rec=working+i*16u;
    if(memcmp(rec+8u,golden+i*16u+8u,8u))return 0;
    const uint64_t id=w138_u64(rec);
    if(!id)continue;
    if((id>>32)!=0x00050000u)return 0;
    bool found=false;
    for(unsigned j=0;j<W145_LEGACY_ALLOW;j++)if(allowed[j]==id){found=true;break;}
    if(!found)return 0;
    for(unsigned j=315;j<i;j++)if(id==w138_u64(working+j*16u))return 0;
    count++;
 }
 if(count!=W145_LEGACY_GAMES)return 0;
 if(changedOpaque)*changedOpaque=diff;
 return diff?2:1;
}
