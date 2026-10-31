/* IXFilm for Canon IXUS 120 IS / SD940, firmware 1.00E. */
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "dirent.h"
#include "task.h"
#include "clock.h"
#include "conf.h"
#include "shutdown.h"
#include "meminfo.h"
#include "properties.h"
#include "shooting.h"
#include "camera_info.h"
#include "sound.h"
#include "ixfilm_status.h"
#include "ixfilm_lang.h"
#include "gui_mpopup.h"
#include "simple_module.h"
#include "se.h"
#include "stock.h"
#include "look.h"
#include "jpeg_dec.h"
#include "jpeg_enc.h"

#define LOG_PATH "A/CHDK/LOGS/IXFILM.LOG"
#define TRACE_PATH "A/CHDK/LOGS/IXTRACE.LOG"
#define CFG_PATH "A/CHDK/DATA/IXFILM.CFG"
#define TEMP_PATH "A/CHDK/DATA/IXFTEMP.JPG"
#define HEADER_BYTES (32*1024)
#define ARENA_COMPACT (192*1024)
#define ARENA_FULL_FAST (320*1024)
#define ARENA_FULL_QUALITY (352*1024)
#define LOOK_BUDGET_COMPACT (56*1024)
#define LOOK_BUDGET_FULL (96*1024)
#define CFG_MAGIC 0x49584632UL
#define STOCK_COUNT 18

enum { QUALITY_COMPACT=0, QUALITY_FULL=1, QUALITY_FULL_FAST=2 };
typedef struct { unsigned long magic; int selected; char last_path[96]; int quality; int completion_beep; } ix_config;
typedef struct { FILE *file; uint32_t size; } input_stream;
typedef struct { FILE *file; unsigned long bytes; int failed; } output_stream;
typedef struct {
 se_jpeg *encoder; se_look_run *look;
 int width, out_width, resize, source_row, output_rows, failed;
 uint8_t *row;
} grade_context;

static volatile int running;
static volatile int conversion_active;
static volatile int menu_busy;
static int power_locked;
static volatile int progress_pct;
static ix_config cfg;
static cam_meminfo mem_canon_before, mem_canon_after;
static cam_meminfo mem_all_before, mem_all_after;
static int mem_arena_ok;
static unsigned long mem_arena_requested;
static int status_current, status_total, status_last_pct = -1;
static char status_stock[SE_STOCK_NAME];
static const char *stock_files[STOCK_COUNT] = {
 "acros.fs","astia.fs","bleachbypass.fs","classicchrome.fs","classicneg.fs",
 "eterna.fs","gold.fs","nostalgicneg.fs","portra.fs","provia.fs",
 "realaace.fs","soup800.fs","velvia.fs","trix400.fs",
 "hp5plus.fs","fp4plus.fs","tmax100.fs","delta3200.fs"
};
static const char *stock_names[STOCK_COUNT] = {
 "ACROS","ASTIA","BLEACH BYPASS","CLASSIC CHROME","CLASSIC NEG",
 "ETERNA","GOLD","NOSTALGIC NEG","PORTRA","PROVIA",
 "REALA ACE","SOUP 800","VELVIA","TRI-X 400",
 "HP5 PLUS","FP4 PLUS","T-MAX 100","DELTA 3200"
};
static struct mpopup_item type_menu[] = {
 {1u<<0,(int)"No film simulation"},
 {1u<<1,(int)"Color"},
 {1u<<2,(int)"Black & white"},
 {1u<<3,(int)"Conversion quality"},
 {1u<<4,(int)"Completion beep"},{0,0}
};
static struct mpopup_item color_menu[] = {
 {1u<<0,(int)"ASTIA"},{1u<<1,(int)"BLEACH BYPASS"},
 {1u<<2,(int)"CLASSIC CHROME"},{1u<<3,(int)"CLASSIC NEG"},
 {1u<<4,(int)"ETERNA"},{1u<<5,(int)"GOLD"},
 {1u<<6,(int)"NOSTALGIC NEG"},{1u<<7,(int)"PORTRA"},
 {1u<<8,(int)"PROVIA"},{1u<<9,(int)"REALA ACE"},
 {1u<<10,(int)"SOUP 800"},{1u<<11,(int)"VELVIA"},{0,0}
};
static const int color_ids[] = {2,3,4,5,6,7,8,9,10,11,12,13};
static struct mpopup_item bw_menu[] = {
 {1u<<0,(int)"ACROS"},{1u<<1,(int)"TRI-X 400"},
 {1u<<2,(int)"HP5 PLUS"},{1u<<3,(int)"FP4 PLUS"},
 {1u<<4,(int)"T-MAX 100"},{1u<<5,(int)"DELTA 3200"},{0,0}
};
static const int bw_ids[] = {1,14,15,16,17,18};
static struct mpopup_item quality_menu[] = {
 {1u<<0,(int)"Full quality 4000x3000"},
 {1u<<1,(int)"Full fast 4000x3000"},
 {1u<<2,(int)"Compact 1600x1200"},{0,0}
};
static const int quality_ids[] = {QUALITY_FULL,QUALITY_FULL_FAST,QUALITY_COMPACT};
static struct mpopup_item beep_menu[] = {
 {1u<<0,(int)"Off"},{1u<<1,(int)"On"},{0,0}
};
static void color_selected(unsigned int a);
static void bw_selected(unsigned int a);
static void quality_selected(unsigned int a);
static void beep_selected(unsigned int a);
static void show_selected_status(void);

