/* Host-only tests of the exact predicate used by the actual WUPS hook.
 * No console SDK is needed for these tests. */
#include "../console_menu_filter/src/filter_logic.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void expect(uint64_t id, uint32_t app_type, int result) {
    assert(wlp_hide_five_game(id, app_type) == result);
}
int main(void) {
    const uint64_t target_ids[5] = {
        UINT64_C(0x000500001010E600),
        UINT64_C(0x0005000010176B00),
        UINT64_C(0x00050000101F2800),
        UINT64_C(0x0005000010202500),
        UINT64_C(0x000500001018FB00)
    };
    for (size_t i = 0; i < 5; ++i) {
        expect(target_ids[i], UINT32_C(0x80000000), 1);
        expect(target_ids[i], UINT32_C(0x8000002E), 0); /* Wii VC */
        expect(target_ids[i], UINT32_C(0x0800001B), 0); /* Update */
        expect(target_ids[i], UINT32_C(0x0800000E), 0); /* DLC */
        expect(target_ids[i] ^ UINT64_C(0x0000000100000000),
               UINT32_C(0x80000000), 0); /* Not base title */
    }
    expect(UINT64_C(0x0005000010139500), UINT32_C(0x80000000), 0);
    expect(UINT64_C(0x0005000F1010E600), UINT32_C(0x80000000), 0);
    expect(UINT64_C(0x0005001010040100), UINT32_C(0x90000001), 0);
    puts("PASS: five exact game IDs hidden; all other kinds passed through");
    return 0;
}
