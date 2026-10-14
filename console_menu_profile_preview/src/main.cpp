/*
 * Wii U Library Profiles v1.2: optional read-only profile preview in
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
#include <errno.h>
#include "wlp_filter_core.h"
#include "wlp_sha256.h"
#include "wlp_inventory_core.h"
#include "wlpdb_core.h"
#include "wlp_game_set.h"
#include "wlps_core.h"

WUPS_PLUGIN_NAME("Library Profiles v12 MENU PREVIEW");
WUPS_PLUGIN_DESCRIPTION("Stable title filter plus optional read-only SD profile preview");
WUPS_PLUGIN_VERSION("v1.2-menu-preview-experimental");
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
#define DB_EVENTS ROOT "/v111-db-events.csv"
#define DB_OUTPUT_ROOT ROOT "/v111-db-audit-"
#define PREVIEW_MARKER ROOT "/ENABLE-V12-PREVIEW.txt"
#define PREVIEW_MARKER_DATA "ENABLE-WLP-V12-PREVIEW\n"
#define PREVIEW_PATH "fs:/vol/external01/wiiu/library_profiles/profiles/active.wlps"
#define PREVIEW_PREFIX ROOT "/v12-preview-"
#define PREVIEW_STATUS ROOT "/v12-preview-status.csv"
#define PREVIEW_EVENTS ROOT "/v12-preview-events.csv"
#define PREVIEW_MAX_SIZE (WLPS_HEADER_BYTES + 16u*WLPS_MAX_IDS)
/* Operational cap, independent of the decoder's wider format limit. */
#define DB_MAX_FILE_BYTES (256u * 1024u)
#define DB_MAX_RECORDS 4096u
static bool checked=false,enabled=false,manifest_ok=false,account_ok=false,barista_hash_match=false,reported=false;
static bool inventory_opted_in=false,inventory_io_failed=false;
static uint32_t inventory_best_count=0;
static bool db_opted_in=false,db_write_failed=false,db_status_reported=false;
static uint8_t db_last_fingerprint[32]={0};
static bool db_has_last_fingerprint=false;
static uint8_t *db_bytes=nullptr;
static wlpdb_view db_view={0};
static const char *db_state="NOT_REQUESTED";
static bool preview_opted_in=false, preview_failed=false, preview_status_reported=false;
static uint8_t *preview_bytes=nullptr;
static wlps_view preview_spec={0};
static uint32_t preview_file_crc=0;
static const char *preview_state="NOT_REQUESTED";
static uint8_t preview_last_hash[32]={0};
static bool preview_has_hash=false;
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
 fputs("stage,db_state,result,db_records,raw_records,games,confirmed,review,unknown\n",f);
 fprintf(f,"V11,%s,%s,%lu,%lu,%lu,%lu,%lu,%lu\n",
         db_state,result,(unsigned long)db_view.count,
         (unsigned long)raw_count,(unsigned long)games,
         (unsigned long)confirmed,(unsigned long)review,(unsigned long)missing);
 fclose(f);
}
/* This is a metadata report only. It MUST NEVER control filtering, Barista,
 * title installation status or change the existing allowlist behavior. */
/* Append-only diagnostic events preserve previous results and capture the actual
 * failing operation and errno. Best effort: never affect profile filtering. */
