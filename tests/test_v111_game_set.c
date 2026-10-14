#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../console_menu_db_integrated/src/wlp_game_set.h"
int main(void){
 const uint64_t id1=UINT64_C(0x0005000010000001),id2=UINT64_C(0x0005000010000002);
 uint64_t a[]={id2,id1},b[]={id1,id2},c[]={id2};
 uint8_t da[32],db[32],dc[32];char h[65];
 assert(wlp_game_set_hash(a,2,da));
 assert(wlp_game_set_hash(b,2,db));
 assert(memcmp(da,db,32)==0);
 assert(a[0]==id1&&a[1]==id2);
 wlp_hash_hex(da,h);
 assert(strcmp(h,"F44A5098EFA6C9417E760CA30630D1952841C1F82D440D3B6BE31FCCDF04CEAB")==0);
 assert(wlp_game_set_hash(c,1,dc));
 assert(memcmp(da,dc,32)!=0);
 wlp_hash_hex(dc,h);
 assert(strcmp(h,"A4EA704A7F2EE11BCE636682815F5A9652D3481929749830D87C7CC2B7ABBF48")==0);
 uint64_t dup[]={id1,id1};
 assert(!wlp_game_set_hash(dup,2,da));
 puts("PASS: canonical game-only SHA256 stable under reordered MCP data");
 return 0;
}
