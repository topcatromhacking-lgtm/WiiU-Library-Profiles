/*
 * Wii U Library Profiles v0.8.1: account-guarded, hash-tolerant Menu filter.
 * Disabled until exact SD marker; binds to current account and selected
 * validated WLPA manifest. Barista SHA256 is advisory only: Menu can rewrite
 * its save without disabling profile filtering. All mandatory checks fail-open.
 * No NAND, USB, installation, or Barista writes.
 */
#include <wups.h>
#include <coreinit/mcp.h>
#include <coreinit/title.h>
#include <nn/act.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wlp_filter_core.h"
#include "wlp_sha256.h"

WUPS_PLUGIN_NAME("Library Profiles v081 HASH ADVISORY");
WUPS_PLUGIN_DESCRIPTION("EXPERIMENTAL account-bound Menu title allowlist; SD opt-in");
WUPS_PLUGIN_VERSION("v0.8.1-experimental");
WUPS_PLUGIN_AUTHOR("TopCat");
WUPS_PLUGIN_LICENSE("MIT");
WUPS_USE_WUT_DEVOPTAB();

#define ROOT "fs:/vol/external01/wiiu/library_profiles/v08"
#define MARKER ROOT "/ENABLE-V08-EXPERIMENT.txt"
#define MARKER_DATA "ENABLE-WLP-V08-EXPERIMENT\n"
#define CONFIG ROOT "/active-profile.cfg"
#define REPORT ROOT "/v08-menu-report.csv"
#define BARISTA_SIZE 130116u
static bool checked=false,enabled=false,manifest_ok=false,account_ok=false,barista_hash_match=false,reported=false;
static uint32_t target_account=0;
static char profile[8]={0},serial[32]={0};
static wlp_manifest selection={0};
static const char *status="NOT_ENABLED";

