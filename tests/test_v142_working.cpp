#include <cassert>
#include <cstdint>
#include <vector>
#include "wulp_v142_working_gate.h"
static void put(std::vector<uint8_t>&v,unsigned at,uint64_t value) {
    for (int i=7;i>=0;--i) {v[at+i]=(uint8_t)value;value>>=8;}
}
int main() {
    std::vector<uint8_t> golden(W140_BARISTA_SIZE,0),working;
    uint64_t allowlist[284]={};
    for(unsigned i=0;i<277;i++) {
        const uint64_t id=UINT64_C(0x0005000010001000)+i;
        allowlist[i]=id; put(golden,(315u+i)*16u,id);
    }
    for(unsigned i=0;i<7;i++) {
        const uint64_t id=UINT64_C(0x0005000f01000001)+i;
        allowlist[i+277]=id; put(golden,(8u+i)*16u,id);
    }
    working=golden;
    assert(w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist)==1);
    working[9600]^=1;
    assert(w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist)==2);
    working=golden;working[11555]^=1;
    assert(w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist)==2);
    // Normal USB icon reorder still allowed.
    working=golden;
    put(working,315u*16u,allowlist[276]);put(working,591u*16u,allowlist[0]);
    working[9823]^=1;
    assert(w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist)==2);
    // More than one opaque metadata byte cannot be inferred safe.
    working=golden;working[9600]^=1;working[11555]^=1;
    assert(!w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist));
    // Absolutely no folder-table, protected-slot or record flag modifications.
    working=golden;working[11556]^=1;
    assert(!w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist));
    working=golden;working[0]^=1;
    assert(!w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist));
    working=golden;working[315*16u+10]^=1;
    assert(!w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist));
    // Any missing, duplicated, unauthorized or wrong-type game rejects.
    working=golden;put(working,315u*16u,0);
    assert(!w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist));
    working=golden;put(working,315u*16u,allowlist[1]);
    assert(!w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist));
    working=golden;put(working,315u*16u,UINT64_C(0x000500000eeeffff));
    assert(!w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist));
    working=golden;put(working,315u*16u,UINT64_C(0x0005000f0eeeffff));
    assert(!w142_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowlist));
    assert(!w142_working_gate(golden.data(),golden.size(),working.data(),working.size()-1,allowlist));
}
