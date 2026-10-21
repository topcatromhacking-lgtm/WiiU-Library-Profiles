#include <cassert>
#include <cstdint>
#include <vector>
#include "wulp_v140_working_gate.h"
static void put(std::vector<uint8_t>&v,unsigned at,uint64_t value){
    for(int k=7;k>=0;--k){v[at+k]=(uint8_t)value;value>>=8;}
}
int main(){
    std::vector<uint8_t> golden(W140_BARISTA_SIZE,0), working;
    uint64_t ids[284]={};
    for(unsigned i=0;i<277;i++){
        auto id=UINT64_C(0x0005000010001000)+i;
        ids[i]=id;put(golden,(315+i)*16u,id);
    }
    for(unsigned i=0;i<7;i++){
        auto id=UINT64_C(0x0005000f01000001)+i;
        ids[277+i]=id;put(golden,(8+i)*16u,id);
    }
    working=golden;
    assert(w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    // Accept supported reorder and moving a game into the final vacant USB slot.
    put(working,315*16u,ids[276]);put(working,591*16u,ids[0]);
    assert(w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    working=golden;put(working,315*16u,0);put(working,599*16u,ids[0]);
    assert(w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    // Reject missing, substituted, duplicate and unauthorized entries.
    working=golden;put(working,315*16u,0);
    assert(!w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    working=golden;put(working,315*16u,ids[1]);
    assert(!w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    working=golden;put(working,315*16u,UINT64_C(0x0005000011111111));
    assert(!w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    // Reject protected first-page, game type, root metadata and folder changes.
    working=golden;working[0]^=1;
    assert(!w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    working=golden;working[315*16u+15]^=1;
    assert(!w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    working=golden;working[9600]^=1;
    assert(!w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    working=golden;working[11556]^=1;
    assert(!w140_working_gate(golden.data(),golden.size(),working.data(),working.size(),ids));
    assert(!w140_working_gate(golden.data(),golden.size(),working.data(),working.size()-1,ids));
}
