#ifndef WLPDB_CORE_H
#define WLPDB_CORE_H
/* Tiny, allocation-free SD database decoder; host C11/Wii U compatible.
 * Caller loads complete .wlpdb into memory ONCE, outside hot-path lookups.
 * No filesystem operations occur during lookup. Does not modify source bytes.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define WLPDB_MAX_RECORDS 32768u
#define WLPDB_RECORD_SIZE 32u
#define WLPDB_HEADER_SIZE 20u
#define WLPDB_MAGIC "WLPD"

typedef struct { const uint8_t *bytes; size_t size; uint32_t count; const uint8_t *strings; uint32_t pool_size;} wlpdb_view;
typedef struct {uint64_t tid; uint8_t platform, region_mask, confirmed; const uint8_t *name, *product_code; uint16_t name_len, code_len;} wlpdb_result;
static inline uint16_t wlpdb_u16(const uint8_t *p){return (uint16_t)(((uint16_t)p[0]<<8)|p[1]);}
static inline uint32_t wlpdb_u32(const uint8_t *p){return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];}
static inline uint64_t wlpdb_u64(const uint8_t *p){return ((uint64_t)wlpdb_u32(p)<<32)|wlpdb_u32(p+4);}
static inline uint32_t wlpdb_crc32(const uint8_t *p, size_t n){
    uint32_t crc=0xffffffffu;
    for(size_t i=0;i<n;i++){
        crc ^=p[i];
        for(unsigned j=0;j<8;j++)crc=(crc>>1)^((crc&1u)?0xedb88320u:0u);
    }
    return ~crc;
}
static inline int wlpdb_open(wlpdb_view *v,const uint8_t *raw,size_t len){
    if(!v)return 0;
    memset(v,0,sizeof(*v));
    if(!raw||len<WLPDB_HEADER_SIZE||memcmp(raw,WLPDB_MAGIC,4))return 0;
    if(wlpdb_u16(raw+4)!=1||wlpdb_u16(raw+6)!=WLPDB_RECORD_SIZE)return 0;
    const uint32_t n=wlpdb_u32(raw+8),pool=wlpdb_u32(raw+12);
    if(n>WLPDB_MAX_RECORDS|| (size_t)n> (len-WLPDB_HEADER_SIZE)/WLPDB_RECORD_SIZE)return 0;
    size_t offset=WLPDB_HEADER_SIZE+(size_t)n*WLPDB_RECORD_SIZE;
    if((size_t)pool!=len-offset || wlpdb_u32(raw+16)!=wlpdb_crc32(raw+20,len-20))return 0;
    uint64_t last=0;
    for(uint32_t i=0;i<n;i++){
        const uint8_t *p=raw+WLPDB_HEADER_SIZE+(size_t)i*WLPDB_RECORD_SIZE;
        uint64_t id=wlpdb_u64(p);
        if(i && id<=last)return 0;
        if(p[8]>9||p[9]>7||p[10]>1||p[11]||wlpdb_u16(p+18)||wlpdb_u16(p+26)||wlpdb_u32(p+28))return 0;
        if(wlpdb_u32(p+12)>pool||(size_t)wlpdb_u16(p+16)>pool-wlpdb_u32(p+12))return 0;
        if(wlpdb_u32(p+20)>pool||(size_t)wlpdb_u16(p+24)>pool-wlpdb_u32(p+20))return 0;
        last=id;
    }
    v->bytes=raw;v->size=len;v->count=n;v->strings=raw+offset;v->pool_size=pool;
    return 1;
}
static inline int wlpdb_find(const wlpdb_view *v,uint64_t tid,wlpdb_result *out){
    if(!v||!v->bytes||!out)return 0;
    uint32_t lo=0, hi=v->count;
    while(lo<hi){
        uint32_t mid=lo+(hi-lo)/2;
        const uint8_t *p=v->bytes+WLPDB_HEADER_SIZE+(size_t)mid*WLPDB_RECORD_SIZE;
        uint64_t id=wlpdb_u64(p);
        if(id<tid)lo=mid+1;
        else hi=mid;
    }
    if(lo==v->count)return 0;
    const uint8_t *p=v->bytes+WLPDB_HEADER_SIZE+(size_t)lo*WLPDB_RECORD_SIZE;
    if(wlpdb_u64(p)!=tid)return 0;
    out->tid=tid;out->platform=p[8];out->region_mask=p[9];out->confirmed=p[10];
    out->name=v->strings+wlpdb_u32(p+12);out->name_len=wlpdb_u16(p+16);
    out->product_code=v->strings+wlpdb_u32(p+20);out->code_len=wlpdb_u16(p+24);
    return 1;
}
#endif
