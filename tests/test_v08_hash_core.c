#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../console_menu_profile_switch/src/wlp_sha256.h"
#include "../console_menu_profile_switch/src/wlp_filter_core.h"
static void digest_hex(const uint8_t d[32],char out[65]){static const char*h="0123456789abcdef";for(int i=0;i<32;i++){out[i*2]=h[d[i]>>4];out[i*2+1]=h[d[i]&15];}out[64]=0;}
int main(void){
 uint8_t digest[32];char hex[65];wlp_sha256_ctx s;
 wlp_sha256_init(&s);wlp_sha256_final(&s,digest);digest_hex(digest,hex);
 assert(strcmp(hex,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")==0);
 wlp_sha256_init(&s);wlp_sha256_update(&s,(const uint8_t*)"abc",3);wlp_sha256_final(&s,digest);digest_hex(digest,hex);
 assert(strcmp(hex,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")==0);
 assert(wlp_is_game(UINT64_C(0x000500001010E600),WLP_GAME_APP_TYPE));
 assert(!wlp_is_game(UINT64_C(0x0005000e1010e600),WLP_GAME_APP_TYPE));
 assert(wlp_is_homebrew(UINT64_C(0x0005000FA12A4ECF)));
 puts("v0.8 portable predicate and SHA256 vectors PASS");return 0;
}
