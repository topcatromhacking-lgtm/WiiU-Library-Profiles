#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/wlp_barista_probe_core.h"
static void be32(uint8_t*p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
static void be64(uint8_t*p,uint64_t v){be32(p,(uint32_t)(v>>32));be32(p+4,(uint32_t)v);}
int main(void){
 uint8_t *p=(uint8_t*)calloc(1,WLP_BARISTA_BYTES);assert(p);
 for(int i=0;i<600;i++)be32(p+i*16+8,i<300?1:2);
 for(int i=0;i<8;i++)be64(p+i*16,UINT64_C(0x0005001010040000)+i);
 for(int i=0;i<7;i++)be64(p+(i+8)*16,UINT64_C(0x0005000f01000001)+i);
 for(int i=0;i<285;i++)be64(p+(i+315)*16,UINT64_C(0x0005000010000001)+i);
 for(int f=0;f<60;f++)for(int i=0;i<120;i++)be32(p+WLP_BARISTA_FOLDER_OFFSET+f*WLP_BARISTA_FOLDER_STRIDE+i*16+8,i<60?1:2);
 p[9607]=0x30;wlp_barista_inspection info={0};
 assert(wlp_barista_inspect(p,WLP_BARISTA_BYTES,&info));
 assert(info.fixed_icons==15 && info.usb_games==285 && info.folders_empty==60);
 assert(!wlp_barista_inspect(p,WLP_BARISTA_BYTES-1,&info));
 p[15*16]=1;assert(!wlp_barista_inspect(p,WLP_BARISTA_BYTES,&info));p[15*16]=0;
 p[315*16+15]=2;assert(!wlp_barista_inspect(p,WLP_BARISTA_BYTES,&info));p[315*16+15]=0;
 p[WLP_BARISTA_FOLDER_OFFSET]=1;assert(!wlp_barista_inspect(p,WLP_BARISTA_BYTES,&info));p[WLP_BARISTA_FOLDER_OFFSET]=0;
 p[WLP_BARISTA_FOLDER_OFFSET+1920]=1;assert(!wlp_barista_inspect(p,WLP_BARISTA_BYTES,&info));p[WLP_BARISTA_FOLDER_OFFSET+1920]=0;
 be64(p+316*16,wlp_barista_be64(p+315*16));assert(!wlp_barista_inspect(p,WLP_BARISTA_BYTES,&info));
 free(p);puts("PASS: Barista 300-bank, 60-folder, fixed first page and fail-closed C11 parser");return 0;
}
