/* Synthetic Menu redirection observer. NOT authentic Wii U telemetry. */
#include "../src/v133_trace_capture.h"
#include "../src/v134_physical_source.h"
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    const char *logical = "/vol/save/80000001/BaristaAccountSaveFile.dat";
    const char *physical = "fs:/vol/external01/wiiu/homebrew_on_menu_plugin/UNITTEST/save/80000001/BaristaAccountSaveFile.dat";
    if (!wlp_v134_matches_redirect(logical, physical, "80000001")) return 11;
    uint8_t *data = malloc(WLP_V134_SAVEBYTES);
    if (!data) return 12;
    for (size_t i = 0; i < WLP_V134_SAVEBYTES; ++i) data[i] = (uint8_t)((i*17u+7u)%256u);
    WlpTrace *t = calloc(1, sizeof(*t));
    if (!t) { free(data); return 13; }
    if (!wlp_trace_init(t, "snapshot-native-demo")) return 14;
    if (!wlp_trace_init_save(t, 1, 1, 1, "80000001")) return 15;
    if (!wlp_trace_register(t, 1, 11)) return 16;
    if (!wlp_trace_open(t, 11, 55, 1)) return 17;
    for (size_t offset = 0; offset < WLP_V134_SAVEBYTES; offset += 4096) {
        size_t n = WLP_V134_SAVEBYTES - offset;
        if (n > 4096) n = 4096;
        uint32_t checked=0;
        if (!wlp_v134_read_offset((int64_t)(offset+n), n, &checked) || checked != offset) return 18;
        if (!wlp_trace_read(t, 55, checked, data+offset, n)) return 19;
    }
    if (!wlp_trace_close(t, 55, 1)) return 20;
    /* Menu keeps redirection registered. Finalize the observation snapshot. */
    if (!wlp_trace_active_snapshot(t, 11, 1)) return 21;
    if (!wlp_trace_end(t)) return 22;
    if (t->overflow) return 23;
    if (fwrite(t->data, 1, t->used, stdout) != t->used) return 24;
    free(t); free(data);
    return 0;
}
