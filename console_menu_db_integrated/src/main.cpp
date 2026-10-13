/*
 * Wii U Library Profiles v1.1: optional SD master metadata classification in
 * hardware-proven v0.8.1 Menu title hook, preserving original filter.
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
#include "wlp_inventory_core.h"
#include "wlpdb_core.h"

WUPS_PLUGIN_NAME("Library Profiles v11 SD DATABASE READ");
WUPS_PLUGIN_DESCRIPTION("v0.8.1 filter, v1.0 snapshot and optional read-only DB join");
WUPS_PLUGIN_VERSION("v1.1-db-read-only-experimental");
WUPS_PLUGIN_AUTHOR("TopCat");
WUPS_PLUGIN_LICENSE("MIT");
WUPS_USE_WUT_DEVOPTAB();

#define ROOT "fs:/vol/external01/wiiu/library_profiles/v08"
#define MARKER ROOT "/ENABLE-V08-EXPERIMENT.txt"
#define MARKER_DATA "ENABLE-WLP-V08-EXPERIMENT\n"
#define CONFIG ROOT "/active-profile.cfg"
#define REPORT ROOT "/v08-menu-report.csv"
#define BARISTA_SIZE 130116u
#define INVENTORY_MARKER ROOT "/ENABLE-V10-INVENTORY.txt"
#define INVENTORY_MARKER_DATA "ENABLE-WLP-V10-INVENTORY\n"
/* Same already-proven devoptab mount as v0.8.1 report, no WHB SD mount. */
#define INVENTORY_TMP ROOT "/v10-inventory.csv.part"
#define INVENTORY_CSV ROOT "/v10-inventory.csv"
#define DB_MARKER ROOT "/ENABLE-V11-DB-READ.txt"
#define DB_MARKER_DATA "ENABLE-WLP-V11-DB-READ\n"
#define DB_PATH "fs:/vol/external01/wiiu/library_profiles/db/master_titles.wlpdb"
#define DB_AUDIT_TMP ROOT "/v11-db-audit.csv.part"
#define DB_AUDIT ROOT "/v11-db-audit.csv"
#define DB_STATUS ROOT "/v11-db-status.csv"
/* Operational cap, independent of the decoder's wider format limit. */
#define DB_MAX_FILE_BYTES (256u * 1024u)
#define DB_MAX_RECORDS 4096u
static bool checked=false,enabled=false,manifest_ok=false,account_ok=false,barista_hash_match=false,reported=false;
static bool inventory_opted_in=false,inventory_io_failed=false;
static uint32_t inventory_best_count=0;
static bool db_opted_in=false,db_write_failed=false,db_status_reported=false;
static uint32_t db_best_count=0;
static uint8_t *db_bytes=nullptr;
static wlpdb_view db_view={0};
static const char *db_state="NOT_REQUESTED";
static uint32_t target_account=0;
static char profile[8]={0},serial[32]={0};
static wlp_manifest selection={0};
static const char *status="NOT_ENABLED";

static bool menu_process(){
 uint64_t tid=OSGetTitleID();
 return tid==UINT64_C(0x0005001010040000)||tid==UINT64_C(0x0005001010040100)||tid==UINT64_C(0x0005001010040200);
}
static bool inventory_marker_present(){
 FILE *f=fopen(INVENTORY_MARKER,"rb");
 if(!f)return false;
 char b[sizeof(INVENTORY_MARKER_DATA)+2]={0};
 size_t n=fread(b,1,sizeof(b),f);int rc=fclose(f);
 return rc==0 && n==sizeof(INVENTORY_MARKER_DATA)-1 &&
        memcmp(b,INVENTORY_MARKER_DATA,sizeof(INVENTORY_MARKER_DATA)-1)==0;
}

/* Full binary load happens at most once per Wii U Menu process, after existing
 * account and manifest authorization. The title loop only performs lookups. */
