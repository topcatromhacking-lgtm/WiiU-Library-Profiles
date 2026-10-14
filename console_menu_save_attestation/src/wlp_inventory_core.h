#ifndef WLP_PASSIVE_INVENTORY_CORE_H
#define WLP_PASSIVE_INVENTORY_CORE_H
#include <stdint.h>
#include "wlp_filter_core.h"

/* Protect the Menu from unexpectedly large, expensive exports.
   This is a diagnostic ceiling, not a 300-icon menu ceiling. */
#define WLP_INVENTORY_MAX_RECORDS 4096u

static inline int wlp_inventory_candidate(uint32_t count,uint32_t capacity,
                                         uint32_t best_count,
                                         const wlp_scan *stats,
                                         int opted_in) {
    return opted_in && stats && wlp_ready(stats)
       && count>best_count && count<=capacity
       && count<=WLP_INVENTORY_MAX_RECORDS;
}
static inline const char *wlp_inventory_kind(uint64_t id,uint32_t app){
    uint32_t hi=(uint32_t)(id>>32);
    if(wlp_is_homebrew(id))return "HOMEBREW_SYNTHETIC";
    if(wlp_is_game(id,app))return "GAME";
    if(hi==0x0005000eu)return "UPDATE";
    if(hi==0x0005000cu)return "DLC";
    return "OTHER";
}
/* The MCP app type does not reliably identify the VC emulation platform.
   Preserve the raw evidence; do not assign arbitrary Wii U/VC platforms. */
static inline const char *wlp_inventory_platform_evidence(uint64_t id,uint32_t app){
    if(wlp_is_homebrew(id))return "HOMEBREW_SYNTHETIC";
    if(wlp_is_game(id,app) && app==WLP_WII_APP_TYPE)return "WII_APP_TYPE";
    if(wlp_is_game(id,app))return "WIIU_OR_VC_UNRESOLVED";
    return "NOT_CLASSIFIED";
}
#endif
