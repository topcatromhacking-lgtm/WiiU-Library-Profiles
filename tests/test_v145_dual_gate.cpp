#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
#include "wulp_v145_dual_gate.h"
static void p64(std::vector<uint8_t> &v,unsigned at,uint64_t x){
 for(int i=7;i>=0;--i){v[at+i]=(uint8_t)x;x>>=8;}
}
static void p32(std::vector<uint8_t> &v,unsigned at,uint32_t x){
 for(int i=3;i>=0;--i){v[at+i]=(uint8_t)x;x>>=8;}
}
int main(){
 std::vector<uint8_t> golden(W138_BARISTA,0),work;
 uint64_t ids[W145_LEGACY_ALLOW]={},parsed[284]={};
 for(unsigned i=0;i<8;i++)p64(golden,i*16,UINT64_C(0x0005001010000001)+i);
 for(unsigned i=0;i<W145_LEGACY_GAMES;i++){
   ids[i]=UINT64_C(0x0005000010000001)+i;
   p64(golden,(315+i)*16,ids[i]);
 }
 for(unsigned i=0;i<7;i++){
   ids[141+i]=UINT64_C(0x0005000f01000001)+i;
   p64(golden,(8+i)*16,ids[141+i]);
 }
 for(unsigned i=0;i<600;i++)p32(golden,i*16+8,i<300?1:2);
 for(unsigned folder=0;folder<60;folder++)
  for(unsigned k=0;k<120;k++)
   p32(golden,11556+1976*folder+k*16+8,k<60?1:2);
 std::vector<uint8_t> wlpa(W145_LEGACY_WLPA,0);
 memcpy(wlpa.data(),"WLPA",4);wlpa[5]=1;p32(wlpa,8,W145_LEGACY_ALLOW);
 uint8_t gh[32],wh[32];char gs[65],ws[65],marker[220];
 w138_sha(golden.data(),golden.size(),gh);
 memcpy(wlpa.data()+12,gh,32);
 for(unsigned i=0;i<W145_LEGACY_ALLOW;i++)p64(wlpa,44+i*8,ids[i]);
 p32(wlpa,wlpa.size()-4,w138_crc(wlpa.data(),wlpa.size()-4));
 w138_sha(wlpa.data(),wlpa.size(),wh);
 w138_hex(gh,gs);w138_hex(wh,ws);
 snprintf(marker,sizeof(marker),
    "WULP145:1\nprofile=original141\nbarista_sha256=%s\nwlpa_sha256=%s\n",gs,ws);
 assert(w145_legacy_pair_gate(marker,strlen(marker),golden.data(),golden.size(),wlpa.data(),wlpa.size(),parsed));
 for(unsigned i=0;i<W145_LEGACY_ALLOW;i++)assert(ids[i]==parsed[i]);
 work=golden;unsigned changed=900;
 assert(w145_legacy_working_gate(golden.data(),golden.size(),work.data(),work.size(),parsed,&changed)==1&&changed==0);
 work[9600]^=1;work[9601]^=1;
 assert(w145_legacy_working_gate(golden.data(),golden.size(),work.data(),work.size(),parsed,&changed)==2&&changed==2);
 work=golden;for(unsigned i=0;i<33;i++)work[9600+i]^=1;
 assert(!w145_legacy_working_gate(golden.data(),golden.size(),work.data(),work.size(),parsed,&changed));
 work=golden;work[0]^=1;
 assert(!w145_legacy_working_gate(golden.data(),golden.size(),work.data(),work.size(),parsed,&changed));
 work=golden;work[11556]^=1;
 assert(!w145_legacy_working_gate(golden.data(),golden.size(),work.data(),work.size(),parsed,&changed));
 work=golden;p32(work,315*16+8,9);
 assert(!w145_legacy_working_gate(golden.data(),golden.size(),work.data(),work.size(),parsed,&changed));
 work=golden;p64(work,315*16,0);
 assert(!w145_legacy_working_gate(golden.data(),golden.size(),work.data(),work.size(),parsed,&changed));
 work=golden;p64(work,315*16,ids[1]);
 assert(!w145_legacy_working_gate(golden.data(),golden.size(),work.data(),work.size(),parsed,&changed));
 work=golden;p64(work,315*16,UINT64_C(0x000500001234ffff));
 assert(!w145_legacy_working_gate(golden.data(),golden.size(),work.data(),work.size(),parsed,&changed));
 wlpa[47]^=1;
 assert(!w145_legacy_pair_gate(marker,strlen(marker),golden.data(),golden.size(),wlpa.data(),wlpa.size(),parsed));
 wlpa[47]^=1;marker[10]='0';
 assert(!w145_legacy_pair_gate(marker,strlen(marker),golden.data(),golden.size(),wlpa.data(),wlpa.size(),parsed));
 return 0;
}
