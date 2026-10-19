#ifndef GODOTFMOD_IOS_PLUGINS_LOADER_H
#define GODOTFMOD_IOS_PLUGINS_LOADER_H

#ifdef IOS_ENABLED
typedef void* FMOD_SYSTEM_PTR;

typedef uint32_t (*REGISTER_DSP_METHOD)(FMOD_SYSTEM_PTR system, FMOD_DSP_DESCRIPTION* description, uint32_t* handle);
typedef uint32_t (*REGISTER_CODEC_METHOD)(FMOD_SYSTEM_PTR system, FMOD_CODEC_DESCRIPTION* description, uint32_t* handle);
typedef uint32_t (*REGISTER_OUTPUT_METHOD)(FMOD_SYSTEM_PTR system, FMOD_OUTPUT_DESCRIPTION* description, uint32_t* handle);

typedef struct {
    FMOD_SYSTEM_PTR system;
    REGISTER_DSP_METHOD register_dsp_method;
    REGISTER_CODEC_METHOD register_codec_method;
    REGISTER_OUTPUT_METHOD register_output_method;
} FMOD_IOS_INTERFACE;

extern "C" {
    // Implemented by the plugin registration code that the iOS export plugin generates.
    // It is declared weak so that the library still links when the exported project does
    // not register any static plugin: the fallback implementation in fmod_server.cpp is
    // then used.
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((weak))
#endif
    uint32_t* load_all_fmod_plugins(FMOD_IOS_INTERFACE* ios_interface, uint32_t* r_count);
};
#endif

#endif //GODOTFMOD_IOS_PLUGINS_LOADER_H