void se_arena_init(se_arena *a,void *m,size_t n){a->base=m;a->size=n;a->used=a->peak=0;}
void *se_arena_alloc(se_arena *a,size_t n,size_t al){size_t o;if(al<1)al=1;o=(a->used+al-1)&~(al-1);if(n>a->size||o>a->size-n)return 0;a->used=o+n;if(a->used>a->peak)a->peak=a->used;return a->base+o;}
void se_arena_reset(se_arena *a){a->used=0;}
void se_set_log(se_log_fn f,void *c){(void)f;(void)c;} void se_log(const char *f,...){(void)f;}
void se_set_progress(se_progress_fn f,void *c){(void)f;(void)c;}
static void status_line(int state,int pct){ixfilm_status_update(status_stock,pct,status_current,status_total,state);}
void se_progress(const char *s,int p){(void)s;progress_pct=p;if(p!=status_last_pct){status_last_pct=p;status_line(1,p);}}
const char *se_strerror(se_err e){switch(e){case 0:return"ok";case SE_E_BADFILE:return"bad JPEG/stock";case SE_E_UNSUPPORTED:return"unsupported";case SE_E_TRUNCATED:return"truncated";case SE_E_NOMEM:return"not enough memory";case SE_E_CORRUPT:return"corrupt data";case SE_E_IO:return"card I/O error";default:return"unknown";}}
/* grade.c contains optional 3D-LUT functions, but IXFilm stocks do not use
 * them. Satisfy that unused object reference without pulling 64-bit LUT code. */
se_err se_lut3_identity(se_lut3 *l,int n,se_arena *a){(void)l;(void)n;(void)a;return SE_E_UNSUPPORTED;}

static void save_config(void){FILE*f=fopen(CFG_PATH,"wb");if(f){fwrite(&cfg,1,sizeof(cfg),f);fclose(f);}}
static void trace_event(const char*phase,const char*path,long a,long b){char s[220];int n;FILE*f=fopen(TRACE_PATH,"ab");if(!f)return;n=sprintf(s,"tick=%lu phase=%s path=%s a=%ld b=%ld\n",get_tick_count(),phase,path?path:"-",a,b);fwrite(s,1,n,f);fclose(f);}
static void load_config(void){FILE*f;memset(&cfg,0,sizeof(cfg));f=fopen(CFG_PATH,"rb");if(f){fread(&cfg,1,sizeof(cfg),f);fclose(f);}if(cfg.magic!=CFG_MAGIC||cfg.selected<0||cfg.selected>STOCK_COUNT){memset(&cfg,0,sizeof(cfg));cfg.magic=CFG_MAGIC;}if(cfg.quality<QUALITY_COMPACT||cfg.quality>QUALITY_FULL_FAST)cfg.quality=QUALITY_COMPACT;if(cfg.completion_beep!=1)cfg.completion_beep=0;}

