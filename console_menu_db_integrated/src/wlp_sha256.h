#ifndef WLP_SHA256_H
#define WLP_SHA256_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef struct { uint32_t state[8]; uint64_t total; unsigned used; uint8_t block[64]; } wlp_sha256_ctx;
static inline uint32_t wlp_ror(uint32_t x,int n){return (x>>n)|(x<<(32-n));}
static inline uint32_t wlp_load32(const uint8_t*p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static inline void wlp_store32(uint8_t*p,uint32_t x){p[0]=(uint8_t)(x>>24);p[1]=(uint8_t)(x>>16);p[2]=(uint8_t)(x>>8);p[3]=(uint8_t)x;}
static const uint32_t wlp_sha_k[64]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
static inline void wlp_sha256_transform(wlp_sha256_ctx*c,const uint8_t *p){
 uint32_t w[64];for(int i=0;i<16;i++)w[i]=wlp_load32(p+4*i);
 for(int i=16;i<64;i++){
  uint32_t x=w[i-15],y=w[i-2];
  w[i]=(wlp_ror(y,17)^wlp_ror(y,19)^(y>>10))+w[i-7]+
       (wlp_ror(x,7)^wlp_ror(x,18)^(x>>3))+w[i-16];
 }
 uint32_t a=c->state[0],b=c->state[1],cc=c->state[2],d=c->state[3];
 uint32_t e=c->state[4],f=c->state[5],g=c->state[6],h=c->state[7];
 for(int i=0;i<64;i++){
   uint32_t t1=h+(wlp_ror(e,6)^wlp_ror(e,11)^wlp_ror(e,25))+((e&f)^(~e&g))+wlp_sha_k[i]+w[i];
   uint32_t t2=(wlp_ror(a,2)^wlp_ror(a,13)^wlp_ror(a,22))+((a&b)^(a&cc)^(b&cc));
   h=g;g=f;f=e;e=d+t1;d=cc;cc=b;b=a;a=t1+t2;
 }
 c->state[0]+=a;c->state[1]+=b;c->state[2]+=cc;c->state[3]+=d;
 c->state[4]+=e;c->state[5]+=f;c->state[6]+=g;c->state[7]+=h;
}
static inline void wlp_sha256_init(wlp_sha256_ctx*c){
 static const uint32_t init[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
 memset(c,0,sizeof(*c));memcpy(c->state,init,sizeof(init));
}
static inline void wlp_sha256_update(wlp_sha256_ctx*c,const uint8_t*p,size_t n){
 c->total+=n;
 while(n){size_t want=64-c->used;if(want>n)want=n;memcpy(c->block+c->used,p,want);
  c->used+=(unsigned)want;p+=want;n-=want;
  if(c->used==64){wlp_sha256_transform(c,c->block);c->used=0;}
 }
}
static inline void wlp_sha256_final(wlp_sha256_ctx*c,uint8_t out[32]){
 uint64_t bitlen=c->total*8;
 uint8_t pad[72]={0x80};
 size_t padlen=(c->used<56)?(56-c->used):(120-c->used);
 wlp_sha256_update(c,pad,padlen);
 uint8_t len[8];for(int i=0;i<8;i++)len[i]=(uint8_t)(bitlen>>(56-i*8));
 wlp_sha256_update(c,len,8);
 for(int i=0;i<8;i++)wlp_store32(out+i*4,c->state[i]);
}
#endif
