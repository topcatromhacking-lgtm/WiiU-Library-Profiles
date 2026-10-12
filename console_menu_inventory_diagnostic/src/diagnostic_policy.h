#ifndef WLP_INVENTORY_DIAGNOSTIC_POLICY_H
#define WLP_INVENTORY_DIAGNOSTIC_POLICY_H
#include <stdint.h>
/* Prefer a call in which more expected games/homebrew appear; if tied,
 * prefer the call enumerating more installed game entries.
 * Never mistake a later but smaller MCP enumeration for full inventory. */
static inline int wlp_better_snapshot(
    int have_best,uint32_t missing,uint32_t base_count,
    uint32_t best_missing,uint32_t best_base_count){
  return !have_best || missing<best_missing ||
         (missing==best_missing && base_count>best_base_count);
}
#endif
