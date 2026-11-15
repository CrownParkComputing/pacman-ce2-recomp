/* The game's version, logged first so a bug report's log tells which dump it came from: param.sfo's
 * TITLE_ID and APP_VER, and what scripts/prepare.py records in analysis.json beside the image: the
 * eboot's SHA-256 and the verified build it is (VERIFIED_BUILDS; empty for one prepared with
 * --any-build). */
#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "gpu/tempest_gpu.h"

/* The string value of "key" in analysis.json's head (it is written first), at most size-1 chars of
 * [0-9a-z.]; 0 when absent. */
static int report_string(const char *head, const char *key, char *out, size_t size) {
    char pattern[64];
    snprintf(pattern,sizeof(pattern),"\"%s\": \"",key);
    const char *value=strstr(head,pattern);
    if (!value) return 0;
    value+=strlen(pattern);
    const size_t n=strspn(value,"0123456789abcdefghijklmnopqrstuvwxyz.");
    if (n>=size || value[n]!='"') return 0;
    memcpy(out,value,n); out[n]=0;
    return 1;
}

void runtime_game_identify(const char *title_id, const char *app_version, const char *app0, const char *image_path) {
    (void)app0;
    char sha[65]="", build[16]="", path[4096], head[4096];
    const char *slash=strrchr(image_path,'/'), *back=strrchr(image_path,'\\');
    if (back>slash) slash=back;
    const int dir=slash ? (int)(slash-image_path)+1 : 0;
    snprintf(path,sizeof(path),"%.*sanalysis.json",dir,image_path);
    FILE *f=fopen(path,"rb");
    if (f) {
        const size_t n=fread(head,1,sizeof(head)-1,f);
        fclose(f);
        head[n]=0;
        if (!report_string(head,"source_sha256",sha,sizeof(sha)) || strlen(sha)!=64) sha[0]=0;
        report_string(head,"verified_build",build,sizeof(build));
    }
    printf("Game: %s version %s, eboot.bin sha256 %s (%s%s)\n",title_id[0] ? title_id : "?",
           app_version[0] ? app_version : "unknown",sha[0] ? sha : "unknown",
           build[0] ? "verified build " : "not a verified build",build);
}

/* The shipped Assembly-CSharp AOT globals identify methods=0x100 and
 * code_offsets=0x613b28. MethodDef 0x0600159b (RenderTextureManager.Awake) is
 * methods+code_offsets[0x159a]=0x1eb680. It constructs the authored 1080/900/720
 * presets, then selects p1080 with the call below before allocating any targets.
 * This loader-only hook runs before Mono boot or FEX translation; it must not
 * be reused to change already-translated/running guest code. */
