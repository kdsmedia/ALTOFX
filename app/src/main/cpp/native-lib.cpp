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

extern "C" JNIEXPORT jstring JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeStart(JNIEnv *env, jobject /*thiz*/) {
    return stdToJstring(env, engine().startAudio());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeStop(JNIEnv *env, jobject /*thiz*/) {
    return stdToJstring(env, engine().stopAudio());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeIsRunning(JNIEnv * /*env*/,
                                                                  jobject /*thiz*/) {
    return engine().isRunning() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSampleRate(JNIEnv * /*env*/,
                                                                    jobject /*thiz*/) {
    return engine().sampleRate();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeFramesPerBurst(JNIEnv * /*env*/,
                                                                        jobject /*thiz*/) {
    return engine().framesPerBurst();
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetDeviceIds(JNIEnv * /*env*/,
                                                                     jobject /*thiz*/,
                                                                     jint inputDeviceId,
                                                                     jint outputDeviceId) {
    engine().setDeviceIds(inputDeviceId, outputDeviceId);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeInputLevel(JNIEnv * /*env*/,
                                                                    jobject /*thiz*/) {
    return engine().inputLevel();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeOutputLevel(JNIEnv * /*env*/,
                                                                     jobject /*thiz*/) {
    return engine().outputLevel();
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeCallbackCount(JNIEnv * /*env*/,
                                                                       jobject /*thiz*/) {
    return engine().callbackCount();
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeFramesProcessed(JNIEnv * /*env*/,
                                                                         jobject /*thiz*/) {
    return engine().framesProcessed();
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeXrunCount(JNIEnv * /*env*/,
                                                                   jobject /*thiz*/) {
    return engine().xrunCount();
}

// ---- Kontrol efek (BAB 7+) ----

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetEffectEnabled(JNIEnv * /*env*/,
                                                                         jobject /*thiz*/,
                                                                         jint type,
                                                                         jboolean enabled) {
    engine().effects().setEnabled(static_cast<alto::EffectType>(type), enabled == JNI_TRUE);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeIsEffectEnabled(JNIEnv * /*env*/,
                                                                         jobject /*thiz*/,
                                                                         jint type) {
    return engine().effects().isEnabled(static_cast<alto::EffectType>(type)) ? JNI_TRUE
                                                                             : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetEffectParam(JNIEnv * /*env*/,
                                                                       jobject /*thiz*/,
                                                                       jint type,
                                                                       jint index,
                                                                       jfloat value) {
    engine().effects().setParam(static_cast<alto::EffectType>(type), index, value);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeGetEffectParam(JNIEnv * /*env*/,
                                                                       jobject /*thiz*/,
                                                                       jint type,
                                                                       jint index) {
    return engine().effects().param(static_cast<alto::EffectType>(type), index);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeEffectParamCount(JNIEnv * /*env*/,
                                                                         jobject /*thiz*/,
                                                                         jint type) {
    return engine().effects().paramCount(static_cast<alto::EffectType>(type));
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeEffectName(JNIEnv *env,
                                                                    jobject /*thiz*/,
                                                                    jint type) {
    return env->NewStringUTF(engine().effects().name(static_cast<alto::EffectType>(type)));
}
