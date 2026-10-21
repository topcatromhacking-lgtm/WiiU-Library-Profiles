#include "../src/v134_physical_source.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
 const char *account="80000001";
 const char *logical="/vol/save/80000001/BaristaAccountSaveFile.dat";
 const char *physical="fs:/vol/external01/wiiu/homebrew_on_menu_plugin/SYNTHETIC123/save/80000001/BaristaAccountSaveFile.dat";
 assert(wlp_v134_matches_redirect(logical,physical,account));
 assert(!wlp_v134_matches_redirect(logical,"/vol/save/80000001/BaristaAccountSaveFile.dat",account));
 assert(!wlp_v134_matches_redirect(logical,"fs:/vol/external01/wiiu/homebrew_on_menu_plugin/save/80000001/BaristaAccountSaveFile.dat",account));
 assert(!wlp_v134_matches_redirect(logical,"fs:/vol/external01/wiiu/homebrew_on_menu_plugin/AB/other/80000001/BaristaAccountSaveFile.dat",account));
 assert(!wlp_v134_matches_redirect(logical,"fs:/vol/external01/wiiu/homebrew_on_menu_plugin/AB/save/80000002/BaristaAccountSaveFile.dat",account));
 assert(!wlp_v134_matches_redirect(logical,"fs:/vol/external01/wiiu/homebrew_on_menu_plugin/AB/save/80000001/BaristaAccountSaveFile.dat.bak",account));
 assert(!wlp_v134_matches_redirect(logical,"fs:/vol/external01/wiiu/homebrew_on_menu_plugin/../save/80000001/BaristaAccountSaveFile.dat",account));
 assert(!wlp_v134_matches_redirect("/vol/save/80000002/BaristaAccountSaveFile.dat",physical,account));
 assert(!wlp_v134_matches_redirect("/vol/save/80000001/Other.dat",physical,account));
 assert(!wlp_v134_matches_redirect(logical,physical,"80000002"));
 assert(!wlp_v134_matches_redirect(logical,physical,"80000a01"));
 assert(!wlp_v134_matches_redirect(NULL,physical,account));
 assert(!wlp_v134_matches_redirect(logical,NULL,account));
 assert(!wlp_v134_matches_redirect(logical,physical,NULL));
 uint32_t pos=UINT32_MAX;
 assert(wlp_v134_read_offset(4096,4096,&pos)&&pos==0);
 assert(wlp_v134_read_offset(130116,20,&pos)&&pos==130096);
 assert(!wlp_v134_read_offset(130117,20,&pos));
 assert(!wlp_v134_read_offset(7,8,&pos));
 assert(!wlp_v134_read_offset(-1,8,&pos));
 assert(!wlp_v134_read_offset(130116,0,&pos));
 assert(!wlp_v134_read_offset(130116,130117,&pos));
 assert(!wlp_v134_read_offset(1,1,NULL));
 puts("v1.3.4 physical-source and nonmoving offset tests: PASS");
 return 0;
}
