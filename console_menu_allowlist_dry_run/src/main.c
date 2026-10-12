/*
 * Wii U Library Profiles v0.4.1: paired allowlist DRY RUN, no icon filtering.
 *
 * Read two versioned, CRC-checked manifests from SD.
 * Compare each against titles the Menu actually receives.
 * Log only aggregate counts. NEVER edit returned MCP records.
 * No Barista/NAND/USB reads or writes; no game save changes.
 * Opt-in marker required, and report generated once per Menu session.
 */
#include <wups.h>
#include <coreinit/mcp.h>
#include <coreinit/title.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "allowlist_core.h"

WUPS_PLUGIN_NAME("Library Profiles - v041 DRY RUN");
WUPS_PLUGIN_DESCRIPTION("Read-only paired allowlist comparison, no filtering");
WUPS_PLUGIN_VERSION("v0.4.1-dry-run");
WUPS_PLUGIN_AUTHOR("TopCat");
WUPS_PLUGIN_LICENSE("MIT");
WUPS_USE_WUT_DEVOPTAB();

#define MARKER_PATH "fs:/vol/external01/wiiu/library_profiles/enable-v041-dry-run.txt"
#define MARKER_TEXT "ENABLE-V0.4.1-DRY-RUN\n"
#define REPORT_PATH "fs:/vol/external01/wiiu/library_profiles/v041-dry-run-report.csv"
#define WIIU_MANIFEST_PATH "fs:/vol/external01/wiiu/library_profiles/v041-wiiu.allowlist"
#define NES_MANIFEST_PATH "fs:/vol/external01/wiiu/library_profiles/v041-nes.allowlist"

typedef struct {
  const char *name;
  const char *path;
  wlp_allowlist parsed;
  bool valid;
} profile_state;
static profile_state profiles[2]={
  {.name="wiiu",.path=WIIU_MANIFEST_PATH},
  {.name="nes",.path=NES_MANIFEST_PATH}
};
static bool activation_checked=false;
static bool activation_enabled=false;
static bool report_attempted=false;

static bool menu_process(void){
  uint64_t t=OSGetTitleID();
  return t==UINT64_C(0x0005001010040000)||
         t==UINT64_C(0x0005001010040100)||
         t==UINT64_C(0x0005001010040200);
}
static bool exact_marker(void){
  FILE *f=fopen(MARKER_PATH,"rb");
  if(!f)return false;
  char b[sizeof(MARKER_TEXT)+1]={0};
  size_t n=fread(b,1,sizeof(b),f);
  int status=fclose(f);
  return status==0&&n==sizeof(MARKER_TEXT)-1&&
         memcmp(b,MARKER_TEXT,sizeof(MARKER_TEXT)-1)==0;
}
static bool load_allowlist(profile_state *p){
  FILE *f=fopen(p->path,"rb");
  if(!f)return false;
  unsigned char b[WLP_MAX_MANIFEST_BYTES+1u]={0};
  size_t n=fread(b,1,sizeof(b),f);
  int status=ferror(f);
  int close_status=fclose(f);
  if(status||close_status!=0||n>WLP_MAX_MANIFEST_BYTES)return false;
  return wlp_parse_allowlist(b,n,&p->parsed)!=0;
}
ON_APPLICATION_START(){
  activation_checked=false;
  activation_enabled=false;
  report_attempted=false;
  for(unsigned j=0;j<2;j++){
    profiles[j].valid=false;
    memset(&profiles[j].parsed,0,sizeof(profiles[j].parsed));
  }
}
/* All expensive work occurs only once after a successful and bounds-checked
 * MCP list. Manifest error rows are logged, not treated as authorization to
 * filter, and the actual MCP title array remains UNCHANGED.
 */
DECL_FUNCTION(int32_t,MCP_TitleList,uint32_t handle,
              uint32_t *outTitleCount,MCPTitleListType *titles,uint32_t size){
  int32_t rc=real_MCP_TitleList(handle,outTitleCount,titles,size);
  if(!menu_process())return rc;
  if(!activation_checked){
    activation_enabled=exact_marker();
    activation_checked=true;
  }
  if(!activation_enabled||report_attempted)return rc;
  if(rc!=0||!outTitleCount||!titles)return rc;
  uint32_t count=*outTitleCount;
  uint32_t capacity=size/sizeof(MCPTitleListType);
  if(count==0||count>10000||count>capacity)return rc;
  report_attempted=true;
  wlp_observed *observed=malloc((size_t)count*sizeof(*observed));
  if(!observed)return rc;
  for(uint32_t i=0;i<count;i++){
    observed[i].tid=titles[i].titleId;
    observed[i].app_type=(uint32_t)titles[i].appType;
  }
  for(unsigned j=0;j<2;j++)profiles[j].valid=load_allowlist(&profiles[j]);
  FILE *report=fopen(REPORT_PATH,"wb");
  if(report){
    fputs("profile,status,manifest_ids,menu_records,buffer_capacity,"
          "expected_game,found_game,missing_game,expected_homebrew,"
          "found_homebrew,missing_homebrew,expected_other,found_other,"
          "missing_other,menu_game_records,menu_homebrew_records,"
          "would_hide_games,would_hide_homebrew,would_leave_records\n",report);
    for(unsigned j=0;j<2;j++){
      profile_state *p=&profiles[j];
      wlp_dry_stats s={0};
      const char *status="INVALID_OR_MISSING_MANIFEST";
      if(p->valid){
        s=wlp_dry_evaluate(&p->parsed,observed,count);
        status=(s.missing_game==0&&s.missing_homebrew==0)?
               "ALL_TARGETS_FOUND":"MISSING_TARGETS";
      }
      uint32_t removed=s.would_hide_game+s.would_hide_homebrew;
      fprintf(report,"%s,%s,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,"
             "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
             p->name,status,(unsigned long)p->parsed.count,
             (unsigned long)count,(unsigned long)capacity,
             (unsigned long)s.expected_game,(unsigned long)s.present_game,
             (unsigned long)s.missing_game,
             (unsigned long)s.expected_homebrew,(unsigned long)s.present_homebrew,
             (unsigned long)s.missing_homebrew,
             (unsigned long)s.expected_other,(unsigned long)s.present_other,
             (unsigned long)s.missing_other,
             (unsigned long)s.game_records,(unsigned long)s.homebrew_records,
             (unsigned long)s.would_hide_game,(unsigned long)s.would_hide_homebrew,
             (unsigned long)(count-removed));
    }
    fclose(report);
  }
  free(observed);
  return rc;
}
WUPS_MUST_REPLACE_FOR_PROCESS(MCP_TitleList,
                              WUPS_LOADER_LIBRARY_COREINIT,
                              MCP_TitleList,
                              WUPS_FP_TARGET_PROCESS_WII_U_MENU);