static void configure_ce2_effects_720(unsigned char *image,const RuntimeModule *module,uint32_t width) {
    const char *option=getenv("TEMPEST_CE2_EFFECTS_720");
    if (!option || strcmp(option,"1")) return;
    if (width!=1280) {
        puts("CE2: effects preset unchanged (720 preset requires the 1280 engine profile)");
        return;
    }
#ifdef _WIN32
    (void)image; (void)module;
    puts("CE2: effects preset unchanged (guarded preset hook requires Linux or Android)");
#else
    static const unsigned char prologue[]={
        0x48,0x83,0xec,0x38,0x4c,0x89,0x3c,0x24,0x4c,0x8b,0xff,0x4d,0x85,0xff,
        0x0f,0x84,0x53,0x01,0x00,0x00};
    /* mov rdi,r15; mov esi,1; call GetResolutionSetting's AOT PLT entry. */
    static const unsigned char callsite[]={
        0x49,0x8b,0xff,0xbe,0x01,0x00,0x00,0x00,0xe8,0xdd,0xd5,0x4e,0x00};
    const uint64_t method_offset=UINT64_C(0x1eb680),call_offset=UINT64_C(0x1eb78a);
    if (module->base>UINT64_MAX-call_offset-sizeof(callsite)) {
        puts("CE2: effects preset unchanged (module bounds differ)");
        return;
    }
    const uint64_t method=module->base+method_offset,call=module->base+call_offset;
    const uint64_t end=call+sizeof(callsite);
    const RuntimeModuleSegment *code=NULL;
    for (uint32_t s=0;s<module->segment_count && s<4;++s) {
        const RuntimeModuleSegment *segment=&module->segments[s];
        if ((segment->prot&5)==5 && segment->size>=end-method &&
            method>=segment->address && method-segment->address<=segment->size-(end-method)) {
            code=segment; break;
        }
    }
    if (!code || memcmp(image+method,prologue,sizeof(prologue)) ||
        memcmp(image+call,callsite,sizeof(callsite))) {
        puts("CE2: effects preset unchanged (Awake executable segment or preset signature differs)");
        return;
    }
    const long page_size=sysconf(_SC_PAGESIZE);
    unsigned char *preset=image+call+3;
    if (page_size<=0 || (uintptr_t)preset>UINTPTR_MAX-5-(uint64_t)page_size) {
        puts("CE2: effects preset unchanged (host page size or address differs)");
        return;
    }
    const uintptr_t page=(uintptr_t)preset-(uintptr_t)preset%(uintptr_t)page_size;
    const uintptr_t page_end=((uintptr_t)preset+5+(uintptr_t)page_size-1)/
                             (uintptr_t)page_size*(uintptr_t)page_size;
    const uintptr_t code_begin=(uintptr_t)(image+code->address);
    if (page<code_begin || page_end<page || page_end-code_begin>code->size) {
        puts("CE2: effects preset unchanged (host page crosses executable segment bounds)");
        return;
    }
    int restore=(code->prot&4 ? PROT_READ : 0)|(code->prot&2 ? PROT_WRITE : 0);
#ifndef TEMPEST_FEX
    if (code->prot&1) restore|=PROT_EXEC;
#endif
    if (mprotect((void *)page,page_end-page,PROT_READ|PROT_WRITE)) {
        printf("CE2: effects preset unchanged (cannot open code page for patch, errno %d)\n",errno);
        return;
    }
    const unsigned char p720[]={0xbe,0x03,0x00,0x00,0x00};
    memcpy(preset,p720,sizeof(p720));
    __builtin___clear_cache((char *)preset,(char *)preset+sizeof(p720));
    if (mprotect((void *)page,page_end-page,restore)) {
        fprintf(stderr,"STOP: CE2 effects preset code protection restore failed, errno %d\n",errno);
        exit(21);
    }
    puts("CE2: authored effects preset p720 (1280x720), selected before RenderTexture allocation");
#endif
}

void runtime_game_configure_modules(unsigned char *image, const RuntimeModule *modules, uint64_t count) {
    const char *option=getenv("TEMPEST_CE2_RENDER_WIDTH");
    if (!option || !*option) return;
    char *end=NULL;
    unsigned long requested=strtoul(option,&end,10);
    if (!end || *end || (requested!=1280 && requested!=1920)) {
        puts("CE2: ignoring render width (supported values: 1280, 1920)");
        return;
    }
    /* CE2's Unity constructor reads this exported render-width variable before
     * allocating its 16:9 targets. Locate the data symbol rather than an eboot
     * instruction address, and require the expected writable, initialized value. */
    for (uint64_t i=0;i<count;++i) {
        const RuntimeModule *m=&modules[i];
        if (strcmp(m->name,"MonoAssembliesPS4.prx")) continue;
        for (uint64_t e=0;e<m->export_count;++e) {
            const RuntimeModuleExport *symbol=&m->exports[e];
            if (strcmp(symbol->nid,"QBbo2kzANmQ")) continue;
            int writable=0;
            for (uint32_t s=0;s<m->segment_count && s<4;++s) {
                const RuntimeModuleSegment *segment=&m->segments[s];
                if ((segment->prot&2) && segment->size>=4 &&
                    symbol->address>=segment->address &&
                    symbol->address-segment->address<=segment->size-4) writable=1;
            }
            uint32_t previous=0;
            if (writable) memcpy(&previous,image+symbol->address,4);
            if (!writable || previous!=1920) {
                puts("CE2: render-width export differs from the supported configuration");
                return;
            }
            const uint32_t width=(uint32_t)requested;
            memcpy(image+symbol->address,&width,4);
            printf("CE2: engine render size %ux%u (module data export)\n",width,width*9/16);
            configure_ce2_effects_720(image,m,width);
            return;
        }
    }
    puts("CE2: render-width export not found; keeping engine defaults");
}

/* Unity manages its own frame clock and render targets. The Tempest/Resogun address
 * patches do not apply to this executable. Keep the common renderer entry points inert. */
void runtime_game_set_image(unsigned char *image) { (void)image; }
void runtime_game_frame_rate(void) {}
int runtime_game_widen(unsigned char *image, uint32_t width_4k) {
    (void)image;
    if (width_4k!=3840) puts("Display: Unity native aspect ratio; custom engine resizing is unavailable");
    return width_4k==3840;
}
uint32_t runtime_game_wide_width(const char *name, uint32_t width, uint32_t height, uint32_t width_4k) {
    (void)name; (void)height; (void)width_4k;
    return width;
}
