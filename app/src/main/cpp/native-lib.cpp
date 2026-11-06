#include <jni.h>

#include <string>

#include "AudioEngine.h"

namespace {

// Satu instance engine per proses. Pada BAB 4 instance ini akan memiliki
// stream audio; untuk sekarang hanya menandai bahwa JNI sudah terhubung.
alto::AudioEngine &engine() {
    static alto::AudioEngine instance;
    return instance;
}

std::string jstringToStd(JNIEnv *env, jstring value) {
    if (value == nullptr) {
        return {};
    }
    const char *chars = env->GetStringUTFChars(value, nullptr);
    std::string result = (chars != nullptr) ? chars : "";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(value, chars);
    }
    return result;
}

jstring stdToJstring(JNIEnv *env, const std::string &value) {
    return env->NewStringUTF(value.c_str());
}

}  // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeVersion(JNIEnv *env, jobject /*thiz*/) {
    return stdToJstring(env, engine().version());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeEcho(JNIEnv *env, jobject /*thiz*/,
                                                             jstring input) {
    return stdToJstring(env, jstringToStd(env, input));
}
