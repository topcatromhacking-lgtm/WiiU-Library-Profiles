/*
 * v0.4 Menu hook visibility probe. OBSERVATION ONLY.
 * Calls original MCP_TitleList and RETURNS ITS UNCHANGED RESULT.
 * Logs counts, not IDs, after the original function returns.
 * Disabled by default; activated only by exact SD opt-in file.
 * Does not read/write Barista, MCP inventory, NAND or USB titles.
 */
#include <wups.h>
#include <coreinit/mcp.h>
#include <coreinit/title.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

WUPS_PLUGIN_NAME("Library Profiles - v0.4 PROBE");
WUPS_PLUGIN_DESCRIPTION("Read-only MCP title-list and Homebrew-on-Menu order diagnostic");
WUPS_PLUGIN_VERSION("v0.4.0-probe");
WUPS_PLUGIN_AUTHOR("TopCat");
WUPS_PLUGIN_LICENSE("MIT");
WUPS_USE_WUT_DEVOPTAB();

#define MARKER_PATH "fs:/vol/external01/wiiu/library_profiles/enable-v04-probe.txt"
#define MARKER_TEXT "ENABLE-V0.4-READONLY-PROBE\n"
#define REPORT_PATH "fs:/vol/external01/wiiu/library_profiles/v04-hook-report.csv"
static bool checked_marker=false;
static bool reporting=false;
static unsigned total_calls=0;
static unsigned max_synthetic=0;
static bool wrote_first=false;
static unsigned writes=0;

static bool is_menu(void){
    uint64_t tid=OSGetTitleID();
    return tid==UINT64_C(0x0005001010040000) ||
           tid==UINT64_C(0x0005001010040100) ||
           tid==UINT64_C(0x0005001010040200);
}
static bool marker_present(void){
    FILE *f=fopen(MARKER_PATH,"rb");
    if(!f) return false;
    char buf[sizeof(MARKER_TEXT)+1]={0};
    size_t n=fread(buf,1,sizeof(buf),f);
    int rc=fclose(f);
    return rc==0 && n==sizeof(MARKER_TEXT)-1 &&
           memcmp(buf,MARKER_TEXT,sizeof(MARKER_TEXT)-1)==0;
}
static void log_counts(int32_t rc,uint32_t count,uint32_t cap,
                      uint32_t base_games,uint32_t synthetic,uint32_t addons){
    const char *mode=wrote_first?"ab":"wb";
    FILE *f=fopen(REPORT_PATH,mode);
    if(!f) return;
    if(!wrote_first) {
        fputs("call,return_code,title_count,buffer_capacity,base_games,synthetic_homebrew,updates_dlc\n",f);
    }
    fprintf(f,"%u,%ld,%lu,%lu,%lu,%lu,%lu\n",
            total_calls,(long)rc,(unsigned long)count,
            (unsigned long)cap,(unsigned long)base_games,
            (unsigned long)synthetic,(unsigned long)addons);
    if(fclose(f)==0){wrote_first=true;writes++;}
}
ON_APPLICATION_START(){
    checked_marker=false;
    reporting=false;
    total_calls=0;
    max_synthetic=0;
    wrote_first=false;
    writes=0;
}
DECL_FUNCTION(int32_t,MCP_TitleList,
              uint32_t handle,uint32_t *outTitleCount,
              MCPTitleListType *list,uint32_t size){
    int32_t rc=real_MCP_TitleList(handle,outTitleCount,list,size);
    if(!is_menu())return rc;
    if(!checked_marker){
        reporting=marker_present();
        checked_marker=true;
    }
    if(!reporting)return rc;
    total_calls++;
    if(!list || !outTitleCount || rc!=0)return rc;
    uint32_t count=*outTitleCount;
    uint32_t cap=size / sizeof(MCPTitleListType);
    /* Don't touch or enumerate invalid buffers. No logging if unsafe. */
    if(count==0 || count>10000 || count>cap)return rc;
    uint32_t synthetic=0,base=0,addons=0;
    for(uint32_t i=0;i<count;i++){
        uint32_t upper=(uint32_t)(list[i].titleId>>32);
        if(upper==UINT32_C(0x0005000f))synthetic++;
        else if(upper==UINT32_C(0x00050000))base++;
        else if(upper==UINT32_C(0x0005000c) ||
                upper==UINT32_C(0x0005000e))addons++;
    }
    /* Only write at most twice: initial valid call and first increase in
     * homebrew count. Reboots reset the report to avoid unbounded SD writes.
     * A zero synthetic count does NOT prove Homebrew is invisible at all
     * times or after our hook in every ordering configuration.
     */
    if(!wrote_first || (synthetic>max_synthetic && writes<2)){
        log_counts(rc,count,cap,base,synthetic,addons);
    }
    if(synthetic>max_synthetic)max_synthetic=synthetic;
    return rc;
}
WUPS_MUST_REPLACE_FOR_PROCESS(MCP_TitleList,
                              WUPS_LOADER_LIBRARY_COREINIT,
                              MCP_TitleList,
                              WUPS_FP_TARGET_PROCESS_WII_U_MENU);
