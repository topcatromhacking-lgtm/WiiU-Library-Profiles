#ifndef WLP_V133_TRACE_CAPTURE_H
#define WLP_V133_TRACE_CAPTURE_H
/*
 * Portable, allocation-free reference trace capture for instrumented MENU FS
 * operations. NO FILE I/O. NO SAVE MODIFICATION. Not a complete WUPS plugin.
 * A future integration must call these functions ONLY from verified sources:
 * registration from Homebrew-on-Menu, open/read/close from ContentRedirection
 * Module's true FSWrapper after successful physical open/read.
 * The log is untrusted without corresponding reviewed on-device instrumentation.
 */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define WLP_TRACE_CAPACITY 65536u

typedef struct {
    char data[WLP_TRACE_CAPACITY];
    size_t used;
    uint32_t next_seq;
    char boot[65];
    int overflow;
} WlpTrace;

/* SHA-256 implemented in C11 to avoid hard-coded devkitPPC crypto dependencies. */
typedef struct { uint32_t h[8]; uint8_t block[64]; uint64_t bits; size_t fill; } WlpSha;
static uint32_t wlp_rotr(uint32_t a,unsigned n){return (a>>n)|(a<<(32-n));}
static const uint32_t wlp_k[64]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
static void wlp_sha_init(WlpSha*s){
const uint32_t h[]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
memcpy(s->h,h,sizeof h);s->bits=0;s->fill=0;
}
static void wlp_sha_transform(WlpSha*s,const uint8_t*b){
uint32_t w[64];
for(int i=0;i<16;i++)w[i]=((uint32_t)b[i*4]<<24)|((uint32_t)b[i*4+1]<<16)|((uint32_t)b[i*4+2]<<8)|b[i*4+3];
for(int i=16;i<64;i++){
 uint32_t t=w[i-15],u=w[i-2];
 uint32_t a=wlp_rotr(t,7)^wlp_rotr(t,18)^(t>>3);
 uint32_t c=wlp_rotr(u,17)^wlp_rotr(u,19)^(u>>10);
 w[i]=w[i-16]+a+w[i-7]+c;
}
uint32_t a=s->h[0],b0=s->h[1],c=s->h[2],d=s->h[3],e=s->h[4],f=s->h[5],g=s->h[6],h=s->h[7];
for(int i=0;i<64;i++){
 uint32_t S1=wlp_rotr(e,6)^wlp_rotr(e,11)^wlp_rotr(e,25);
 uint32_t ch=(e&f)^(~e&g);
 uint32_t t1=h+S1+ch+wlp_k[i]+w[i];
 uint32_t S0=wlp_rotr(a,2)^wlp_rotr(a,13)^wlp_rotr(a,22);
 uint32_t maj=(a&b0)^(a&c)^(b0&c);
 uint32_t t2=S0+maj;
 h=g;g=f;f=e;e=d+t1;d=c;c=b0;b0=a;a=t1+t2;
}
s->h[0]+=a;s->h[1]+=b0;s->h[2]+=c;s->h[3]+=d;s->h[4]+=e;s->h[5]+=f;s->h[6]+=g;s->h[7]+=h;
}
static void wlp_sha_update(WlpSha*s,const void*data,size_t n){
const uint8_t *p=(const uint8_t*)data;
for(size_t i=0;i<n;i++){
 s->block[s->fill++]=p[i];
 if(s->fill==64){wlp_sha_transform(s,s->block);s->fill=0;}
}
s->bits+=(uint64_t)n*8;
}
static void wlp_sha_final(WlpSha*s,uint8_t*out){
uint64_t bits=s->bits;
s->block[s->fill++]=0x80;
if(s->fill>56){while(s->fill<64)s->block[s->fill++]=0;wlp_sha_transform(s,s->block);s->fill=0;}
while(s->fill<56)s->block[s->fill++]=0;
for(int i=7;i>=0;i--)s->block[s->fill++]=(uint8_t)(bits>>(i*8));
wlp_sha_transform(s,s->block);
for(int i=0;i<8;i++)for(int j=0;j<4;j++)out[i*4+j]=(uint8_t)(s->h[i]>>(24-8*j));
}
static void wlp_sha256_hex(const void*p,size_t n,char hex[65]){
static const char h[]="0123456789abcdef";WlpSha s;uint8_t sum[32];wlp_sha_init(&s);wlp_sha_update(&s,p,n);wlp_sha_final(&s,sum);
for(int i=0;i<32;i++){hex[i*2]=h[sum[i]>>4];hex[i*2+1]=h[sum[i]&15];}hex[64]=0;
}

