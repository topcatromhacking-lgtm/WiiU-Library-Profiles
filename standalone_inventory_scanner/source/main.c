/*
 * Wii U Library Profiles v1.0 Stage A
 * Standalone, read-only MCP + SD homebrew inventory collector.
 * NEVER edits Barista, MCP lists, title storage, homebrew files, or allowlists.
 * Only writes two CSVs and one summary to sd:/wiiu/library_profiles/v10/
 */
#include <coreinit/mcp.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/sdcard.h>
#include <vpad/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

#define OUT_DIR "sd:/wiiu/library_profiles/v10"
#define TITLE_OUT OUT_DIR "/v10-installed-titles.csv"
#define HB_OUT OUT_DIR "/v10-homebrew-apps.csv"
#define SUMMARY_OUT OUT_DIR "/v10-summary.txt"
#define MAX_TITLES 8192u
#define MAX_XML 65536u

static void csv_field(FILE *f, const char *s) {
    fputc('"',f);
    if(s) for(size_t i=0;s[i] && i<1024;i++) {
        unsigned char c=(unsigned char)s[i];
        if(c=='"')fputc('"',f);
        if(c=='"')fputc('"',f);
        else if(c=='\r'||c=='\n'||c==0x7f||c<0x20)fputc(' ',f);
        else fputc(c,f);
    }
    fputc('"',f);
}
static void copied(char *dst,size_t size,const char *src,size_t limit) {
    if(!dst||!size)return;
    if(!src){dst[0]=0;return;}
    size_t i=0;
    while(i+1<size && i<limit && src[i]) {dst[i]=src[i];i++;}
    dst[i]=0;
}
static const char *kind(uint64_t tid,uint32_t app) {
    uint32_t hi=(uint32_t)(tid>>32);
    if(hi==0x0005000f)return "HOMEBREW_SYNTHETIC";
    if(hi==0x0005000c||hi==0x0005000e)return "UPDATE_OR_DLC";
    if(hi==0x00050000&&(app==0x80000000u||app==0x8000002eu))return "GAME";
    return "OTHER";
}
static const char *platform(uint64_t tid,uint32_t app) {
    if((uint32_t)(tid>>32)==0x0005000f)return "HOMEBREW";
    if((uint32_t)(tid>>32)==0x00050000 && app==0x8000002eu)return "WII";
    if((uint32_t)(tid>>32)==0x00050000 && app==0x80000000u)
        return "WIIU_OR_VIRTUAL_CONSOLE_UNKNOWN";
    return "NOT_APPLICABLE";
}
static const char *area(uint32_t region) {
    if(region==0x01)return "JPN";
    if(region==0x02)return "USA";
    if(region==0x04)return "EUR";
    if(region==0)return "UNKNOWN";
    return "MULTI_OR_OTHER";
}
static int ends_with_ci(const char*s,const char*suffix){
    size_t n=strlen(s),m=strlen(suffix);
    if(m>n)return 0;
    for(size_t i=0;i<m;i++)
        if(tolower((unsigned char)s[n-m+i])!=tolower((unsigned char)suffix[i]))return 0;
    return 1;
}
static void xml_tag(char*out,size_t cap,const char*xml,const char*tag){
    out[0]=0;
    char open[40],close[40];
    snprintf(open,sizeof(open),"<%s",tag);
    snprintf(close,sizeof(close),"</%s>",tag);
    const char*start=strstr(xml,open);
    if(!start)return;
    start=strchr(start,'>');
    if(!start)return;
    start++;
    const char*end=strstr(start,close);
    if(!end)return;
    size_t n=(size_t)(end-start);
    if(n>=cap)n=cap-1;
    memcpy(out,start,n);
    out[n]=0;
}
static int read_meta(const char *base,char *name,size_t namecap,char *region,size_t regioncap){
    name[0]=region[0]=0;
    if(!base||!base[0]||strstr(base,".."))return 0;
    char path[384];
    if(snprintf(path,sizeof(path),"%s/meta/meta.xml",base)>=(int)sizeof(path))return 0;
    FILE*f=fopen(path,"rb");if(!f)return 0;
    char *b=malloc(MAX_XML+1);if(!b){fclose(f);return 0;}
    size_t n=fread(b,1,MAX_XML,f);int err=ferror(f);
    int overflow=0;
    if(n==MAX_XML){overflow=fgetc(f)!=EOF;}
    fclose(f);
    if(err||overflow||n==0){free(b);return 0;}
    b[n]=0;
    xml_tag(name,namecap,b,"longname_en");
    if(!name[0])xml_tag(name,namecap,b,"shortname_en");
    char v[32]={0};
    xml_tag(v,sizeof(v),b,"region");
    if(v[0]){
        char*end=0;unsigned long bits=strtoul(v,&end,16);
        if(end!=v && (!*end||isspace((unsigned char)*end)))
            copied(region,regioncap,area((uint32_t)bits),32);
    }
    free(b);return 1;
}
static int collect_titles(uint32_t *rows,uint32_t *games,uint32_t *synth,
                          uint32_t *named,uint32_t *regioned,
                          int *mcp_error) {
    *rows=*games=*synth=*named=*regioned=0;
    FILE*f=fopen(TITLE_OUT,"wb");if(!f)return 0;
    fputs("title_id,app_type_hex,kind,platform_inferred,platform_confidence,region_inferred,region_confidence,display_name,metadata_readable,indexed_device,install_path,title_version,group_id_hex\n",f);
    int32_t h=MCP_Open();
    if(h<0){*mcp_error=(int)h;fclose(f);return 0;}
    int32_t declared=MCP_TitleCount(h);
    if(declared<0){*mcp_error=(int)declared;MCP_Close(h);fclose(f);return 0;}
    uint32_t capacity=(uint32_t)declared+128u;
    if(capacity<256u)capacity=256u;
    if(capacity>MAX_TITLES){*mcp_error=-10001;MCP_Close(h);fclose(f);return 0;}
    MCPTitleListType *list=calloc(capacity,sizeof(*list));
    if(!list){*mcp_error=-10002;MCP_Close(h);fclose(f);return 0;}
    uint32_t count=0;
    int32_t rc=MCP_TitleList(h,&count,list,capacity*sizeof(*list));
    /* Close MCP before metadata I/O. The list is a standalone app snapshot,
       unaffected by the Menu-only v0.8.1 hook. */
    MCP_Close(h);
    if(rc!=0||count>capacity){*mcp_error=(rc!=0)?(int)rc:-10003;free(list);fclose(f);return 0;}
    for(uint32_t i=0;i<count;i++){
        const MCPTitleListType *p=&list[i];
        uint64_t tid=p->titleId;
        uint32_t app=(uint32_t)p->appType;
        char path[57],device[11],title[160],r[32];
        copied(path,sizeof(path),p->path,sizeof(p->path));
        copied(device,sizeof(device),p->indexedDevice,sizeof(p->indexedDevice));
        int meta=read_meta(path,title,sizeof(title),r,sizeof(r));
        const char *label=platform(tid,app);
        int confirmed=!strcmp(label,"WII")||!strcmp(label,"HOMEBREW");
        const char *region=(*r)?r:"UNKNOWN";
        const char *k=kind(tid,app);
        if(!strcmp(k,"GAME"))(*games)++;
        if(!strcmp(k,"HOMEBREW_SYNTHETIC"))(*synth)++;
        if(*title)(*named)++;
        if(strcmp(region,"UNKNOWN")!=0)(*regioned)++;
        fprintf(f,"%08lX%08lX,%08lX,",
                (unsigned long)(uint32_t)(tid>>32),
                (unsigned long)(uint32_t)tid,
                (unsigned long)app);
        csv_field(f,k);fputc(',',f);
        csv_field(f,label);fputc(',',f);
        csv_field(f,confirmed?"APP_TYPE":"UNKNOWN");fputc(',',f);
        csv_field(f,region);fputc(',',f);
        csv_field(f,(*r)?"META_XML":"UNKNOWN");fputc(',',f);
        csv_field(f,title);fprintf(f,",%d,",meta);
        csv_field(f,device);fputc(',',f);
        csv_field(f,path);
        fprintf(f,",%u,%08lX\n",(unsigned)p->titleVersion,(unsigned long)p->groupId);
        (*rows)++;
    }
    free(list);
    if(fclose(f)!=0){*mcp_error=-10004;return 0;}
    *mcp_error=0;
    return 1;
}
static uint32_t scan_homebrew_dir(FILE*f,const char*dir,int depth,uint32_t*errors){
    DIR*d=opendir(dir);if(!d){(*errors)++;return 0;}
    struct dirent*e;uint32_t found=0;
    while((e=readdir(d))!=NULL){
        if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
        char path[768];
        if(snprintf(path,sizeof(path),"%s/%s",dir,e->d_name)>=(int)sizeof(path))continue;
        struct stat st;
        if(stat(path,&st)!=0)continue;
        if(S_ISDIR(st.st_mode)&&depth>0){found+=scan_homebrew_dir(f,path,depth-1,errors);continue;}
        if(!S_ISREG(st.st_mode))continue;
        if(!ends_with_ci(e->d_name,".wuhb")&&!ends_with_ci(e->d_name,".rpx"))continue;
        csv_field(f,e->d_name);fputc(',',f);
        csv_field(f,path);fputc(',',f);
        csv_field(f,ends_with_ci(e->d_name,".wuhb")?"WUHB":"RPX");
        fputs(",UNKNOWN,NOT_MAPPED_TO_SYNTHETIC_ID\n",f);
        found++;
    }
    closedir(d);return found;
}
static int collect_homebrew(uint32_t *found,uint32_t *errors){
    *found=*errors=0;
    FILE*f=fopen(HB_OUT,"wb");if(!f)return 0;
    fputs("filename,sd_path,format,menu_visibility,synthetic_title_mapping\n",f);
    *found=scan_homebrew_dir(f,"sd:/wiiu/apps",2,errors);
    return fclose(f)==0;
}
/*
 * v1.0.1: start WITHOUT disk or MCP I/O. Hardware launch/exit first,
 * then opt-in with A to run inventory. This isolates launch freezes.
 */
