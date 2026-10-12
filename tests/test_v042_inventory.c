#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include "../console_menu_inventory_diagnostic/src/diagnostic_policy.h"
#include "../console_menu_inventory_diagnostic/src/allowlist_core.h"
int main(void){
  /* First call: partial 492 visible game records, 10 Wii U targets missing.
   * Later call: more exact selected games => choose later snapshot. */
  assert(wlp_better_snapshot(0,10,492,0,0));
  assert(wlp_better_snapshot(1,5,502,10,492));
  assert(wlp_better_snapshot(1,10,511,10,492));
  assert(!wlp_better_snapshot(1,11,511,10,492));
  assert(!wlp_better_snapshot(1,10,492,10,492));
  assert(!wlp_better_snapshot(1,10,490,10,492));
  assert(wlp_better_snapshot(1,0,500,10,492));
  /* Strict discriminator: an update or DLC is never a game to filter. */
  assert(wlp_is_base_game(UINT64_C(0x000500001012DC00),WLP_GAME_APP_TYPE));
  assert(!wlp_is_base_game(UINT64_C(0x0005000E1012DC00),WLP_GAME_APP_TYPE));
  assert(!wlp_is_base_game(UINT64_C(0x0005000C1012DC00),WLP_GAME_APP_TYPE));
  assert(wlp_is_synthetic(UINT64_C(0x0005000FA12A4ECF)));
  puts("v0.4.2 monotonic visibility and protected-type tests PASS");
  return 0;
}