static int find_latest_jpeg(char*out){DIR*d,*id;struct dirent*de,*ie;char dp[80],candidate[96];out[0]=0;d=opendir("A/DCIM");if(!d)return 0;while((de=readdir(d))){if(de->d_name[0]=='.'||de->d_name[0]=='\xE5')continue;sprintf(dp,"A/DCIM/%s",de->d_name);id=opendir(dp);if(!id)continue;while((ie=readdir(id))){char*n=ie->d_name;if(strlen(n)==12&&!strncmp(n,"IMG_",4)&&n[8]=='.'&&(!strcmp(n+9,"JPG")||!strcmp(n+9,"jpg"))){sprintf(candidate,"%s/%s",dp,n);if(!out[0]||strcmp(candidate,out)>0)strcpy(out,candidate);}}closedir(id);}closedir(d);return out[0]!=0;}
static int scan_pending(const char*after,char*next,char*latest){DIR*d,*id;struct dirent*de,*ie;char dp[80],p[96];int count=0;next[0]=latest[0]=0;d=opendir("A/DCIM");if(!d)return 0;while((de=readdir(d))){if(de->d_name[0]=='.'||de->d_name[0]=='\xE5')continue;sprintf(dp,"A/DCIM/%s",de->d_name);id=opendir(dp);if(!id)continue;while((ie=readdir(id))){char*n=ie->d_name;if(strlen(n)==12&&!strncmp(n,"IMG_",4)&&n[8]=='.'&&(!strcmp(n+9,"JPG")||!strcmp(n+9,"jpg"))){sprintf(p,"%s/%s",dp,n);if(!latest[0]||strcmp(p,latest)>0)strcpy(latest,p);if((!after[0]||strcmp(p,after)>0)){count++;if(!next[0]||strcmp(p,next)<0)strcpy(next,p);}}}closedir(id);}closedir(d);return count;}

static char *load_stock(int sel,size_t*len){char p[96],*t;struct stat st;FILE*f;if(sel<1||sel>STOCK_COUNT)return 0;sprintf(p,"A/CHDK/FILM/%s",stock_files[sel-1]);if(stat(p,&st)||st.st_size<16||st.st_size>8192)return 0;t=malloc(st.st_size+1);if(!t)return 0;f=fopen(p,"rb");if(!f||fread(t,1,st.st_size,f)!=(size_t)st.st_size){if(f)fclose(f);free(t);return 0;}fclose(f);t[st.st_size]=0;*len=st.st_size;return t;}

