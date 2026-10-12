/*
 * Wii U Library Profiles v0.4.2: multiple-call MCP visibility diagnostic.
 * OBSERVATION ONLY: returns the original MCP list and count unchanged.
 * A single 32-bit opt-in marker enables short diagnostic CSV writes to SD.
 * Exact missing Title IDs are logged to a separate PRIVATE LOCAL CSV.
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
#include "diagnostic_policy.h"

WUPS_PLUGIN_NAME("Library Profiles - v042 DIAGNOSTIC");
WUPS_PLUGIN_DESCRIPTION("Read-only missing Title IDs across Menu MCP calls");
WUPS_PLUGIN_VERSION("v0.4.2-observation");
WUPS_PLUGIN_AUTHOR("TopCat");
WUPS_PLUGIN_LICENSE("MIT");
WUPS_USE_WUT_DEVOPTAB();

#define MARKER_PATH "fs:/vol/external01/wiiu/library_profiles/enable-v042-diagnostic.txt"
#define MARKER_TEXT "ENABLE-V0.4.2-READONLY\n"
#define REPORT_PATH "fs:/vol/external01/wiiu/library_profiles/v042-call-history.csv"
#define MISSING_PATH "fs:/vol/external01/wiiu/library_profiles/v042-missing-ids.csv"
#define WIIU_PATH "fs:/vol/external01/wiiu/library_profiles/v041-wiiu.allowlist"
#define NES_PATH "fs:/vol/external01/wiiu/library_profiles/v041-nes.allowlist"
#define MAX_CALLS_TO_INSPECT 64u
#define MAX_HISTORY_EVENTS 20u

static wlp_allowlist manifests[2];
static const char *const names[2]={"wiiu","nes"};
static bool checked=false,active=false,loaded=false,valid[2]={false,false};
static bool history_header=false,have_best=false,have_last=false;
static uint32_t inspected=0,events=0;
static uint32_t best_missing=0,best_base=0;
static uint32_t last_count=0,last_base=0,last_prefix=0,last_homebrew=0,last_missing=0;

static bool in_menu(void){
  uint64_t t=OSGetTitleID();
  return t==UINT64_C(0x0005001010040000)||
         t==UINT64_C(0x0005001010040100)||
         t==UINT64_C(0x0005001010040200);
}
static bool has_exact_marker(void){
  FILE *f=fopen(MARKER_PATH,"rb");if(!f)return false;
  char b[sizeof(MARKER_TEXT)+1]={0};
  size_t n=fread(b,1,sizeof(b),f);
  int rc=fclose(f);
  return rc==0&&n==sizeof(MARKER_TEXT)-1&&
         memcmp(b,MARKER_TEXT,sizeof(MARKER_TEXT)-1)==0;
}
static bool load_file(const char *path,wlp_allowlist *out){
  FILE *f=fopen(path,"rb");if(!f)return false;
  unsigned char buffer[WLP_MAX_MANIFEST_BYTES+1u];
  size_t n=fread(buffer,1,sizeof(buffer),f);
  int error=ferror(f);
  int rc=fclose(f);
  if(error||rc!=0||n>WLP_MAX_MANIFEST_BYTES)return false;
  return wlp_parse_allowlist(buffer,n,out)!=0;
}
static void log_history(
    uint32_t call_id,uint32_t n,uint32_t capacity,uint32_t base,uint32_t prefix,
    uint32_t synthetic,uint32_t addons,
    const wlp_dry_stats *w,const wlp_dry_stats *e){
  if(events>=MAX_HISTORY_EVENTS)return;
  FILE *f=fopen(REPORT_PATH,history_header?"ab":"wb");
  if(!f)return;
  if(!history_header){
    fputs("call,menu_records,buffer_capacity,recognized_game_app_type_records,all_00050000_prefix_records,"
          "synthetic_homebrew,updates_dlc,wiiu_found,wiiu_missing,"
          "nes_found,nes_missing,selected_hb_found_wiiu,"
          "selected_hb_missing_wiiu,selected_hb_found_nes,"
          "selected_hb_missing_nes\n",f);
  }
  fprintf(f,"%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
      (unsigned long)call_id,(unsigned long)n,(unsigned long)capacity,
      (unsigned long)base,(unsigned long)prefix,(unsigned long)synthetic,(unsigned long)addons,
      (unsigned long)w->present_game,(unsigned long)w->missing_game,
      (unsigned long)e->present_game,(unsigned long)e->missing_game,
      (unsigned long)w->present_homebrew,(unsigned long)w->missing_homebrew,
      (unsigned long)e->present_homebrew,(unsigned long)e->missing_homebrew);
  if(fclose(f)==0){history_header=true;events++;}
}
static int seen_id(uint64_t title_id,const wlp_observed *list,uint32_t count){
  for(uint32_t i=0;i<count;i++)
    if(list[i].tid==title_id)return 1;
  return 0;
}
/* Overwrite this report only when expected IDs become more visible.
 * Do not enumerate all of the user's installed titles; only missing
 * allowlisted ones, in a private SD report. */
