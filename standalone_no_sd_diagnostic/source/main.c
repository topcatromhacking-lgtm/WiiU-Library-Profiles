/*
 * Wii U Library Profiles v1.0.3: SDUSB isolation test.
 * Absolutely no SD/filesystem mount calls or disk reads/writes.
 * Only explicitly gated MCP inventory calls. All output on screen.
 */
#include <coreinit/mcp.h>
#include <coreinit/thread.h>
#include <coreinit/time.h>
#include <vpad/input.h>
#include <whb/proc.h>
#include <whb/log.h>
#include <whb/log_console.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TITLES 8192u
static int mcp=-1,has_count=0,listed=0;
static uint32_t count=0;

static void open_mcp() {
    if(mcp>=0){WHBLogPrintf("MCP already open.");return;}
    WHBLogPrintf("MCP_Open: BEGIN (no SD operations)");
    WHBLogConsoleDraw();
    int32_t h=MCP_Open();
    WHBLogPrintf("MCP_Open RETURN: %ld",(long)h);
    if(h>=0)mcp=h;
    WHBLogConsoleDraw();
}
static void count_titles(){
    if(mcp<0){WHBLogPrintf("Press A (open MCP) first.");return;}
    if(has_count){WHBLogPrintf("Cached title count: %lu",(unsigned long)count);return;}
    WHBLogPrintf("MCP_TitleCount: BEGIN");
    WHBLogConsoleDraw();
    int32_t n=MCP_TitleCount(mcp);
    WHBLogPrintf("MCP_TitleCount RETURN: %ld",(long)n);
    if(n>=0 && (uint32_t)n<MAX_TITLES-128u){
        count=(uint32_t)n;
        has_count=1;
        WHBLogPrintf("Count OK. Y is optional next step.");
    }else{WHBLogPrintf("Count failed or exceeds safety bound.");}
    WHBLogConsoleDraw();
}
static void list_titles(){
    if(!has_count||mcp<0){WHBLogPrintf("Press A, then X before Y.");return;}
    if(listed){WHBLogPrintf("Enumeration already completed.");return;}
    uint32_t capacity=count+128u;
    if(capacity<256u)capacity=256u;
    if(capacity>MAX_TITLES){WHBLogPrintf("Capacity guard failed.");return;}
    MCPTitleListType *buf=calloc((size_t)capacity,sizeof(*buf));
    if(!buf){WHBLogPrintf("Allocation failed.");return;}
    uint32_t n=0;
    WHBLogPrintf("MCP_TitleList: BEGIN");
    WHBLogConsoleDraw();
    int32_t rc=MCP_TitleList(mcp,&n,buf,(uint32_t)((size_t)capacity*sizeof(*buf)));
    WHBLogPrintf("MCP_TitleList RETURN: rc=%ld n=%lu",
                 (long)rc,(unsigned long)n);
    if(rc==0&&n<=capacity){
        uint32_t games=0,homebrew=0,others=0,wii=0;
        for(uint32_t i=0;i<n;i++){
            uint32_t hi=(uint32_t)(buf[i].titleId>>32);
            uint32_t app=(uint32_t)buf[i].appType;
            if(hi==0x0005000fu)homebrew++;
            else if(hi==0x00050000u && (app==0x80000000u||app==0x8000002eu)){
                games++;
                if(app==0x8000002eu)wii++;
            }else others++;
        }
        WHBLogPrintf("Titles=%lu  Games=%lu  Wii-type=%lu",
                     (unsigned long)n,(unsigned long)games,(unsigned long)wii);
        WHBLogPrintf("Synthetic HB=%lu Other=%lu",
                     (unsigned long)homebrew,(unsigned long)others);
        WHBLogPrintf("No SD read/write or Barista changes.");
        listed=1;
    }else{WHBLogPrintf("Enumeration failed. No files modified.");}
    free(buf);
    WHBLogConsoleDraw();
}
int main(int argc,char**argv){
    (void)argc;(void)argv;
    WHBProcInit();
    WHBLogConsoleInit();
    VPADInit();
    WHBLogPrintf("WLP v1.0.3 SDUSB-safe diagnostic");
    WHBLogPrintf("NO SD MOUNT, NO DISK I/O.");
    WHBLogPrintf("First launch: B to exit (safety check).");
    WHBLogPrintf("Then: A open MCP, X count, Y enumerate.");
    WHBLogPrintf("Each MCP step is MANUAL; stop if frozen.");
    WHBLogPrintf("B: exit (unless a service call blocks).");
    WHBLogConsoleDraw();
    while(WHBProcIsRunning()){
        VPADStatus v={0};
        VPADReadError err=VPAD_READ_NO_SAMPLES;
        int got=VPADRead(VPAD_CHAN_0,&v,1,&err);
        uint32_t down=(got>0&&err==VPAD_READ_SUCCESS)?v.trigger:0;
        if(down&VPAD_BUTTON_B)break;
        if(down&VPAD_BUTTON_A)open_mcp();
        else if(down&VPAD_BUTTON_X)count_titles();
        else if(down&VPAD_BUTTON_Y)list_titles();
        WHBLogConsoleDraw();
        OSSleepTicks(OSMillisecondsToTicks(70));
    }
    if(mcp>=0){MCP_Close(mcp);}
    VPADShutdown();
    WHBLogConsoleFree();
    WHBProcShutdown();
    return 0;
}
