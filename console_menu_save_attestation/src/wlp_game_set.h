#ifndef WLP_GAME_SET_H
#define WLP_GAME_SET_H
/* Portable canonical SHA-256 of installed game Title IDs.
 * Raw MCP call order, updates, DLC and synthetic homebrew MUST NOT affect it.
 * Caller builds the game-only array. The array is sorted IN PLACE.
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wlp_sha256.h"
static inline int wlp_id_cmp(const void *a,const void *b){
 uint64_t x=*(const uint64_t*)a,y=*(const uint64_t*)b;
 return (x>y)-(x<y);
}
static inline int wlp_game_set_hash(uint64_t *ids,size_t n,uint8_t digest[32]){
 if(!digest||(!ids&&n)||n>4096u)return 0;
 qsort(ids,n,sizeof(uint64_t),wlp_id_cmp);
 for(size_t i=1;i<n;i++)if(ids[i]<=ids[i-1])return 0;
 wlp_sha256_ctx ctx;wlp_sha256_init(&ctx);
 for(size_t i=0;i<n;i++){
  uint8_t be[8];uint64_t x=ids[i];
  for(int j=7;j>=0;j--){be[j]=(uint8_t)(x&255u);x>>=8;}
  wlp_sha256_update(&ctx,be,8);
 }
 wlp_sha256_final(&ctx,digest);
 return 1;
}
static inline void wlp_hash_hex(const uint8_t digest[32],char hex[65]){
 static const char dig[]="0123456789ABCDEF";
 for(int i=0;i<32;i++){hex[2*i]=dig[digest[i]>>4];hex[2*i+1]=dig[digest[i]&15];}
 hex[64]=0;
}
#endif