static void db_event(const char *step,int e,uint32_t raw,uint32_t games,const char *fingerprint){
 FILE *f=fopen(DB_EVENTS,"ab");if(!f)return;
 fprintf(f,"V111,%s,%d,%lu,%lu,%s\n",step,e,
   (unsigned long)raw,(unsigned long)games,fingerprint?fingerprint:"");
 fclose(f);
}
static void db_audit_capture(const MCPTitleListType *list,uint32_t count,uint32_t capacity,
                             const wlp_scan *stats){
 if(!db_opted_in || !stats || !wlp_ready(stats) || !list ||
    count==0 || count>capacity || count>WLP_INVENTORY_MAX_RECORDS)return;
 if(!db_bytes){
  if(!db_status_reported){
   db_write_status("DB_UNAVAILABLE_FILTER_UNCHANGED",count,0,0,0,0);
   db_event("DB_UNAVAILABLE",0,count,0,"");
   db_status_reported=true;
  }
  return;
 }
 if(db_write_failed)return;
 /* Compare only installed GAME Title IDs, never the unstable raw MCP count. */
 uint64_t *game_ids=(uint64_t*)malloc((size_t)count*sizeof(uint64_t));
 if(!game_ids){db_write_failed=true;db_event("ALLOCATION_FAILED",ENOMEM,count,0,"");return;}
 uint32_t n=0;
 for(uint32_t i=0;i<count;i++)
  if(wlp_is_game(list[i].titleId,(uint32_t)list[i].appType))
   game_ids[n++]=list[i].titleId;
 uint8_t fingerprint[32]={0};
 if(!wlp_game_set_hash(game_ids,n,fingerprint)){
  free(game_ids);db_write_failed=true;
  db_event("GAME_SET_HASH_FAILED",EINVAL,count,n,"");
  return;
 }
 free(game_ids);
 if(db_has_last_fingerprint && memcmp(fingerprint,db_last_fingerprint,32)==0)return;
 char hash[65];wlp_hash_hex(fingerprint,hash);
 /* The DB CRC is part of the filename: reclassification can generate a new
  * diagnostic even when installed games do not change. */
 uint32_t master_crc=wlpdb_u32(db_bytes+16);
 char final[250],part[260];
 int len=snprintf(final,sizeof(final),DB_OUTPUT_ROOT "%s-%08lX.csv",
                  hash,(unsigned long)master_crc);
 if(len<0||(size_t)len>=sizeof(final)){db_write_failed=true;return;}
 len=snprintf(part,sizeof(part),"%s.part",final);
 if(len<0||(size_t)len>=sizeof(part)){db_write_failed=true;return;}
 /* A completed immutable snapshot is never replaced (not even across boots).
  * This sidesteps FAT/devoptab rename-over-existing limitations. */
 errno=0;
 FILE *existing=fopen(final,"rb");
 if(existing){
  int c=fclose(existing);
  if(c!=0){db_event("EXISTING_CLOSE_FAILED",errno,count,n,hash);db_write_failed=true;return;}
  memcpy(db_last_fingerprint,fingerprint,32);db_has_last_fingerprint=true;
  uint32_t confirmed=0,review=0,missing=0;
  for(uint32_t i=0;i<count;i++){
   uint64_t tid=list[i].titleId;
   if(!wlp_is_game(tid,(uint32_t)list[i].appType))continue;
   wlpdb_result info={0};
   if(!wlpdb_find(&db_view,tid,&info))missing++;
   else if(info.confirmed)confirmed++;
   else review++;
  }
  db_event("UNCHANGED_ALREADY_SAVED",0,count,n,hash);
  db_write_status("SNAPSHOT_ALREADY_SAVED_FILTER_UNCHANGED",count,n,confirmed,review,missing);
  return;
 }
 if(errno!=ENOENT){
  int e=errno;
  db_event("DESTINATION_LOOKUP_FAILED",e,count,n,hash);
  db_write_failed=true;
  db_write_status("DESTINATION_LOOKUP_FAILED_FILTER_UNCHANGED",count,n,0,0,0);
  return;
 }
 errno=0;
 FILE *f=fopen(part,"wb");
 if(!f){
  int e=errno;db_write_failed=true;db_event("PART_OPEN_FAILED",e,count,n,hash);
  db_write_status("PART_OPEN_FAILED_FILTER_UNCHANGED",count,n,0,0,0);
  return;
 }
 fputs("title_id,app_type_hex,db_match,platform,region_mask,verified,name,product_code\n",f);
 uint32_t games=0,confirmed=0,review=0,missing=0;
 for(uint32_t i=0;i<count;i++){
  uint64_t tid=list[i].titleId;
  uint32_t app=(uint32_t)list[i].appType;
  if(!wlp_is_game(tid,app))continue;
  ++games;
  wlpdb_result info={0};
  bool found=wlpdb_find(&db_view,tid,&info)!=0;
  const char *match=found?(info.confirmed?"CONFIRMED":"REVIEW"):"UNKNOWN";
  if(!found)++missing;
  else if(info.confirmed)++confirmed;
  else ++review;
  fprintf(f,"%08lX%08lX,%08lX,%s,%s,%02X,%u,",
   (unsigned long)(uint32_t)(tid>>32),(unsigned long)(uint32_t)tid,
   (unsigned long)app,match,found?db_platform_name(info.platform):"UNKNOWN",
   found?info.region_mask:0u,found?info.confirmed:0u);
  if(found)db_csv_text(f,info.name,info.name_len);
  else fputs("\"\"",f);
  fputc(',',f);
  if(found)db_csv_text(f,info.product_code,info.code_len);
  else fputs("\"\"",f);
  fputc('\n',f);
 }
 int io_error=ferror(f);
 int write_errno=io_error?errno:0;
 errno=0;
 int closed=fclose(f);
 int close_errno=closed!=0?errno:0;
 if(io_error || closed!=0){
  db_write_failed=true;
  db_event(io_error?"PART_WRITE_FAILED":"PART_CLOSE_FAILED",
           io_error?(write_errno?write_errno:EIO):close_errno,count,games,hash);
  db_write_status("AUDIT_WRITE_FAILED_FILTER_UNCHANGED",count,games,confirmed,review,missing);
  return;
 }
 errno=0;
 if(rename(part,final)!=0){
  int e=errno;
  db_write_failed=true;
  db_event("RENAME_FAILED",e,count,games,hash);
  db_write_status("AUDIT_RENAME_FAILED_FILTER_UNCHANGED",count,games,confirmed,review,missing);
  return;
 }
 memcpy(db_last_fingerprint,fingerprint,32);db_has_last_fingerprint=true;
 db_event("SNAPSHOT_SAVED",0,count,games,hash);
 db_write_status("AUDIT_OK_FILTER_UNCHANGED",count,games,confirmed,review,missing);
}


