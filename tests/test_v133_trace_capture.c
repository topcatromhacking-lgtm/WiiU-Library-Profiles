#include "../src/v133_trace_capture.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char**argv){
char out[65];wlp_sha256_hex("",0,out);
assert(strcmp(out,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")==0);
wlp_sha256_hex("abc",3,out);
assert(strcmp(out,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")==0);
if(argc<3)return 0;
FILE*f=fopen(argv[1],"rb");if(!f)return 2;
WlpTrace*t=(WlpTrace*)calloc(1,sizeof *t);if(!t)return 3;
unsigned char*data=(unsigned char*)malloc(130116);if(!data)return 4;
assert(fread(data,1,130116,f)==130116);fclose(f);
assert(wlp_trace_init(t,"offline-synthetic-20261020"));
assert(wlp_trace_init_save(t,1,1,1,"80000001"));
assert(wlp_trace_register(t,1,11));
assert(wlp_trace_open(t,11,55,1));
for(size_t i=0;i<130116;i+=4096){size_t n=130116-i;if(n>4096)n=4096;assert(wlp_trace_read(t,55,i,data+i,n));}
assert(wlp_trace_close(t,55,1));
assert(wlp_trace_remove(t,11));
assert(wlp_trace_end(t));
f=fopen(argv[2],"wb");if(!f)return 5;
assert(fwrite(t->data,1,t->used,f)==t->used);fclose(f);
free(data);free(t);return 0;
}
