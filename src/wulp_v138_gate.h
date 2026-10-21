#pragma once
// WULP138 portable cryptographic profile-pair gate. No console paths or owner data.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
static inline uint32_t w138_rotr(uint32_t x,unsigned n){return (x>>n)|(x<<(32-n));}
static inline void w138_sha(const uint8_t *src,size_t n,uint8_t out[32]){
 static const uint32_t K[64]={
 0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
 0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
 0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
 0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
 0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
 0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
 0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
 0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
 uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
 size_t blocks=(n/64)+1+((n%64)>=56?1:0);
 for(size_t b=0;b<blocks;b++){
  uint8_t m[64]={};
  for(size_t i=0;i<64;i++){
   size_t pos=b*64+i;
   if(pos<n)m[i]=src[pos]; else if(pos==n)m[i]=0x80;
   if(b==blocks-1&&i>=56){uint64_t bits=(uint64_t)n*8u;m[i]=(uint8_t)(bits>>((63-i)*8));}
  }
  uint32_t w[64];
  for(unsigned i=0;i<16;i++)w[i]=((uint32_t)m[4*i]<<24)|((uint32_t)m[4*i+1]<<16)|((uint32_t)m[4*i+2]<<8)|m[4*i+3];
  for(unsigned i=16;i<64;i++){uint32_t a=w[i-15],c=w[i-2];
   w[i]=(w138_rotr(c,17)^w138_rotr(c,19)^(c>>10))+w[i-7]+(w138_rotr(a,7)^w138_rotr(a,18)^(a>>3))+w[i-16];
  }
  uint32_t a=h[0],bb=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
  for(unsigned i=0;i<64;i++){
   uint32_t t1=z+(w138_rotr(e,6)^w138_rotr(e,11)^w138_rotr(e,25))+((e&f)^(~e&g))+K[i]+w[i];
   uint32_t t2=(w138_rotr(a,2)^w138_rotr(a,13)^w138_rotr(a,22))+((a&bb)^(a&c)^(bb&c));
   z=g;g=f;f=e;e=d+t1;d=c;c=bb;bb=a;a=t1+t2;
  }
  h[0]+=a;h[1]+=bb;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;
 }
 for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++)out[i*4+j]=(uint8_t)(h[i]>>(24-j*8));
}
static inline uint64_t w138_u64(const uint8_t *p){uint64_t n=0;for(unsigned j=0;j<8;j++)n=(n<<8)|p[j];return n;}
static inline uint32_t w138_u32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static inline uint32_t w138_crc(const uint8_t *p,size_t n){uint32_t c=~0u;for(size_t i=0;i<n;i++){c^=p[i];for(int j=0;j<8;j++)c=(c>>1)^(0xedb88320u&(0u-(c&1u)));}return ~c;}
static inline void w138_hex(const uint8_t h[32],char out[65]){static const char x[]="0123456789abcdef";for(int i=0;i<32;i++){out[2*i]=x[h[i]>>4];out[2*i+1]=x[h[i]&15];}out[64]=0;}
#define W138_BARISTA 130116u
#define W138_WLPA 2320u
// 0 = reject/fallback, 1 = explicit legacy, 2 = verified extended.
static inline int w138_gate(const char *marker,size_t mn,const uint8_t *baseline,size_t bn,
 const uint8_t *candidate,size_t cn,const uint8_t *wlpa,size_t wn,uint64_t ids[284]){
 static const char legacy[]="WULP137:1\nprofile=legacy\n";
 if(marker&&mn==sizeof(legacy)-1u&&!memcmp(marker,legacy,mn))return 1;
 if(!marker||mn>220||!baseline||!candidate||!wlpa||!ids||bn!=W138_BARISTA||cn!=W138_BARISTA||wn!=W138_WLPA)return 0;
 uint8_t bh[32],ah[32];char bs[65],as[65],wanted[220];
 w138_sha(candidate,cn,bh);w138_sha(wlpa,wn,ah);w138_hex(bh,bs);w138_hex(ah,as);
 int len=snprintf(wanted,sizeof(wanted),"WULP137:1\nprofile=extended\nbarista_sha256=%s\nwlpa_sha256=%s\n",bs,as);
 if(len<0||(size_t)len>=sizeof(wanted)||mn!=(size_t)len||memcmp(marker,wanted,mn))return 0;
 if(memcmp(baseline,candidate,5040u)||memcmp(baseline+9600u,candidate+9600u,W138_BARISTA-9600u))return 0;
 for(unsigned i=0;i<600;i++)if(memcmp(baseline+i*16u+8u,candidate+i*16u+8u,8u))return 0;
 for(unsigned i=315;i<592;i++){
  uint64_t id=w138_u64(candidate+i*16u);if((id>>32)!=0x00050000u)return 0;
  for(unsigned j=315;j<i;j++)if(id==w138_u64(candidate+j*16u))return 0;
 }
 for(unsigned i=592;i<600;i++)if(w138_u64(candidate+i*16u))return 0;
 if(memcmp(wlpa,"WLPA",4)||wlpa[4]||wlpa[5]!=1||wlpa[6]||wlpa[7]||
    w138_u32(wlpa+8)!=284||memcmp(wlpa+12,bh,32)||
    w138_crc(wlpa,wn-4)!=w138_u32(wlpa+wn-4))return 0;
 uint64_t last=0;unsigned games=0,hb=0;
 for(unsigned i=0;i<284;i++){
  uint64_t id=w138_u64(wlpa+44u+i*8u);if(i&&id<=last)return 0;last=id;
  bool found=false;
  if((id>>32)==0x00050000u){for(unsigned j=315;j<592;j++)if(id==w138_u64(candidate+j*16u)){found=true;break;}if(found)games++;}
  else if((id>>32)==0x0005000fu){for(unsigned j=8;j<15;j++)if(id==w138_u64(candidate+j*16u)){found=true;break;}if(found)hb++;}
  if(!found)return 0;
  ids[i]=id;
 }
 return (games==277&&hb==7)?2:0;
}