static bool preview_marker_present(){
 FILE *f=fopen(PREVIEW_MARKER,"rb");
 if(!f)return false;
 char b[sizeof(PREVIEW_MARKER_DATA)+2]={0};
 size_t n=fread(b,1,sizeof(b),f);int close_error=fclose(f);
 return close_error==0 && n==sizeof(PREVIEW_MARKER_DATA)-1 &&
        memcmp(b,PREVIEW_MARKER_DATA,sizeof(PREVIEW_MARKER_DATA)-1)==0;
}
/* One-time, bounded load; no JSON parser or per-title reads inside Menu. */
static void preview_load_once(){
 FILE *f=fopen(PREVIEW_PATH,"rb");
 if(!f){preview_state="PROFILE_NOT_FOUND";return;}
 uint8_t header[WLPS_HEADER_BYTES]={0};
 if(fread(header,1,sizeof(header),f)!=sizeof(header)){
  preview_state="PROFILE_SHORT_HEADER";fclose(f);return;
 }
 uint32_t total=wlpdb_u32(header+20);
 if(total<WLPS_HEADER_BYTES || total>PREVIEW_MAX_SIZE){
  preview_state="PROFILE_TOO_LARGE";fclose(f);return;
 }
 uint8_t *raw=(uint8_t*)malloc(total);
 if(!raw){preview_state="PROFILE_NO_MEMORY";fclose(f);return;}
 memcpy(raw,header,sizeof(header));
 size_t remain=total-WLPS_HEADER_BYTES;
 size_t got=fread(raw+WLPS_HEADER_BYTES,1,remain,f);
 int extra=fgetc(f), io=ferror(f),closed=fclose(f);
 if(got!=remain || extra!=EOF || io || closed!=0){
  preview_state="PROFILE_IO_OR_LENGTH";free(raw);return;
 }
 wlps_view view={0};
 if(!wlps_open(&view,raw,total)){
  preview_state="PROFILE_CRC_OR_FORMAT_INVALID";free(raw);return;
 }
 preview_bytes=raw;preview_spec=view;
 preview_file_crc=wlpdb_crc32(raw,total);
 preview_state="PROFILE_READY";
}
static void preview_event(const char *action,int error,const char *hash){
 FILE *f=fopen(PREVIEW_EVENTS,"ab");if(!f)return;
 fprintf(f,"V12,%s,%d,%s\n",action,error,hash?hash:"");
 fclose(f);
}
static void preview_status(const char *result,uint32_t raw,const wlps_stats *s){
 wlps_stats empty={0};if(!s)s=&empty;
 FILE *f=fopen(PREVIEW_STATUS,"wb");if(!f)return;
 fputs("stage,db_state,profile_state,result,raw_records,games,joined,confirmed,review,unknown,eligible,shown,overflow,missing_includes,policy_blocked,profile_crc,db_crc\n",f);
 fprintf(f,"V12,%s,%s,%s,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%08lX,%08lX\n",
  db_state,preview_state,result,(unsigned long)raw,
  (unsigned long)s->games,(unsigned long)s->joined,(unsigned long)s->confirmed,
  (unsigned long)s->review,(unsigned long)s->unknown,(unsigned long)s->eligible,
  (unsigned long)s->shown,(unsigned long)s->overflow,
  (unsigned long)s->requested_missing,(unsigned long)s->blocked,
  (unsigned long)preview_file_crc,
  (unsigned long)(db_bytes?wlpdb_u32(db_bytes+16):0));
 fclose(f);
}
static void preview_capture(const MCPTitleListType*list,uint32_t count,uint32_t cap,const wlp_scan*scan){
 if(!preview_opted_in || !list || !scan || !wlp_ready(scan) ||
    count==0 || count>cap || count>4096u || preview_failed)return;
 if(!db_bytes || !preview_bytes){
  if(!preview_status_reported){
   preview_status("PREVIEW_INPUT_UNAVAILABLE_FILTER_UNCHANGED",count,nullptr);
   preview_event("INPUT_UNAVAILABLE",0,"");
   preview_status_reported=true;
  }
  return;
 }
 uint64_t *ids=(uint64_t*)malloc((size_t)count*sizeof(*ids));
 wlps_item *out=(wlps_item*)malloc((size_t)count*sizeof(*out));
 if(!ids||!out){
  free(ids);free(out);preview_failed=true;preview_event("NO_MEMORY",ENOMEM,"");
  preview_status("PREVIEW_NO_MEMORY_FILTER_UNCHANGED",count,nullptr);
  return;
 }
 uint32_t games=0;
 for(uint32_t i=0;i<count;i++)if(wlp_is_game(list[i].titleId,(uint32_t)list[i].appType))
  ids[games++]=list[i].titleId;
 uint8_t digest[32]={0};
 if(!wlp_game_set_hash(ids,games,digest)){
  free(ids);free(out);preview_failed=true;preview_event("GAME_SET_INVALID",EINVAL,"");
  preview_status("PREVIEW_GAME_SET_INVALID_FILTER_UNCHANGED",count,nullptr);
  return;
 }
 if(preview_has_hash && memcmp(preview_last_hash,digest,32)==0){
  free(ids);free(out);return;
 }
 wlps_stats stats={0};
 if(!wlps_plan(&preview_spec,&db_view,ids,games,out,count,&stats)){
  free(ids);free(out);preview_failed=true;preview_event("PROFILE_PLAN_INVALID",EINVAL,"");
  preview_status("PREVIEW_PLAN_INVALID_FILTER_UNCHANGED",count,nullptr);
  return;
 }
 free(ids);
 char hash[65];wlp_hash_hex(digest,hash);
 char path[280],part[290];
 int z=snprintf(path,sizeof(path),PREVIEW_PREFIX "%s-%08lX-%08lX.csv",
   hash,(unsigned long)preview_file_crc,
   (unsigned long)wlpdb_u32(db_bytes+16));
 if(z<0 || (size_t)z>=sizeof(path)){free(out);preview_failed=true;return;}
 z=snprintf(part,sizeof(part),"%s.part",path);
 if(z<0 || (size_t)z>=sizeof(part)){free(out);preview_failed=true;return;}
 errno=0;
 FILE *existing=fopen(path,"rb");
 if(existing){
  int done=fclose(existing);
  if(done!=0){preview_failed=true;preview_event("EXISTING_CLOSE_FAILED",errno,hash);}
  else {
   memcpy(preview_last_hash,digest,32);preview_has_hash=true;
   preview_event("UNCHANGED_ALREADY_SAVED",0,hash);
   preview_status("PREVIEW_ALREADY_SAVED_NO_BARISTA_WRITES",count,&stats);
  }
  free(out);return;
 }
 if(errno!=ENOENT){
  int error=errno;preview_failed=true;preview_event("LOOKUP_FAILED",error,hash);
  preview_status("PREVIEW_LOOKUP_FAILED_FILTER_UNCHANGED",count,&stats);free(out);return;
 }
 errno=0;
 FILE *f=fopen(part,"wb");
 if(!f){
  int error=errno;preview_failed=true;preview_event("PART_OPEN_FAILED",error,hash);
  preview_status("PREVIEW_OPEN_FAILED_FILTER_UNCHANGED",count,&stats);free(out);return;
 }
 fputs("selection,menu_position,title_id,platform,region_mask,manual_inclusion,name\n",f);
 for(uint32_t i=0;i<stats.eligible;i++){
  const wlps_item *entry=&out[i];
  wlpdb_result meta={0};
  if(!wlpdb_find(&db_view,entry->id,&meta))continue;
  if(i<WLPS_ICON_LIMIT)fprintf(f,"PREVIEW,%lu,",16ul+(unsigned long)i);
  else fputs("OVERFLOW,,",f);
  fprintf(f,"%08lX%08lX,%s,%02X,%u,",
      (unsigned long)(uint32_t)(entry->id>>32),
      (unsigned long)(uint32_t)entry->id,
      db_platform_name(entry->platform),entry->region,entry->manual);
  db_csv_text(f,meta.name,meta.name_len);fputc('\n',f);
 }
 int err=ferror(f),write_error=err?errno:0;
 errno=0;int closed=fclose(f),close_error=closed?errno:0;
 if(err||closed){
  preview_failed=true;
  preview_event(err?"PART_WRITE_FAILED":"PART_CLOSE_FAILED",
      err?(write_error?write_error:EIO):close_error,hash);
  preview_status("PREVIEW_WRITE_FAILED_FILTER_UNCHANGED",count,&stats);free(out);return;
 }
 errno=0;
 if(rename(part,path)!=0){
  int error=errno;preview_failed=true;preview_event("RENAME_FAILED",error,hash);
  preview_status("PREVIEW_RENAME_FAILED_FILTER_UNCHANGED",count,&stats);free(out);return;
 }
 preview_event("PREVIEW_SAVED",0,hash);
 preview_status("PREVIEW_OK_NO_BARISTA_WRITES",count,&stats);
 memcpy(preview_last_hash,digest,32);preview_has_hash=true;
 free(out);
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
 preview_opted_in=preview_marker_present();
 if(db_opted_in || preview_opted_in)db_load_once();
 if(preview_opted_in){
  if(db_bytes)preview_load_once();
  else preview_state="DB_UNAVAILABLE";
 }
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
 if(preview_bytes)free(preview_bytes);
 preview_bytes=nullptr;memset(&preview_spec,0,sizeof(preview_spec));
 preview_opted_in=preview_failed=preview_status_reported=preview_has_hash=false;
 preview_file_crc=0;preview_state="NOT_REQUESTED";
 memset(preview_last_hash,0,sizeof(preview_last_hash));
 db_opted_in=db_write_failed=db_status_reported=false;
 db_has_last_fingerprint=false;memset(db_last_fingerprint,0,sizeof(db_last_fingerprint));db_state="NOT_REQUESTED";
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
 /* Read-only preview: does not influence Barista or the proven filtering loop. */
 preview_capture(list,count,cap,&stats);
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
