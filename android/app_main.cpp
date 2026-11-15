/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <SDL3/SDL.h>
#include <adrenotools/driver.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vulkan/vulkan.h>

int tempest_android_boot(const char *image, const char *app0, const char *user);
namespace {
PFN_vkGetInstanceProcAddr vulkan_proc;
void launch_settings(const std::string& directory) {
    FILE* file=fopen((directory+"/launch.ini").c_str(),"r");
    if (!file) return;
    char line[4096];
    while (fgets(line,sizeof(line),file)) {
        char* equals=strchr(line,'=');
        if (!equals || strncmp(line,"TEMPEST_",8)) continue;
        *equals++=0;
        bool valid=true;
        for (const char* p=line; *p; ++p)
            if (!(*p=='_' || (*p>='A' && *p<='Z') || (*p>='0' && *p<='9'))) valid=false;
        if (!valid) continue;
        equals[strcspn(equals,"\r\n")]=0;
        setenv(line,equals,1);
    }
    fclose(file);
}
std::string libraries(JNIEnv *env, jobject activity) {
    jclass type=env->GetObjectClass(activity);
    jmethodID method=env->GetMethodID(type,"getApplicationInfo","()Landroid/content/pm/ApplicationInfo;");
    jobject info=env->CallObjectMethod(activity,method);
    jclass info_type=env->GetObjectClass(info);
    jfieldID field=env->GetFieldID(info_type,"nativeLibraryDir","Ljava/lang/String;");
    auto value=static_cast<jstring>(env->GetObjectField(info,field));
    const char *text=env->GetStringUTFChars(value,nullptr);
    std::string result(text);
    env->ReleaseStringUTFChars(value,text);
    env->DeleteLocalRef(value); env->DeleteLocalRef(info_type);
    env->DeleteLocalRef(info); env->DeleteLocalRef(type);
    return result;
}
void driver() {
    auto *env=static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
    auto activity=static_cast<jobject>(SDL_GetAndroidActivity());
    if (!env || !activity) throw std::runtime_error("Android activity unavailable");
    jclass type=env->GetObjectClass(activity);
    jmethodID get_assets=env->GetMethodID(type,"getAssets","()Landroid/content/res/AssetManager;");
    jobject java_assets=env->CallObjectMethod(activity,get_assets);
    AAssetManager *assets=AAssetManager_fromJava(env,java_assets);
    AAsset *asset=AAssetManager_open(assets,"libvulkan_freedreno.so",AASSET_MODE_STREAMING);
    if (!asset) throw std::runtime_error("missing packaged Turnip driver");
    const std::string directory=std::string(SDL_GetAndroidInternalStoragePath())+"/";
    FILE *file=fopen((directory+"libvulkan_freedreno.so").c_str(),"wb");
    if (!file) { AAsset_close(asset); throw std::runtime_error("cannot stage Turnip driver"); }
    char buffer[65536]; int count;
    bool okay=true;
    while ((count=AAsset_read(asset,buffer,sizeof(buffer)))>0)
        if (fwrite(buffer,1,count,file)!=static_cast<size_t>(count)) { okay=false; break; }
    AAsset_close(asset);
    if (fclose(file) || count<0 || !okay) throw std::runtime_error("Turnip driver copy failed");
    const auto native_libraries=libraries(env,activity);
    void *loader=adrenotools_open_libvulkan(RTLD_NOW|RTLD_LOCAL,ADRENOTOOLS_DRIVER_CUSTOM,
        directory.c_str(),native_libraries.c_str(),directory.c_str(),"libvulkan_freedreno.so",nullptr,nullptr);
    if (!loader) throw std::runtime_error("Turnip Vulkan loader failed");
    vulkan_proc=reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(loader,"vkGetInstanceProcAddr"));
    if (!vulkan_proc) throw std::runtime_error("Turnip vkGetInstanceProcAddr missing");
    env->DeleteLocalRef(java_assets); env->DeleteLocalRef(type); env->DeleteLocalRef(activity);
    puts("Android: rootless Turnip driver ready");
}
}
extern "C" PFN_vkGetInstanceProcAddr tempest_android_vulkan_proc() { return vulkan_proc; }
extern "C" SDL_DECLSPEC int SDL_main(int, char **) {
    int result=1;
    try {
        const char *external=SDL_GetAndroidExternalStoragePath();
        if (!external) throw std::runtime_error("Android game directory unavailable");
        std::string directory(external);
        auto *log=fopen((directory+"/pacman-ce2.log").c_str(),"w");
        if (!log) throw std::runtime_error("cannot open Android game log");
        if (dup2(fileno(log),STDOUT_FILENO)<0 || dup2(fileno(log),STDERR_FILENO)<0)
            throw std::runtime_error("cannot redirect Android game log");
        fclose(log);
        setvbuf(stdout,nullptr,_IOLBF,0); setvbuf(stderr,nullptr,_IOLBF,0);
        puts("PAC-MAN CE2 Android experimental startup");
        launch_settings(directory);
        // Optional per-app Turnip tuning file; the driver can switch rendering
        // modes at runtime for controlled comparisons without restarting CE2.
        if (const char* flags_file=getenv("TEMPEST_TURNIP_DEBUG_FILE"); flags_file && *flags_file) {
            setenv("TU_DEBUG_FILE",flags_file,1);
            printf("Android: Turnip tuning file %s\n",flags_file);
        }
        std::filesystem::create_directories(directory+"/user");
        setenv("TEMPEST_SKIP_INTRO","1",0);
        setenv("TEMPEST_RENDER_RESOLUTION","1080p",0);
        // Retroid's measured 720p profile; launch.ini can override each setting.
        setenv("TEMPEST_CE2_RENDER_WIDTH","1280",0);
        setenv("TEMPEST_INTERNAL_WIDTH","1280",0);
        setenv("TEMPEST_INTERNAL_HEIGHT","720",0);
        setenv("TEMPEST_FEX_MAX_INST","500",0);
        setenv("TEMPEST_CE2_EFFECTS_720","1",0);
        setenv("TEMPEST_CE2_REDUCED_BACKGROUND","0",0);
        setenv("TEMPEST_CE2_NATIVE_COPY","1",0);
        // The bundled Turnip build supports nonblocking completion polls.
        setenv("TEMPEST_VK_DEFER_COUNTER_REFRESH","1",0);
        // Retroid uses single-sampled host images while preserving guest memory layouts.
        setenv("TEMPEST_NO_MSAA","1",0);
        // Restored Android pipelines can produce black frames; compile on demand for now.
        setenv("TEMPEST_PIPELINE_CACHE","0",0);
        setenv("TEMPEST_DISK_SHADERS","0",0);
        setenv("TEMPEST_DISPLAY_MODE","borderless",0);
        setenv("TEMPEST_OUTPUT_RESOLUTION","1280x720",0);
        setenv("TEMPEST_GPU_USER_DIR",(directory+"/user").c_str(),1);
        driver();
        result=tempest_android_boot((directory+"/boot-linked.bin").c_str(),
            (directory+"/app0").c_str(),(directory+"/user").c_str());
    } catch (const std::exception &e) {
        fprintf(stderr,"STOP: Android: %s\n",e.what());
        __android_log_print(ANDROID_LOG_ERROR,"PacmanCE2","%s",e.what());
    }
    fflush(nullptr);
    _exit(result);
}