static bool menu_process(){
 uint64_t tid=OSGetTitleID();
 return tid==UINT64_C(0x0005001010040000)||tid==UINT64_C(0x0005001010040100)||tid==UINT64_C(0x0005001010040200);
}
static bool exact_marker(){
 FILE*f=fopen(MARKER,"rb");if(!f)return false;
 char b[sizeof(MARKER_DATA)+2]={0};size_t n=fread(b,1,sizeof(b),f);int rc=fclose(f);
 return rc==0 && n==sizeof(MARKER_DATA)-1 && memcmp(b,MARKER_DATA,sizeof(MARKER_DATA)-1)==0;
}
static bool read_cfg(){
 FILE*f=fopen(CONFIG,"rb");if(!f)return false;
 char lines[4][64]={{0}};bool ok=true;
 for(int i=0;i<4;i++)if(!fgets(lines[i],sizeof(lines[i]),f))ok=false;
 if(fgetc(f)!=EOF)ok=false;
 if(fclose(f)!=0)ok=false;
 if(!ok || strcmp(lines[0],"WLP-V08\n")!=0)return false;
 if(strncmp(lines[1],"account=",8)!=0 || strlen(lines[1])!=17 || lines[1][16]!='\n')return false;
 target_account=0;
 for(int i=8;i<16;i++){
  char c=lines[1][i];uint32_t d=(c>='0'&&c<='9')?(uint32_t)(c-'0'):(c>='A'&&c<='F')?(uint32_t)(c-'A'+10):16;
  if(d==16)return false;target_account=(target_account<<4)|d;
 }
 if((target_account&0x80000000u)==0)return false;
 if(strncmp(lines[2],"serial=",7)!=0)return false;
 size_t slen=strlen(lines[2]);if(slen<9||slen>=32||lines[2][slen-1]!='\n')return false;
 for(size_t i=7;i<slen-1;i++){
  char c=lines[2][i];if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')))return false;
 }
 memcpy(serial,lines[2]+7,slen-8);serial[slen-8]=0;
 if(strcmp(lines[3],"profile=wiiu\n")==0)strcpy(profile,"wiiu");
 else if(strcmp(lines[3],"profile=nes\n")==0)strcpy(profile,"nes");
 else return false;
 return true;
}
static bool read_manifest(){
 char path[256];if(snprintf(path,sizeof(path),ROOT "/%s.allowlist",profile)>=(int)sizeof(path))return false;
 FILE*f=fopen(path,"rb");if(!f)return false;
 uint8_t buf[WLP_MAX_MANIFEST_BYTES+1]={0};size_t n=fread(buf,1,sizeof(buf),f);
 int err=ferror(f),closed=fclose(f);if(err||closed!=0||n>WLP_MAX_MANIFEST_BYTES)return false;
 return wlp_manifest_read(buf,n,&selection)!=0;
}
static bool check_save_digest(){
 char path[300];
 if(snprintf(path,sizeof(path),"fs:/vol/external01/wiiu/homebrew_on_menu_plugin/%s/save/%08X/BaristaAccountSaveFile.dat",serial,(unsigned)target_account)>=(int)sizeof(path))return false;
 FILE*f=fopen(path,"rb");if(!f)return false;
 uint8_t chunk[2048];size_t got;size_t total=0;wlp_sha256_ctx sha;wlp_sha256_init(&sha);
 while((got=fread(chunk,1,sizeof(chunk),f))>0){
  total+=got;if(total>BARISTA_SIZE)break;wlp_sha256_update(&sha,chunk,got);
 }
 int error=ferror(f),closed=fclose(f);if(error||closed!=0||total!=BARISTA_SIZE)return false;
 uint8_t digest[32];wlp_sha256_final(&sha,digest);
 return memcmp(digest,selection.digest,32)==0;
}
static bool current_account_matches(){
 nn::act::Initialize();
 uint32_t actual=UINT32_C(0x80000000)|nn::act::GetPersistentId();
 nn::act::Finalize();
 return actual==target_account;
}
static void initialize_once(){
 checked=true;
 if(!exact_marker())return;
 enabled=true;
 if(!read_cfg()){status="INVALID_CONFIG";return;}
 if(!read_manifest()){status="INVALID_MANIFEST";return;}
 manifest_ok=true;
 if(!current_account_matches()){status="WRONG_ACCOUNT";return;}
 account_ok=true;
 /* Menu regularly rewrites Barista after game exit/cold boot. This check is
  * strictly diagnostic: an altered/missing save must NOT re-expose titles. */
 barista_hash_match=check_save_digest();
 status="READY";
}
static void report(uint32_t call_count,uint32_t cap,const wlp_scan*s,const char *result){
 if(reported)return;reported=true;
 FILE*f=fopen(REPORT,"wb");if(!f)return;
 fputs("plugin_version,status,account,profile,barista_hash,records,capacity,expected_games,found_games,expected_homebrew,found_homebrew,would_hide_games,would_hide_homebrew\n",f);
 fprintf(f,"0.8.1,%s,%08lX,%s,%s,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",result,
 (unsigned long)target_account,profile,barista_hash_match?"MATCH":"MISMATCH_OR_UNAVAILABLE",(unsigned long)call_count,(unsigned long)cap,
 (unsigned long)s->required_games,(unsigned long)s->found_games,
 (unsigned long)s->required_homebrew,(unsigned long)s->found_homebrew,
 (unsigned long)s->hide_games,(unsigned long)s->hide_homebrew);
 fclose(f);
}
ON_APPLICATION_START(){
 checked=enabled=manifest_ok=account_ok=barista_hash_match=reported=false;
 target_account=0;profile[0]=serial[0]=0;status="NOT_ENABLED";
 memset(&selection,0,sizeof(selection));
}
DECL_FUNCTION(int32_t,MCP_TitleList,uint32_t handle,uint32_t*outCount,MCPTitleListType*list,uint32_t size){
 int32_t rc=real_MCP_TitleList(handle,outCount,list,size);
 if(!menu_process())return rc;
 if(!checked)initialize_once();
 if(!enabled)return rc;
 wlp_scan empty={0};
 if(!manifest_ok||!account_ok){report(0,0,&empty,status);return rc;}
 if(!current_account_matches()){report(0,0,&empty,"ACCOUNT_CHANGED");return rc;}
 if(rc!=0||!list||!outCount)return rc;
 uint32_t count=*outCount,cap=size/sizeof(MCPTitleListType);
 if(!count||count>10000u||count>cap)return rc;
 wlp_record*observed=(wlp_record*)malloc((size_t)count*sizeof(wlp_record));
 if(!observed)return rc;
 for(uint32_t i=0;i<count;i++){
  observed[i].id=list[i].titleId;
  observed[i].app_type=(uint32_t)list[i].appType;
 }
 wlp_scan stats=wlp_scan_titles(&selection,observed,count);
 free(observed);
 if(!wlp_ready(&stats)){
  report(count,cap,&stats,"EXPECTED_TITLES_MISSING_NO_FILTER");
  return rc;
 }
 uint32_t write=0;
 for(uint32_t i=0;i<count;i++){
  if(wlp_would_hide(&selection,list[i].titleId,(uint32_t)list[i].appType))continue;
  if(write!=i)list[write]=list[i];
  write++;
 }
 *outCount=write;
 report(count,cap,&stats,"FILTER_APPLIED");
 return rc;
}
WUPS_MUST_REPLACE_FOR_PROCESS(MCP_TitleList,WUPS_LOADER_LIBRARY_COREINIT,MCP_TitleList,WUPS_FP_TARGET_PROCESS_WII_U_MENU);