int main(int argc,char**argv){
    (void)argc;(void)argv;
    WHBProcInit();
    WHBLogConsoleInit();
    VPADInit();
    WHBLogPrintf("WLP Inventory Scanner v1.0.1 - SAFE START");
    WHBLogPrintf("NO SCAN has run yet. No Menu/Barista changes.");
    WHBLogPrintf("B: exit now   A: scan once");
    WHBLogPrintf("Test B before testing A on a later run.");
    WHBLogConsoleDraw();
    int scanned=0;
    while(WHBProcIsRunning()){
        VPADStatus v={0};
        VPADReadError err=VPAD_READ_NO_SAMPLES;
        int n=VPADRead(VPAD_CHAN_0,&v,1,&err);
        uint32_t down=(n>0&&err==VPAD_READ_SUCCESS)?v.trigger:0;
        if(down & VPAD_BUTTON_B)break;
        if((down & VPAD_BUTTON_A) && !scanned){
            scanned=1;
            WHBLogPrintf("A received. Mounting SD...");
            WHBLogConsoleDraw();
            if(!WHBMountSdCard()){
                WHBLogPrintf("FAILED: SD mount. B to exit.");
            } else {
                WHBLogPrintf("SD mounted. Starting MCP inventory...");
                WHBLogConsoleDraw();
                mkdir("sd:/wiiu",0777);
                mkdir("sd:/wiiu/library_profiles",0777);
                mkdir(OUT_DIR,0777);
                uint32_t rows=0,games=0,synth=0,named=0,regioned=0,hb=0,errors=0;
                int mcp_error=0;
                int ok1=collect_titles(&rows,&games,&synth,&named,&regioned,&mcp_error);
                WHBLogPrintf("MCP done: status %d, records %lu",mcp_error,(unsigned long)rows);
                WHBLogConsoleDraw();
                int ok2=collect_homebrew(&hb,&errors);
                FILE *f=fopen(SUMMARY_OUT,"wb");
                if(f){
                    fprintf(f,"version=1.0.1-safe-start\nmcp_ok=%d\nmcp_error=%d\nmcp_records=%lu\nbase_games=%lu\nsynthetic_homebrew=%lu\nmetadata_names=%lu\nmetadata_regions=%lu\nsd_homebrew_apps=%lu\nhomebrew_scan_errors=%lu\nhomebrew_csv_ok=%d\nmenu_filter_modified=NO\nbarista_modified=NO\n",
                            ok1,mcp_error,(unsigned long)rows,(unsigned long)games,
                            (unsigned long)synth,(unsigned long)named,(unsigned long)regioned,
                            (unsigned long)hb,(unsigned long)errors,ok2);
                    fclose(f);
                }
                WHBUnmountSdCard();
                WHBLogPrintf("Homebrew: %lu  output: sd:/wiiu/library_profiles/v10/",
                             (unsigned long)hb);
            }
            WHBLogPrintf("B: exit.");
            WHBLogConsoleDraw();
        }
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(70));
    }
    VPADShutdown();
    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}