static bool db_marker_present(){
 FILE *f=fopen(DB_MARKER,"rb");if(!f)return false;
 char b[sizeof(DB_MARKER_DATA)+2]={0};
 size_t n=fread(b,1,sizeof(b),f);int closed=fclose(f);
 return closed==0 && n==sizeof(DB_MARKER_DATA)-1 &&
        memcmp(b,DB_MARKER_DATA,sizeof(DB_MARKER_DATA)-1)==0;
}
static void db_load_once(){
 FILE *f=fopen(DB_PATH,"rb");
 if(!f){db_state="DB_NOT_FOUND";return;}
 uint8_t header[WLPDB_HEADER_SIZE]={0};
 if(fread(header,1,sizeof(header),f)!=sizeof(header)){
  db_state="SHORT_HEADER";fclose(f);return;
 }
 if(memcmp(header,WLPDB_MAGIC,4)!=0 ||
    wlpdb_u16(header+4)!=1 || wlpdb_u16(header+6)!=WLPDB_RECORD_SIZE){
  db_state="BAD_FORMAT";fclose(f);return;
 }
 uint32_t n=wlpdb_u32(header+8),pool=wlpdb_u32(header+12);
 /* Size guards precede allocation, to avoid overflow and surprise SD loads. */
 if(n>DB_MAX_RECORDS || pool>DB_MAX_FILE_BYTES ||
    (size_t)n>(DB_MAX_FILE_BYTES-WLPDB_HEADER_SIZE)/WLPDB_RECORD_SIZE ||
    (size_t)pool>DB_MAX_FILE_BYTES-WLPDB_HEADER_SIZE-(size_t)n*WLPDB_RECORD_SIZE){
  db_state="DB_TOO_LARGE";fclose(f);return;
 }
 size_t size=WLPDB_HEADER_SIZE+(size_t)n*WLPDB_RECORD_SIZE+(size_t)pool;
 uint8_t *raw=(uint8_t*)malloc(size);
 if(!raw){db_state="DB_NO_MEMORY";fclose(f);return;}
 memcpy(raw,header,sizeof(header));
 size_t remaining=size-WLPDB_HEADER_SIZE;
 size_t received=fread(raw+WLPDB_HEADER_SIZE,1,remaining,f);
 int extra=fgetc(f),err=ferror(f),closed=fclose(f);
 if(received!=remaining || extra!=EOF || err || closed!=0){
  db_state="DB_IO_OR_LENGTH";free(raw);return;
 }
 wlpdb_view view={0};
 if(!wlpdb_open(&view,raw,size)){
  db_state="DB_CRC_OR_RECORD_INVALID";free(raw);return;
 }
 db_bytes=raw;db_view=view;db_state="DB_READY";
}
static const char *db_platform_name(uint8_t code){
 static const char *names[]={"UNKNOWN","WIIU","WII","NDS","GBA","N64","SNES","PCE","MSX","NES"};
 return code<10?names[code]:"INVALID";
}
static void db_csv_text(FILE *f,const uint8_t *text,uint16_t len){
 fputc('"',f);
 for(uint16_t i=0;i<len;i++){
  unsigned char c=text[i];
  if(c=='"')fputc('"',f);
  if(c>=0x20 && c!=0x7f || c=='"')fputc(c,f);
  else fputc(' ',f); /* flatten embedded CR/LF in names, one physical CSV row */
 }
 fputc('"',f);
}
static void db_write_status(const char *result,uint32_t raw_count,uint32_t games,
                            uint32_t confirmed,uint32_t review,uint32_t missing){
 FILE *f=fopen(DB_STATUS,"wb");
 if(!f)return;
 fputs("stage,db_state,result,db_records,raw_records,games,confirmed,review,unknown\\n",f);
 fprintf(f,"V11,%s,%s,%lu,%lu,%lu,%lu,%lu,%lu\\n",
         db_state,result,(unsigned long)db_view.count,
         (unsigned long)raw_count,(unsigned long)games,
         (unsigned long)confirmed,(unsigned long)review,(unsigned long)missing);
 fclose(f);
}
/* This is a metadata report only. It MUST NEVER control filtering, Barista,
 * title installation status or change the existing allowlist behavior. */
