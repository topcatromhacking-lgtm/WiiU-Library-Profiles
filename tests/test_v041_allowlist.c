/* Portable host-side test of v0.4.1 read-only Menu allowlist logic. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "../console_menu_allowlist_dry_run/src/allowlist_core.h"

static void set_be32(unsigned char *p,uint32_t x){
  p[0]=(unsigned char)(x>>24); p[1]=(unsigned char)(x>>16);
  p[2]=(unsigned char)(x>>8);p[3]=(unsigned char)x;
}
static void set_be64(unsigned char *p,uint64_t x){
  set_be32(p,(uint32_t)(x>>32));set_be32(p+4,(uint32_t)x);
}
int main(void){
  const uint64_t g1=UINT64_C(0x0005000010000001);
  const uint64_t g2=UINT64_C(0x0005000010000002);
  const uint64_t h1=UINT64_C(0x0005000F00000001);
  const uint64_t h2=UINT64_C(0x0005000F00000002);
  const uint64_t s1=UINT64_C(0x0005001010040100);
  const uint64_t patch=UINT64_C(0x0005000E10000001);
  const uint64_t dlc=UINT64_C(0x0005000C10000001);
  unsigned char b[WLP_HEADER_BYTES+3u*8u+4u]={0};
  memcpy(b,"WLPA",4); b[5]=1; set_be32(b+8,3);
  set_be64(b+44,g1); set_be64(b+52,h1); set_be64(b+60,s1);
  set_be32(b+68,wlp_crc32(b,68));
  wlp_allowlist p={0};
  assert(wlp_parse_allowlist(b,sizeof(b),&p));
  assert(p.count==3);
  assert(wlp_contains(&p,g1)&&wlp_contains(&p,h1)&&wlp_contains(&p,s1));
  assert(!wlp_contains(&p,g2)&&!wlp_contains(&p,h2));
  assert(!wlp_hide_from_menu(&p,g1,WLP_GAME_APP_TYPE));
  assert(wlp_hide_from_menu(&p,g2,WLP_GAME_APP_TYPE));
  assert(wlp_hide_from_menu(&p,g2,WLP_WII_APP_TYPE));
  assert(wlp_hide_from_menu(&p,h2,0)); /* synthetic any app type */
  assert(!wlp_hide_from_menu(&p,s1,0));
  assert(!wlp_hide_from_menu(&p,patch,WLP_GAME_APP_TYPE));
  assert(!wlp_hide_from_menu(&p,dlc,WLP_GAME_APP_TYPE));
  assert(!wlp_hide_from_menu(&p,g2,UINT32_C(0x40000000))); /* unknown fail-open */

  wlp_observed real_menu[]={
    {g1,WLP_GAME_APP_TYPE},{g2,WLP_GAME_APP_TYPE},
    {h1,0},{h2,0},{s1,0},{patch,WLP_GAME_APP_TYPE},{dlc,WLP_GAME_APP_TYPE}
  };
  wlp_dry_stats stats=wlp_dry_evaluate(&p,real_menu,7);
  assert(stats.expected_game==1&&stats.present_game==1&&stats.missing_game==0);
  assert(stats.expected_homebrew==1&&stats.present_homebrew==1&&stats.missing_homebrew==0);
  assert(stats.expected_other==1&&stats.present_other==1&&stats.missing_other==0);
  assert(stats.game_records==2&&stats.homebrew_records==2);
  assert(stats.would_hide_game==1&&stats.would_hide_homebrew==1);
  wlp_observed missing[]={{g2,WLP_GAME_APP_TYPE},{h2,0},{patch,0}};
  stats=wlp_dry_evaluate(&p,missing,3);
  assert(stats.missing_game==1&&stats.missing_homebrew==1&&stats.missing_other==1);
  b[55]^=0x40;
  assert(!wlp_parse_allowlist(b,sizeof(b),&p)); /* CRC */
  b[55]^=0x40;
  b[5]=2;
  assert(!wlp_parse_allowlist(b,sizeof(b),&p)); /* wrong version */
  b[5]=1;
  assert(!wlp_parse_allowlist(b,sizeof(b)-1,&p)); /* truncation */
  puts("v0.4.1 read-only predicate and manifest tests PASS");
  return 0;
}
