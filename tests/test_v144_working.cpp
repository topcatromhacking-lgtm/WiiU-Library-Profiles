#include <cassert>
#include <cstdint>
#include <vector>
#include "wulp_v144_working_gate.h"
static void put(std::vector<uint8_t> &v,unsigned offset,uint64_t n) {
    for(int b=7;b>=0;--b) {v[offset+b]=(uint8_t)n;n>>=8;}
}
int main() {
    std::vector<uint8_t> golden(W140_BARISTA_SIZE,0), working;
    uint64_t allowed[284]={};
    for(unsigned i=0;i<277;i++) {
        allowed[i]=UINT64_C(0x0005000010001000)+i;
        put(golden,(315+i)*16u,allowed[i]);
    }
    for(unsigned i=0;i<7;i++) {
        allowed[i+277]=UINT64_C(0x0005000f01000000)+i;
        put(golden,(8+i)*16u,allowed[i+277]);
    }
    unsigned drift=999;
    working=golden;
    assert(w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift)==1);
    assert(drift==0);
    // 1, 2 and up to 32 bytes of cumulative opaque metadata drift are
    // accepted without rewriting or normalizing those bytes.
    for(unsigned n=1;n<=W144_MAX_OPAQUE_CHANGED_BYTES;n++) {
        working[W144_OPAQUE_START+n-1]^=0x01;
        assert(w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift)==2);
        assert(drift==n);
    }
    // Beyond the bounded drift count, fail to the still-verified title filter.
    working[W144_OPAQUE_START+W144_MAX_OPAQUE_CHANGED_BYTES]^=0x01;
    assert(w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift)==0);
    // A valid ID reordering does not invalidate the working profile.
    working=golden;
    put(working,315*16u,allowed[276]);put(working,591*16u,allowed[0]);
    working[W144_OPAQUE_END-1]^=0x01;
    assert(w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift)==2);
    // No changes anywhere in immutable protected icons, flags, folder bytes.
    working=golden; working[0]^=1;
    assert(!w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift));
    working=golden; working[315*16u+8]^=1;
    assert(!w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift));
    working=golden; working[W144_OPAQUE_END]^=1;
    assert(!w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift));
    working=golden; put(working,315*16u,allowed[1]);
    assert(!w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift));
    working=golden; put(working,315*16u,0);
    assert(!w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift));
    working=golden; put(working,315*16u,UINT64_C(0x000500001234ffff));
    assert(!w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift));
    working=golden; put(working,315*16u,UINT64_C(0x0005000f1234ffff));
    assert(!w144_working_gate(golden.data(),golden.size(),working.data(),working.size(),allowed,&drift));
    assert(!w144_working_gate(golden.data(),golden.size(),working.data(),working.size()-1,allowed,&drift));
}
