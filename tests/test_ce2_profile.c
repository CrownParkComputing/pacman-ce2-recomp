/* CE2 profile selection through the public loader hook, using synthetic mapped
 * module bytes. No game assets or user saves are opened.
 * Standalone: cc -D_GNU_SOURCE [-DTEMPEST_FEX] -I. tests/test_ce2_profile.c
 *               src/runtime_game.c -o /tmp/ce2-profile-test */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#undef NDEBUG
#include "../src/runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

enum {
    MODULE_BASE=0x10000,
    CODE_SIZE=0x200000,
    DATA_SIZE=0x4000,
    IMAGE_SIZE=MODULE_BASE+CODE_SIZE+DATA_SIZE,
    AWAKE=MODULE_BASE+0x1eb680,
    CALLSITE=MODULE_BASE+0x1eb78a,
    WIDTH_EXPORT=MODULE_BASE+CODE_SIZE,
};
enum Scenario {
    SELECT_720, CHANGED_CALL, CHANGED_PROLOGUE, NON_EXECUTABLE,
    DEFAULT_OFF, EXPLICIT_OFF, KEEP_1080, UNRECOGNIZED_WIDTH,
};

static void assert_code_protection(const unsigned char *address,int executable) {
    FILE *maps=fopen("/proc/self/maps","r");
    assert(maps);
    char line[1024],permissions[5];
    unsigned long begin,end;
    int found=0;
    while (fgets(line,sizeof(line),maps)) {
        if (sscanf(line,"%lx-%lx %4s",&begin,&end,permissions)==3 &&
            (uintptr_t)address>=begin && (uintptr_t)address<end) {
            assert(permissions[0]=='r' && permissions[1]=='-');
            assert((permissions[2]=='x')==executable);
            found=1;
            break;
        }
    }
    assert(fclose(maps)==0 && found);
}

static void test_profile(enum Scenario scenario) {
    static const unsigned char prologue[]={
        0x48,0x83,0xec,0x38,0x4c,0x89,0x3c,0x24,0x4c,0x8b,0xff,0x4d,0x85,0xff,
        0x0f,0x84,0x53,0x01,0x00,0x00};
    unsigned char callsite[]={
        0x49,0x8b,0xff,0xbe,0x01,0x00,0x00,0x00,0xe8,0xdd,0xd5,0x4e,0x00};
    unsigned char *image=mmap(NULL,IMAGE_SIZE,PROT_READ|PROT_WRITE,
                              MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(image!=MAP_FAILED);
    memset(image,0xa5,IMAGE_SIZE);
    memcpy(image+AWAKE,prologue,sizeof(prologue));
    if (scenario==CHANGED_CALL) callsite[9]^=1;
    memcpy(image+CALLSITE,callsite,sizeof(callsite));
    if (scenario==CHANGED_PROLOGUE) image[AWAKE]=0x90;
    uint32_t initial_width=scenario==UNRECOGNIZED_WIDTH ? 1024 : 1920;
    memcpy(image+WIDTH_EXPORT,&initial_width,sizeof(initial_width));

    RuntimeModuleExport symbol={.nid="QBbo2kzANmQ",.address=WIDTH_EXPORT};
    RuntimeModule module={.base=MODULE_BASE,.size=CODE_SIZE+DATA_SIZE,
        .name="MonoAssembliesPS4.prx",.exports=&symbol,.export_count=1,.segment_count=2};
    module.segments[0]=(RuntimeModuleSegment){MODULE_BASE,CODE_SIZE,
        scenario==NON_EXECUTABLE ? 4 : 5};
    module.segments[1]=(RuntimeModuleSegment){WIDTH_EXPORT,DATA_SIZE,6};
    int executable=0;
#ifndef TEMPEST_FEX
    executable=scenario!=NON_EXECUTABLE;
#endif
    assert(mprotect(image+MODULE_BASE,CODE_SIZE,PROT_READ|(executable ? PROT_EXEC : 0))==0);
    assert_code_protection(image+CALLSITE,executable);

    assert(setenv("TEMPEST_CE2_RENDER_WIDTH",scenario==KEEP_1080 ? "1920" : "1280",1)==0);
    assert(unsetenv("TEMPEST_CE2_EFFECTS_720")==0);
    if (scenario!=DEFAULT_OFF)
        assert(setenv("TEMPEST_CE2_EFFECTS_720",scenario==EXPLICIT_OFF ? "0" : "1",1)==0);
    unsigned char *expected=malloc(IMAGE_SIZE);
    assert(expected);
    memcpy(expected,image,IMAGE_SIZE);
    const uint32_t expected_width=scenario==UNRECOGNIZED_WIDTH ? initial_width :
                                  scenario==KEEP_1080 ? 1920 : 1280;
    memcpy(expected+WIDTH_EXPORT,&expected_width,sizeof(expected_width));
    if (scenario==SELECT_720) expected[CALLSITE+4]=3;

    runtime_game_configure_modules(image,&module,1);
    /* Every mapped byte is compared: only the recognized width export and the
     * selected preset immediate may change, including on rejected signatures. */
    assert(memcmp(image,expected,IMAGE_SIZE)==0);
    assert_code_protection(image+CALLSITE,executable);
    assert_code_protection(image+AWAKE,executable);
    free(expected);
    assert(munmap(image,IMAGE_SIZE)==0);
}

int main(void) {
    alarm(10);
    for (int scenario=SELECT_720;scenario<=UNRECOGNIZED_WIDTH;++scenario)
        test_profile((enum Scenario)scenario);
    alarm(0);
#ifdef TEMPEST_FEX
    puts("CE2 profile guards and exact writes passed; FEX code pages remain read-only/NX");
#else
    puts("CE2 profile guards and exact writes passed; native code pages remain read-only/RX");
#endif
    return 0;
}
