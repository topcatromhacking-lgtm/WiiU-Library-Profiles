#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "../console_menu_inventory_embedded/src/wlp_inventory_core.h"

int main(void){
 const uint64_t game=UINT64_C(0x000500001010e600);
 const uint64_t hb=UINT64_C(0x0005000fa12a4ecf);
 wlp_scan good={1,1,1,1,370,0};
 wlp_scan partial={1,1,0,1,0,0};
 assert(wlp_inventory_candidate(623,700,0,&good,1));
 assert(!wlp_inventory_candidate(623,700,0,&good,0));
 assert(!wlp_inventory_candidate(623,700,623,&good,1));
 assert(!wlp_inventory_candidate(623,700,624,&good,1));
 assert(!wlp_inventory_candidate(701,700,0,&good,1));
 assert(!wlp_inventory_candidate(623,700,0,&partial,1));
 assert(!wlp_inventory_candidate(4097,4097,0,&good,1));
 assert(!wlp_inventory_candidate(12,700,0,NULL,1));
 assert(!wlp_inventory_candidate(623,700,0,&good,0));
 assert(strcmp(wlp_inventory_kind(game,WLP_GAME_APP_TYPE),"GAME")==0);
 assert(strcmp(wlp_inventory_kind(hb,0),"HOMEBREW_SYNTHETIC")==0);
 assert(strcmp(wlp_inventory_platform_evidence(game,WLP_GAME_APP_TYPE),"WIIU_OR_VC_UNRESOLVED")==0);
 assert(strcmp(wlp_inventory_platform_evidence(game,WLP_WII_APP_TYPE),"WII_APP_TYPE")==0);
 assert(strcmp(wlp_inventory_platform_evidence(hb,0),"HOMEBREW_SYNTHETIC")==0);
 puts("PASS: only opt-in, ready, larger snapshots; no unsupported VC platform claim");
 return 0;
}
