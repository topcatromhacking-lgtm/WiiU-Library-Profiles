#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "../console_menu_db_integrated/src/wlpdb_core.h"

static void put16(uint8_t*p,uint16_t x){p[0]=(uint8_t)(x>>8);p[1]=(uint8_t)x;}
static void put32(uint8_t*p,uint32_t x){p[0]=(uint8_t)(x>>24);p[1]=(uint8_t)(x>>16);p[2]=(uint8_t)(x>>8);p[3]=(uint8_t)x;}
static void put64(uint8_t*p,uint64_t x){put32(p,(uint32_t)(x>>32));put32(p+4,(uint32_t)x);}
int main(void){
 uint8_t db[20+2*32+15]={0};
 memcpy(db,"WLPD",4);put16(db+4,1);put16(db+6,32);
 put32(db+8,2);put32(db+12,15);
 uint8_t *a=db+20,*b=a+32,*str=db+84;
 put64(a,UINT64_C(0x0005000010000001));a[8]=1;a[9]=2;a[10]=1;
 put32(a+12,0);put16(a+16,6);put32(a+20,6);put16(a+24,2);
 put64(b,UINT64_C(0x0005000010000002));b[8]=9;b[9]=5;b[10]=0;
 put32(b+12,8);put16(b+16,5);put32(b+20,13);put16(b+24,2);
 memcpy(str,"Game AUGame BZZ",15);
 put32(db+16,wlpdb_crc32(db+20,sizeof(db)-20));
 wlpdb_view v={0};wlpdb_result result={0};
 assert(wlpdb_open(&v,db,sizeof(db)));
 assert(v.count==2);
 assert(wlpdb_find(&v,UINT64_C(0x0005000010000001),&result));
 assert(result.platform==1 && result.region_mask==2 && result.confirmed==1);
 assert(result.name_len==6 && memcmp(result.name,"Game A",6)==0);
 assert(wlpdb_find(&v,UINT64_C(0x0005000010000002),&result));
 assert(result.platform==9 && result.confirmed==0);
 assert(!wlpdb_find(&v,UINT64_C(0x0005000010000003),&result));
 assert(!wlpdb_open(&v,db,sizeof(db)-1));
 db[96]^=1;assert(!wlpdb_open(&v,db,sizeof(db)));db[96]^=1;
 put64(b,UINT64_C(0x0005000010000000));
 put32(db+16,wlpdb_crc32(db+20,sizeof(db)-20));
 assert(!wlpdb_open(&v,db,sizeof(db)));
 puts("PASS: sorted index, region, verification, unknown, CRC, size, order");
 return 0;
}