static void write_missing_ids(uint32_t call_id,const wlp_observed *list,uint32_t count){
  FILE *f=fopen(MISSING_PATH,"wb");
  if(!f)return;
  fputs("best_snapshot_call,profile,missing_kind,title_id\n",f);
  for(unsigned p=0;p<2;p++){
    for(uint32_t i=0;i<manifests[p].count;i++){
      uint64_t tid=manifests[p].ids[i];
      if(seen_id(tid,list,count))continue;
      uint32_t hi=(uint32_t)(tid>>32);
      const char *kind=(hi==UINT32_C(0x0005000f))?"homebrew":
          (hi==UINT32_C(0x00050000))?"game":"system_not_in_this_call";
      fprintf(f,"%lu,%s,%s,%08lX%08lX\n",
          (unsigned long)call_id,names[p],kind,
          (unsigned long)(uint32_t)(tid>>32),
          (unsigned long)(uint32_t)tid);
    }
  }
  fclose(f);
}
static void write_problem(void){
  FILE *f=fopen(REPORT_PATH,"wb");
  if(!f)return;
  fputs("status,wiiu_manifest_valid,nes_manifest_valid\n",f);
  fprintf(f,"MANIFEST_INVALID_OR_MISSING,%u,%u\n",valid[0]?1u:0u,valid[1]?1u:0u);
  fclose(f);
  history_header=true;
}
ON_APPLICATION_START(){
  checked=false;active=false;loaded=false;
  valid[0]=valid[1]=false;
  memset(manifests,0,sizeof(manifests));
  inspected=events=best_missing=best_base=0;
  last_count=last_base=last_prefix=last_homebrew=last_missing=0;
  history_header=have_best=have_last=false;
}
DECL_FUNCTION(int32_t,MCP_TitleList,
              uint32_t handle,uint32_t *outTitleCount,
              MCPTitleListType *titles,uint32_t size){
  int32_t rc=real_MCP_TitleList(handle,outTitleCount,titles,size);
  if(!in_menu())return rc;
  if(!checked){active=has_exact_marker();checked=true;}
  if(!active||rc!=0||!outTitleCount||!titles||inspected>=MAX_CALLS_TO_INSPECT)
    return rc;
  uint32_t count=*outTitleCount;
  uint32_t capacity=size/sizeof(MCPTitleListType);
  if(!count||count>10000u||count>capacity)return rc;
  if(!loaded){
    valid[0]=load_file(WIIU_PATH,&manifests[0]);
    valid[1]=load_file(NES_PATH,&manifests[1]);
    loaded=true;
    if(!valid[0]||!valid[1]){
      write_problem();
      return rc;
    }
  }
  if(!valid[0]||!valid[1])return rc;
  ++inspected;
  wlp_observed *list=malloc((size_t)count*sizeof(*list));
  if(!list)return rc;
  uint32_t base=0,prefix=0,homebrew=0,addons=0;
  for(uint32_t i=0;i<count;i++){
    list[i].tid=titles[i].titleId;
    list[i].app_type=(uint32_t)titles[i].appType;
    uint32_t hi=(uint32_t)(list[i].tid>>32);
    if(hi==UINT32_C(0x00050000))prefix++;
    if(wlp_is_base_game(list[i].tid,list[i].app_type))base++;
    else if(wlp_is_synthetic(list[i].tid))homebrew++;
    else if(hi==UINT32_C(0x0005000c)||hi==UINT32_C(0x0005000e))addons++;
  }
  wlp_dry_stats w=wlp_dry_evaluate(&manifests[0],list,count);
  wlp_dry_stats n=wlp_dry_evaluate(&manifests[1],list,count);
  uint32_t missing=w.missing_game+w.missing_homebrew+
                    n.missing_game+n.missing_homebrew;
  if(!have_last||last_count!=count||last_base!=base||last_prefix!=prefix||
     last_homebrew!=homebrew||last_missing!=missing){
    log_history(inspected,count,capacity,base,prefix,homebrew,addons,&w,&n);
    last_count=count;last_base=base;last_prefix=prefix;
    last_homebrew=homebrew;last_missing=missing;have_last=true;
  }
  if(wlp_better_snapshot(have_best,missing,prefix,best_missing,best_base)){
    write_missing_ids(inspected,list,count);
    best_missing=missing;best_base=prefix;have_best=true;
  }
  free(list);
  /* Deliberately no modification to titles, count, or original rc. */
  return rc;
}
WUPS_MUST_REPLACE_FOR_PROCESS(MCP_TitleList,
                              WUPS_LOADER_LIBRARY_COREINIT,
                              MCP_TitleList,
                              WUPS_FP_TARGET_PROCESS_WII_U_MENU);