static uint16_t ex16(const uint8_t*p,int le){return le?(uint16_t)(p[0]|p[1]<<8):(uint16_t)(p[0]<<8|p[1]);}
static uint32_t ex32(const uint8_t*p,int le){return le?((uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24):((uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]);}
static void put16(uint8_t*p,int le,uint16_t v){if(le){p[0]=v;p[1]=v>>8;}else{p[0]=v>>8;p[1]=v;}}
static void put32(uint8_t*p,int le,uint32_t v){if(le){p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;}else{p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}}
static void patch_ifd(uint8_t*t,size_t n,uint32_t off,int le,int w,int h,uint32_t*exifoff){uint16_t count,i;if(off+2>n)return;count=ex16(t+off,le);if(off+2+(uint32_t)count*12>n)return;for(i=0;i<count;i++){uint8_t*e=t+off+2+i*12;uint16_t tag=ex16(e,le),type=ex16(e+2,le);uint32_t val=ex32(e+8,le);if(tag==0x8769&&exifoff)*exifoff=val;if(tag==0x0100||tag==0xa002){if(type==3)put16(e+8,le,w);else if(type==4)put32(e+8,le,w);}if(tag==0x0101||tag==0xa003){if(type==3)put16(e+8,le,h);else if(type==4)put32(e+8,le,h);}}}
static int preserve_exif(uint8_t*h,size_t n,int w,int ht,se_jpeg_exif*x){size_t p=2;while(p+4<n&&h[p]==0xff){uint8_t m=h[p+1];uint16_t sl;if(m==0xda||m==0xd9)break;sl=(uint16_t)(h[p+2]<<8|h[p+3]);if(sl<2||p+2+sl>n)break;if(m==0xe1&&sl>14&&!memcmp(h+p+4,"Exif\0\0",6)){uint8_t*t=h+p+10;size_t tn=sl-8;int le=(t[0]=='I'&&t[1]=='I');uint32_t exoff=0;if((le|| (t[0]=='M'&&t[1]=='M'))&&ex16(t+2,le)==42){patch_ifd(t,tn,ex32(t+4,le),le,w,ht,&exoff);if(exoff)patch_ifd(t,tn,exoff,le,w,ht,0);x->raw_app1=h+p+4;x->raw_app1_len=sl-2;return 1;}}p+=2+sl;}return 0;}
static int stream_read(void*c,uint32_t o,void*d,uint32_t n){input_stream*i=c;if(o>i->size||n>i->size-o||fseek(i->file,o,SEEK_SET))return-1;return fread(d,1,n,i->file)==n?0:-1;}
static int stream_write(void*c,const uint8_t*d,size_t n){output_stream*o=c;if(o->failed||fwrite(d,1,n,o->file)!=n){o->failed=1;return-1;}o->bytes+=n;return 0;}
static int look_emit(void*c,const uint8_t*r,int w){grade_context*g=c;const uint8_t*out=r;int x;if(w!=g->width){g->failed=1;return-1;}if(g->resize){/* 2000x1500 -> Canon-native 1600x1200. Select the same 4/5 grid in both axes, streaming one row at a time. */if((g->source_row%5)==4){g->source_row++;return 0;}for(x=0;x<g->out_width;x++){int sx=(x*5)/4;g->row[x*3]=r[sx*3];g->row[x*3+1]=r[sx*3+1];g->row[x*3+2]=r[sx*3+2];}out=g->row;}g->source_row++;if(se_jpeg_write_rows(g->encoder,out,1)!=SE_OK){g->failed=1;return-1;}g->output_rows++;return 0;}
static int grade_rows(void*c,const uint8_t*r,int n,int w){grade_context*g=c;if(w!=g->width||se_look_rows(g->look,r,n,look_emit,g)!=SE_OK){g->failed=1;return-1;}return 0;}

static const char* quality_name(int q){return q==QUALITY_FULL?"full-quality":q==QUALITY_FULL_FAST?"full-fast":"compact";}
static void write_log(const char*p,const char*s,int q,se_err rc,unsigned long ms,size_t peak,int w,int h,unsigned long bytes){char b[900];int n;FILE*f=fopen(LOG_PATH,"wb");if(!f)return;n=sprintf(b,"IXFilm\ninput=%s\nstock=%s\nquality=%s\nresult=%d (%s)\nsize=%dx%d\nelapsed_ms=%lu\narena_requested=%lu\narena_allocated=%s\narena_peak=%lu\noutput_bytes=%lu\nprogress=%d\ncanon_before_free=%d\ncanon_before_largest=%d\ncanon_before_blocks=%d\nall_before_free=%d\nall_before_largest=%d\nall_before_blocks=%d\ncanon_after_alloc_free=%d\ncanon_after_alloc_largest=%d\ncanon_after_alloc_blocks=%d\nall_after_alloc_free=%d\nall_after_alloc_largest=%d\nall_after_alloc_blocks=%d\n",p,s,quality_name(q),rc,se_strerror(rc),w,h,ms,mem_arena_requested,mem_arena_ok?"yes":"no",(unsigned long)peak,bytes,progress_pct,mem_canon_before.free_size,mem_canon_before.free_block_max_size,mem_canon_before.free_block_count,mem_all_before.free_size,mem_all_before.free_block_max_size,mem_all_before.free_block_count,mem_canon_after.free_size,mem_canon_after.free_block_max_size,mem_canon_after.free_block_count,mem_all_after.free_size,mem_all_after.free_block_max_size,mem_all_after.free_block_count);fwrite(b,1,n,f);fclose(f);}

static se_err develop(const char*path,int sel){
 struct stat st;FILE*src=0,*dst=0;uint8_t*head=0,*mem=0,*strip;char*txt=0,bak[96];size_t tlen=0,arena_bytes,look_budget;se_arena ar;se_jdec_info ji;se_jpeg*enc;se_jpeg_exif ex;se_stock stock;se_look look;se_look_run*lr=0;input_stream in;output_stream out;grade_context gr;se_err rc=SE_E_IO;int w=0,h=0,ow=0,oh=0,scale,jpeg_quality,q=cfg.quality;unsigned long started=get_tick_count();
 memset(&ar,0,sizeof(ar));memset(&in,0,sizeof(in));memset(&out,0,sizeof(out));memset(&gr,0,sizeof(gr));memset(&stock,0,sizeof(stock));progress_pct=0;trace_event("develop-begin",path,q,sel);
 scale=q==QUALITY_COMPACT?2:1;jpeg_quality=q==QUALITY_FULL_FAST?86:90;arena_bytes=q==QUALITY_FULL?ARENA_FULL_QUALITY:q==QUALITY_FULL_FAST?ARENA_FULL_FAST:ARENA_COMPACT;look_budget=q==QUALITY_COMPACT?LOOK_BUDGET_COMPACT:LOOK_BUDGET_FULL;mem_arena_requested=arena_bytes;
 memset(&mem_canon_before,0,sizeof(mem_canon_before));memset(&mem_all_before,0,sizeof(mem_all_before));memset(&mem_canon_after,0,sizeof(mem_canon_after));memset(&mem_all_after,0,sizeof(mem_all_after));mem_arena_ok=0;GetMemInfo(&mem_canon_before);GetCombinedMemInfo(&mem_all_before);
 txt=load_stock(sel,&tlen);if(!txt)goto done;rc=se_stock_parse(txt,tlen,&stock);if(rc)goto done;trace_event("stock-ready",path,tlen,0);strcpy(status_stock,stock.name);status_last_pct=-1;status_line(1,0);
 if(stat(path,&st)||st.st_size<HEADER_BYTES)goto done;
 src=fopen(path,"rb");if(!src)goto done;
 /* Reserve every small fixed object before taking the 448 KB arena. With
  * the arena first there is no 32 KB block left for the JPEG header; with
  * fopen last there is too little heap left for its FILE/buffer. The largest
  * Canon block is still over 500 KB after these small allocations. */
 head=malloc(HEADER_BYTES);if(!head){rc=SE_E_NOMEM;goto done;}
 remove(TEMP_PATH);dst=fopen(TEMP_PATH,"wb");if(!dst){rc=SE_E_IO;goto done;}out.file=dst;
 mem=malloc(arena_bytes);mem_arena_ok=(mem!=0);GetMemInfo(&mem_canon_after);GetCombinedMemInfo(&mem_all_after);trace_event("arena",path,arena_bytes,mem_arena_ok);if(!mem){rc=SE_E_NOMEM;goto done;}if(fread(head,1,HEADER_BYTES,src)!=HEADER_BYTES)goto done;
 rc=se_jpeg_dec_info(head,HEADER_BYTES,&ji);if(rc)goto done;se_jpeg_scaled_size(ji.width,ji.height,scale,&w,&h);ow=w;oh=h;if(q==QUALITY_COMPACT&&w==2000&&h==1500){ow=1600;oh=1200;}trace_event("jpeg-info",path,ow,oh);se_arena_init(&ar,mem,arena_bytes);
 enc=SE_NEW(&ar,se_jpeg,1);strip=SE_NEW(&ar,uint8_t,(size_t)8*ow*3);if(!enc||!strip){rc=SE_E_NOMEM;goto done;}
 se_look_from_stock(&stock,scale,&look);if(q==QUALITY_FULL_FAST){look.hal_amount=0;look.grain=0;}se_look_fit(&look,w,look_budget);rc=se_look_begin(&lr,&look,w,&ar);if(rc)goto done;
 memset(&ex,0,sizeof(ex));if(!preserve_exif(head,HEADER_BYTES,ow,oh,&ex)){ex.make="Canon";ex.model="Digital IXUS 120 IS";ex.software=stock.name;}
 rc=se_jpeg_begin(enc,ow,oh,jpeg_quality,strip,&ex,stream_write,&out);if(rc)goto done;in.file=src;in.size=st.st_size;gr.encoder=enc;gr.look=lr;gr.width=w;gr.out_width=ow;gr.resize=(ow!=w);if(gr.resize){gr.row=SE_NEW(&ar,uint8_t,(size_t)ow*3);if(!gr.row){rc=SE_E_NOMEM;goto done;}}
 trace_event("decode-start",path,st.st_size,0);rc=se_jpeg_decode_stream_scaled(head,HEADER_BYTES,st.st_size,stream_read,&in,scale,&ar,grade_rows,&gr,&ji);trace_event("decode-end",path,rc,progress_pct);if(rc==SE_OK&&!gr.failed)rc=se_look_end(lr,look_emit,&gr);if(rc==SE_OK&&!gr.failed)rc=se_jpeg_end(enc);if(out.failed||gr.failed)rc=SE_E_IO;
done: if(dst)fclose(dst);if(src)fclose(src);trace_event("develop-end",path,rc,out.bytes);write_log(path,(txt&&stock.name[0])?stock.name:"?",q,rc,get_tick_count()-started,ar.peak,ow,oh,out.bytes);if(mem)free(mem);if(head)free(head);if(txt)free(txt);if(rc){remove(TEMP_PATH);return rc;}
 strcpy(bak,path);strcpy(bak+strlen(bak)-3,"BAK");remove(bak);if(rename(path,bak)){remove(TEMP_PATH);trace_event("rename-source-failed",path,0,0);return SE_E_IO;}if(rename(TEMP_PATH,path)){rename(bak,path);remove(TEMP_PATH);trace_event("rename-output-failed",path,0,0);return SE_E_IO;}remove(bak);trace_event("commit-ok",path,out.bytes,0);return SE_OK;
}

static void set_selected_label(void){if(!cfg.selected)strcpy(status_stock,"OFF");else sprintf(status_stock,"%s [%s]",stock_names[cfg.selected-1],cfg.quality==QUALITY_FULL?"FULL":cfg.quality==QUALITY_FULL_FAST?"FAST":"COMPACT");}
static void watcher_task(void)
{
    char p[96],latest[96];
    struct stat a,b;
    int pending,done;
    load_config();
    while(running){
        pending=scan_pending(cfg.last_path,p,latest);
        if(cfg.selected==0){
            if(latest[0]&&strcmp(latest,cfg.last_path)){
                strcpy(cfg.last_path,latest);save_config();
            }
        }else if(pending>0){
            done=0;trace_event("queue-found",p,pending,cfg.quality);if(!power_locked){disable_shutdown();power_locked=1;}conversion_active=1;
            status_line(7,0);
            while(running&&pending>0){
                status_current=done+1;status_total=done+pending;
                /* Announce the queued conversion immediately. The two-second
                 * size check below is still needed to avoid reading Canon's
                 * JPEG while it is being written, but should not look like an
                 * unexplained pause to the user. */
                status_last_pct=-1;status_line(1,0);
                if(!stat(p,&a)){
                    msleep(2000);
                    if(!stat(p,&b)&&a.st_size==b.st_size){
                        se_err rc=develop(p,cfg.selected);
                        if(rc==SE_OK){
                            strcpy(cfg.last_path,p);save_config();done++;
                            if(cfg.completion_beep)play_sound(4);
                            status_line(2,100);
                        }else{
                            /* Keep the original JPEG, log the failure, but do
                             * not let one bad image block every later photo. */
                            strcpy(cfg.last_path,p);save_config();
                            status_line(3,-1);
                            break;
                        }
                    }
                }
                pending=scan_pending(cfg.last_path,p,latest);
            }
            conversion_active=0;
            if(running&&done>0&&pending==0){
                msleep(1500);set_selected_label();status_line(6,0);
            }else status_line(8,0);
            if(power_locked){enable_shutdown();power_locked=0;}
        }
        msleep(1000);
    }
    ExitTask();
}
static void show_selected_status(void){set_selected_label();status_line(5,0);}
static void menu_finished(void){menu_busy=0;show_selected_status();}
static void select_stock(int id){char p[96];cfg.selected=id;if(find_latest_jpeg(p))strcpy(cfg.last_path,p);save_config();}
static void color_selected(unsigned int a){int i;for(i=0;i<(int)(sizeof(color_ids)/sizeof(color_ids[0]));i++)if(a==(1u<<i)){select_stock(color_ids[i]);break;}menu_finished();}
static void bw_selected(unsigned int a){int i;for(i=0;i<(int)(sizeof(bw_ids)/sizeof(bw_ids[0]));i++)if(a==(1u<<i)){select_stock(bw_ids[i]);break;}menu_finished();}
static void quality_selected(unsigned int a){int i;for(i=0;i<3;i++)if(a==(1u<<i)){cfg.quality=quality_ids[i];save_config();break;}menu_finished();}
static void beep_selected(unsigned int a){if(a==(1u<<0)||a==(1u<<1)){cfg.completion_beep=(a==(1u<<1));save_config();}menu_finished();}
static void type_selected(unsigned int a){
 if(a==(1u<<0)){select_stock(0);menu_finished();}
 else if(a==(1u<<1))libmpopup->show_popup(color_menu,0x0fff|MPOPUP_RESTORE,color_selected);
 else if(a==(1u<<2))libmpopup->show_popup(bw_menu,0x003f|MPOPUP_RESTORE,bw_selected);
 else if(a==(1u<<3))libmpopup->show_popup(quality_menu,0x0007|MPOPUP_RESTORE,quality_selected);
 else if(a==(1u<<4))libmpopup->show_popup(beep_menu,0x0003|MPOPUP_RESTORE,beep_selected);
 else menu_finished();
}
/* PROPCASE_LANGUAGE is 61 on the IXUS 120 / propset 3. Modules are built
 * without a target propset define, so keep this target-specific ID local. */
static void show_stock_menu(void){const ixfilm_texts*t;if(menu_busy)return;menu_busy=1;t=&ixfilm_languages[ixfilm_language_index(shooting_get_prop(61)>>8)];type_menu[0].text=(int)t->none;type_menu[1].text=(int)t->color;type_menu[2].text=(int)t->bw;type_menu[3].text=(int)t->quality;type_menu[4].text=(int)t->beep;quality_menu[0].text=(int)t->full_quality;quality_menu[1].text=(int)t->full_fast;quality_menu[2].text=(int)t->compact;beep_menu[0].text=(int)t->off;beep_menu[1].text=(int)t->on;libmpopup->show_popup(type_menu,0x001f|MPOPUP_RESTORE,type_selected);}
static int module_run(void){if(running){show_stock_menu();return 0;}running=1;load_config();show_selected_status();CreateTask("IXFilm",0x19,0x4000,watcher_task);return 0;}
static int module_can_unload(void){return!running;}static int module_exit_alt(void){return 0;}
libsimple_sym _librun={{0,0,module_can_unload,module_exit_alt,module_run}};
ModuleInfo _module_info={MODULEINFO_V1_MAGICNUM,sizeof(ModuleInfo),{0,2},ANY_CHDK_BRANCH,0,OPT_ARCHITECTURE,ANY_PLATFORM_ALLOWED,(int32_t)"IXFilm",MTYPE_TOOL,&_librun.base,ANY_VERSION,ANY_VERSION,ANY_VERSION,ANY_VERSION,0};