static void db_audit_capture(const MCPTitleListType *list,uint32_t count,uint32_t capacity,
                             const wlp_scan *stats){
 if(!db_opted_in || !stats || !wlp_ready(stats) || !list ||
    count==0 || count>capacity || count>WLP_INVENTORY_MAX_RECORDS)return;
 if(!db_bytes){
  if(!db_status_reported){
   db_write_status("DB_UNAVAILABLE_FILTER_UNCHANGED",count,0,0,0,0);
   db_status_reported=true;
  }
  return;
 }
 if(db_write_failed || count<=db_best_count)return;
 FILE *f=fopen(DB_AUDIT_TMP,"wb");
 if(!f){
  db_write_failed=true;
  db_write_status("AUDIT_OPEN_FAILED_FILTER_UNCHANGED",count,0,0,0,0);
  return;
 }
 fputs("title_id,app_type_hex,db_match,platform,region_mask,verified,name,product_code\\n",f);
 uint32_t games=0,confirmed=0,review=0,missing=0;
 for(uint32_t i=0;i<count;i++){
  uint64_t id=list[i].titleId;
  uint32_t app=(uint32_t)list[i].appType;
  if(!wlp_is_game(id,app))continue; /* updates/DLC/system do not consume icon slots */
  ++games;
  wlpdb_result info={0};
  bool found=wlpdb_find(&db_view,id,&info)!=0;
  const char *match=found?(info.confirmed?"CONFIRMED":"REVIEW"):"UNKNOWN";
  if(!found)++missing;
  else if(info.confirmed)++confirmed;
  else ++review;
  fprintf(f,"%08lX%08lX,%08lX,%s,%s,%02X,%u,",
   (unsigned long)(uint32_t)(id>>32),(unsigned long)(uint32_t)id,
   (unsigned long)app,match,found?db_platform_name(info.platform):"UNKNOWN",
   found?info.region_mask:0u,found?info.confirmed:0u);
  if(found)db_csv_text(f,info.name,info.name_len);
  else fputs("\"\"",f);
  fputc(',',f);
  if(found)db_csv_text(f,info.product_code,info.code_len);
  else fputs("\"\"",f);
  fputc('\n',f);
 }
 int write_err=ferror(f),closed=fclose(f);
 if(write_err || closed!=0 || rename(DB_AUDIT_TMP,DB_AUDIT)!=0){
  db_write_failed=true;
  db_write_status("AUDIT_WRITE_FAILED_FILTER_UNCHANGED",count,games,confirmed,review,missing);
  return;
 }
 db_best_count=count;
 db_write_status("AUDIT_OK_FILTER_UNCHANGED",count,games,confirmed,review,missing);
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
 inventory_opted_in=inventory_marker_present();
 /* Separate exact opt-in: no DB operations when marker is absent. */
 db_opted_in=db_marker_present();
 if(db_opted_in)db_load_once();
 status="READY";
}
/* Bounded CSV strings: never assume the packed MCP fields are NUL-terminated. */
static void csv_quoted(FILE *f,const char *raw,size_t bound){
 fputc('"',f);
 for(size_t i=0;i<bound && raw[i];i++){
  unsigned char c=(unsigned char)raw[i];
  if(c=='"')fputc('"',f);
  if(c=='"'||c>=0x20u && c!=0x7fu)fputc(c,f);
  else fputc(' ',f);
 }
 fputc('"',f);
}
/* Called ONLY after full manifest readiness, before compacting the live list.
 * A failed capture has no bearing on profile filtering (fail open telemetry).
 * Use a temporary file so failure does not truncate last complete snapshot. */
static void inventory_capture(const MCPTitleListType*list,uint32_t count,uint32_t cap,const wlp_scan*stats){
 if(inventory_io_failed || !wlp_inventory_candidate(count,cap,inventory_best_count,stats,inventory_opted_in))return;
 FILE *f=fopen(INVENTORY_TMP,"wb");
 if(!f){inventory_io_failed=true;return;}
 fputs("title_id,app_type_hex,kind,platform_evidence,region_status,group_id_hex,title_version,sdk_version,indexed_device,install_path\n",f);
 for(uint32_t i=0;i<count;i++){
  const MCPTitleListType *p=&list[i];uint64_t id=p->titleId;
  fprintf(f,"%08lX%08lX,%08lX,%s,%s,UNKNOWN,%08lX,%u,%lu,",
          (unsigned long)(uint32_t)(id>>32),(unsigned long)(uint32_t)id,
          (unsigned long)(uint32_t)p->appType,
          wlp_inventory_kind(id,(uint32_t)p->appType),
          wlp_inventory_platform_evidence(id,(uint32_t)p->appType),
          (unsigned long)p->groupId,(unsigned)p->titleVersion,
          (unsigned long)p->sdkVersion);
  csv_quoted(f,p->indexedDevice,sizeof(p->indexedDevice));fputc(',',f);
  csv_quoted(f,p->path,sizeof(p->path));fputc('\n',f);
 }
 int write_error=ferror(f);
 int close_error=fclose(f);
 if(write_error || close_error!=0 || rename(INVENTORY_TMP,INVENTORY_CSV)!=0){
  inventory_io_failed=true;
  /* Do not remove the .part file if the filesystem is malfunctioning;
     never attempt to alter Barista, cfg, manifests, or other user files. */
  return;
 }
 inventory_best_count=count;
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
 inventory_opted_in=inventory_io_failed=false;inventory_best_count=0;
 if(db_bytes)free(db_bytes);
 db_bytes=nullptr;memset(&db_view,0,sizeof(db_view));
 db_opted_in=db_write_failed=db_status_reported=false;
 db_best_count=0;db_state="NOT_REQUESTED";
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
 /* Observe ORIGINAL, UNFILTERED TitleList, before in-place compaction. */
 inventory_capture(list,count,cap,&stats);
 /* Read-only database join; no changes to allowlist compaction below. */
 db_audit_capture(list,count,cap,&stats);
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
