#ifndef WLP_FIVE_FILTER_LOGIC_H
#define WLP_FIVE_FILTER_LOGIC_H

#include <stdint.h>

/*
 * v0.2: exact five game Title IDs chosen for the first on-console test.
 * No names, partial IDs or game-title heuristics are used.
 * Only base Wii U GAME-type records can be excluded.
 */
static inline int wlp_hide_five_game(uint64_t title_id, uint32_t app_type) {
    if ((title_id >> 32) != UINT64_C(0x00050000) ||
        app_type != UINT32_C(0x80000000)) {
        return 0; /* Never touch updates, DLC, OS components, Wii or homebrew. */
    }
    switch (title_id) {
        case UINT64_C(0x000500001010E600): /* 007 Legends */
        case UINT64_C(0x0005000010176B00): /* 1001 Spikes */
        case UINT64_C(0x00050000101F2800): /* 8Bit Hero */
        case UINT64_C(0x0005000010202500): /* Alice in Wonderland */
        case UINT64_C(0x000500001018FB00): /* Alphadia Genesis */
            return 1;
        default:
            return 0;
    }
}

#endif
