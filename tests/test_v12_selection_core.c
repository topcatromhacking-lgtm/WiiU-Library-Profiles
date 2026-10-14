#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../console_menu_profile_preview/src/wlps_core.h"
static void p16(uint8_t*p,uint16_t x){p[0]=x>>8;p[1]=x&255;}
static void p32(uint8_t*p,uint32_t x){p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x;}
static void p64(uint8_t*p,uint64_t x){p32(p,(uint32_t)(x>>32));p32(p+4,(uint32_t)x);}
int main(void){
 uint8_t db[20+4*32]={0};
 memcpy(db,"WLPD",4);p16(db+4,1);p16(db+6,32);p32(db+8,4);
 for(int i=0;i<4;i++)p64(db+20+32*i,UINT64_C(0x0005000010000001)+i);
 uint8_t *a=db+20,*b=a+32,*c=b+32,*d=c+32;
 a[8]=1;a[9]=2;a[10]=1; /* WIIU USA */
 b[8]=9;b[9]=1;b[10]=1; /* NES JPN */
 c[8]=1;c[9]=4;c[10]=1; /* WIIU EUR, explicitly excluded */
 d[8]=9;d[9]=2;d[10]=0; /* REVIEW */
 p32(db+16,wlpdb_crc32(db+20,sizeof(db)-20));
 wlpdb_view m={0};assert(wlpdb_open(&m,db,sizeof(db)));
 uint8_t profile[32+24]={0};
 memcpy(profile,"WLPS",4);p16(profile+4,1);p16(profile+6,32);
 p16(profile+8,1);profile[10]=2;profile[11]=0;
 p16(profile+12,2);p16(profile+14,1);
 p32(profile+20,sizeof(profile));
 p64(profile+32,UINT64_C(0x0005000010000002)); /* force include NES JPN */
 p64(profile+40,UINT64_C(0x0005000010000008)); /* requested but not installed */
 p64(profile+48,UINT64_C(0x0005000010000003)); /* exclude WIIU EUR */
 p32(profile+16,wlpdb_crc32(profile+32,sizeof(profile)-32));
 wlps_view v={0};
 assert(wlps_open(&v,profile,sizeof(profile)));
 uint64_t installed[]={UINT64_C(0x0005000010000001),UINT64_C(0x0005000010000002),
 UINT64_C(0x0005000010000003),UINT64_C(0x0005000010000004),UINT64_C(0x0005000010000005)};
 wlps_item out[5]={0};wlps_stats st={0};
 assert(wlps_plan(&v,&m,installed,5,out,5,&st));
 assert(st.games==5&&st.joined==4&&st.confirmed==3&&st.review==1&&st.unknown==1);
 assert(st.excluded==1&&st.requested_missing==1&&st.eligible==2&&st.shown==2&&st.overflow==0);
 assert(out[0].id==installed[0]&&out[0].platform==1&&out[0].manual==0);
 assert(out[1].id==installed[1]&&out[1].platform==9&&out[1].manual==1);
 profile[40]^=1;assert(!wlps_open(&v,profile,sizeof(profile)));profile[40]^=1;
 p64(profile+48,UINT64_C(0x0005000010000002));
 p32(profile+16,wlpdb_crc32(profile+32,sizeof(profile)-32));
 assert(!wlps_open(&v,profile,sizeof(profile)));
 puts("PASS: validated selection, manual include, exclusion, unconfirmed, unknown, CRC, missing ID");
 return 0;
}