static int wlp_trace_init(WlpTrace*t,const char*boot){
if(!t||!boot)return 0;
size_t n=strlen(boot);if(n<8||n>64)return 0;
for(size_t i=0;i<n;i++)if(!((boot[i]>='0'&&boot[i]<='9')||(boot[i]>='A'&&boot[i]<='Z')||(boot[i]>='a'&&boot[i]<='z')||boot[i]=='-'))return 0;
memset(t,0,sizeof *t);memcpy(t->boot,boot,n);return 1;
}
static int wlp_emit(WlpTrace*t,const char*event,const char*fields){
if(!t||t->overflow)return 0;
size_t remain=WLP_TRACE_CAPACITY-t->used;
int n=snprintf(t->data+t->used,remain,"{\"seq\":%u,\"boot\":\"%s\",\"event\":\"%s\"%s}\n",t->next_seq,t->boot,event,fields);
if(n<0||(size_t)n>=remain){t->overflow=1;return 0;}
t->used+=(size_t)n;t->next_seq++;return 1;
}
static int wlp_trace_init_save(WlpTrace*t,int is_menu,int sd_mounted,int visible,const char account[9]){
if(!account||strlen(account)!=8)return 0;
for(int i=0;i<8;i++)if(!((account[i]>='0'&&account[i]<='9')||(account[i]>='A'&&account[i]<='F')))return 0;
char fields[220];snprintf(fields,sizeof fields,",\"menu_title\":%s,\"sd_mounted\":%s,\"homebrew_visible\":%s,\"account\":\"%s\"",is_menu?"true":"false",sd_mounted?"true":"false",visible?"true":"false",account);
return wlp_emit(t,"SAVE_INIT",fields);
}
static int wlp_trace_register(WlpTrace*t,int success,uint32_t layer_handle){
char fields[160];snprintf(fields,sizeof fields,",\"layer\":\"homp_save_redirection\",\"result\":\"%s\",\"handle\":%u",success?"SUCCESS":"FAILED",layer_handle);
return wlp_emit(t,"LAYER_REGISTER",fields);
}
static int wlp_trace_open(WlpTrace*t,uint32_t layer_handle,uint32_t file_handle,int did_open_redirected_barista){
char fields[260];snprintf(fields,sizeof fields,",\"layer\":\"homp_save_redirection\",\"handle\":%u,\"file\":\"BaristaAccountSaveFile.dat\",\"physical_source\":\"%s\",\"file_handle\":%u,\"result\":\"%s\"",layer_handle,did_open_redirected_barista?"SD_REDIRECT_ACCOUNT_SAVE":"UNVERIFIED",file_handle,did_open_redirected_barista?"SUCCESS":"FAILED");
return wlp_emit(t,"MENU_OPEN",fields);
}
static int wlp_trace_read(WlpTrace*t,uint32_t file_handle,size_t offset,const void*buffer,size_t actual_bytes){
if(!buffer||!actual_bytes||offset>UINT32_MAX||actual_bytes>UINT32_MAX)return 0;
char digest[65],fields[240];
wlp_sha256_hex(buffer,actual_bytes,digest);
snprintf(fields,sizeof fields,",\"file_handle\":%u,\"offset\":%u,\"length\":%u,\"sha256\":\"%s\",\"result\":\"SUCCESS\"",file_handle,(unsigned)offset,(unsigned)actual_bytes,digest);
return wlp_emit(t,"MENU_READ",fields);
}
static int wlp_trace_close(WlpTrace*t,uint32_t file_handle,int success){
char fields[120];snprintf(fields,sizeof fields,",\"file_handle\":%u,\"result\":\"%s\"",file_handle,success?"SUCCESS":"FAILED");return wlp_emit(t,"MENU_CLOSE",fields);
}
static inline int wlp_trace_remove(WlpTrace*t,uint32_t layer_handle){
char fields[64];snprintf(fields,sizeof fields,",\"handle\":%u",layer_handle);return wlp_emit(t,"LAYER_REMOVE",fields);
}
static inline int wlp_trace_active_snapshot(WlpTrace*t,uint32_t layer_handle,int still_active){
char fields[96];snprintf(fields,sizeof fields,",\"handle\":%u,\"still_active\":%s",layer_handle,still_active?"true":"false");
return wlp_emit(t,"ACTIVE_SNAPSHOT",fields);
}
static int wlp_trace_end(WlpTrace*t){return wlp_emit(t,"SESSION_END","");}
#endif
