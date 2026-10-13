/*
 * Wii U Library Profiles v1.0.2: one-button-at-a-time MCP diagnosis.
 *
 * This is a standalone WUHB, NOT a WUPS Menu filter.
 * It never writes Barista or title installation data.
 * A mounts SD and opens a diagnostic CSV, X opens MCP, Y counts titles,
 * PLUS enumerates IDs and saves RAW CSV WITHOUT ANY per-title metadata reads.
 * B exits. Each potentially blocking service call is bracketed in an
 * append-and-close journal: the last BEGIN identifies the exact stall.
 */
#include <coreinit/mcp.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <vpad/input.h>
#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <whb/sdcard.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define OUTDIR "sd:/wiiu/library_profiles/v102"
#define JOURNAL OUTDIR "/v102-stages.csv"
#define TITLES OUTDIR "/v102-raw-titles.csv"
#define MAX_TITLES 8192u

static int sd_ok=0,handle=-1,have_count=0,have_titles=0;
static uint32_t declared=0;
static void stage_log(const char *stage,const char *phase,long detail) {
    if(!sd_ok)return;
    FILE *f=fopen(JOURNAL,"ab");
    if(!f)return;
    fprintf(f,"%s,%s,%ld\n",stage,phase,detail);
    fflush(f);
    fclose(f);
}
static int init_sd(){
    if(sd_ok)return 1;
    if(!WHBMountSdCard())return 0;
    sd_ok=1;
    mkdir("sd:/wiiu",0777);
    mkdir("sd:/wiiu/library_profiles",0777);
    mkdir(OUTDIR,0777);
    FILE *f=fopen(JOURNAL,"wb");
    if(f){fputs("stage,phase,detail\n",f);fflush(f);fclose(f);}
    stage_log("SD_MOUNT","OK",0);
    return 1;
}
static void begin_mcp(){
    if(!sd_ok){WHBLogPrintf("First press A to mount SD");return;}
    if(handle>=0){WHBLogPrintf("MCP already open");return;}
    WHBLogPrintf("MCP_Open: BEGIN");
    WHBLogConsoleDraw();
    stage_log("MCP_OPEN","BEGIN",0);
    int h=MCP_Open();
    stage_log("MCP_OPEN","RETURN",(long)h);
    if(h<0){WHBLogPrintf("MCP_Open failed: %d",h);return;}
    handle=h;
    WHBLogPrintf("MCP_Open: OK (%d)",h);
    WHBLogPrintf("Next: press Y to count titles");
}
static void count_mcp(){
    if(handle<0){WHBLogPrintf("First press X to open MCP");return;}
    if(have_count){WHBLogPrintf("MCP_TitleCount already completed: %lu",(unsigned long)declared);return;}
    WHBLogPrintf("MCP_TitleCount: BEGIN");
    WHBLogConsoleDraw();
    stage_log("MCP_TITLECOUNT","BEGIN",0);
    int32_t n=MCP_TitleCount(handle);
    stage_log("MCP_TITLECOUNT","RETURN",(long)n);
    if(n<0){WHBLogPrintf("MCP_TitleCount error: %ld",(long)n);return;}
    if(n>=(int32_t)(MAX_TITLES-128)){
        WHBLogPrintf("Unusually high count: %ld, aborting",(long)n);
        return;
    }
    declared=(uint32_t)n;
    have_count=1;
    WHBLogPrintf("MCP_TitleCount: %lu",(unsigned long)declared);
    WHBLogPrintf("Next: press PLUS for raw titles (no metadata)");
}
static void list_mcp(){
    if(!have_count || handle<0){
        WHBLogPrintf("First press X then Y");return;
    }
    if(have_titles){WHBLogPrintf("Raw title export already completed");return;}
    uint32_t capacity=declared+128u;
    if(capacity<256u)capacity=256u;
    if(capacity>MAX_TITLES){WHBLogPrintf("Capacity guard failed");return;}
    MCPTitleListType *list=calloc((size_t)capacity,sizeof(*list));
    if(!list){WHBLogPrintf("No memory for %lu records",(unsigned long)capacity);return;}
    uint32_t n=0;
    WHBLogPrintf("MCP_TitleList: BEGIN, capacity %lu",(unsigned long)capacity);
    WHBLogConsoleDraw();
    stage_log("MCP_TITLELIST","BEGIN",(long)capacity);
    int32_t rc=MCP_TitleList(handle,&n,list,(uint32_t)((size_t)capacity*sizeof(*list)));
    stage_log("MCP_TITLELIST","RETURN",(long)rc);
    if(rc!=0||n>capacity){
        WHBLogPrintf("MCP_TitleList error %ld / returned %lu",(long)rc,(unsigned long)n);
        free(list);return;
    }
    stage_log("CSV_EXPORT","BEGIN",(long)n);
    FILE *f=fopen(TITLES,"wb");
    if(!f){
        stage_log("CSV_EXPORT","OPEN_FAILED",-1);
        WHBLogPrintf("Failed to open CSV output");
        free(list);return;
    }
    fputs("title_id,app_type_hex,title_version,indexed_device,group_id_hex\n",f);
    for(uint32_t i=0;i<n;i++){
        uint64_t tid=list[i].titleId;
        char dev[11]={0};
        memcpy(dev,list[i].indexedDevice,10);
        for(int j=0;j<10;j++)
            if(dev[j]==','||dev[j]=='"'||dev[j]=='\r'||dev[j]=='\n')dev[j]='_';
        fprintf(f,"%08lX%08lX,%08lX,%u,\"%s\",%08lX\n",
                (unsigned long)(uint32_t)(tid>>32),
                (unsigned long)(uint32_t)tid,
                (unsigned long)(uint32_t)list[i].appType,
                (unsigned)list[i].titleVersion,
                dev,(unsigned long)list[i].groupId);
    }
    int err=ferror(f);
    int close_err=fclose(f);
    stage_log("CSV_EXPORT",(err||close_err)?"WRITE_FAILED":"OK",(long)n);
    free(list);
    if(err||close_err){WHBLogPrintf("CSV write failed");return;}
    have_titles=1;
    WHBLogPrintf("Export complete: %lu raw Title IDs",(unsigned long)n);
    WHBLogPrintf("No meta.xml read. Press B to exit.");
}
int main(int argc,char**argv){
    (void)argc;(void)argv;
    WHBProcInit();
    WHBLogConsoleInit();
    VPADInit();
    WHBLogPrintf("WLP Scanner v1.0.2 - STEP DIAGNOSTIC");
    WHBLogPrintf("No automatic MCP or storage access on launch.");
    WHBLogPrintf("A: mount SD");
    WHBLogPrintf("X: MCP_Open (after A)");
    WHBLogPrintf("Y: MCP_TitleCount (after X)");
    WHBLogPrintf("PLUS: MCP_TitleList raw export (after Y)");
    WHBLogPrintf("B: exit at any point");
    WHBLogPrintf("Perform ONE step at a time; STOP if frozen.");
    WHBLogConsoleDraw();
    while(WHBProcIsRunning()){
        VPADStatus v={0};
        VPADReadError err=VPAD_READ_NO_SAMPLES;
        int n=VPADRead(VPAD_CHAN_0,&v,1,&err);
        uint32_t trigger=(n>0&&err==VPAD_READ_SUCCESS)?v.trigger:0u;
        if(trigger&VPAD_BUTTON_B)break;
        if(trigger&VPAD_BUTTON_A){
            WHBLogPrintf("SD mount: BEGIN");
            WHBLogConsoleDraw();
            WHBLogPrintf(init_sd()?"SD mount: OK":"SD mount: FAILED");
        }else if(trigger&VPAD_BUTTON_X){
            begin_mcp();
        }else if(trigger&VPAD_BUTTON_Y){
            count_mcp();
        }else if(trigger&VPAD_BUTTON_PLUS){
            list_mcp();
        }
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(70));
    }
    if(handle>=0){
        stage_log("MCP_CLOSE","BEGIN",0);
        int32_t rc=MCP_Close(handle);
        stage_log("MCP_CLOSE","RETURN",(long)rc);
    }
    stage_log("EXIT","OK",0);
    if(sd_ok)WHBUnmountSdCard();
    VPADShutdown();
    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}
