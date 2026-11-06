#include <jni.h>

#include <array>
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

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetPreferredSampleRate(
        JNIEnv * /*env*/, jobject /*thiz*/, jint sampleRate) {
    engine().setPreferredSampleRate(sampleRate);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativePreferredSampleRate(
        JNIEnv * /*env*/, jobject /*thiz*/) {
    return engine().preferredSampleRate();
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

// ---- Pengelola urutan efek (BAB 12) ----

extern "C" JNIEXPORT jint JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeEffectCount(JNIEnv * /*env*/,
                                                                     jobject /*thiz*/) {
    return alto::EffectChain::kEffectCount;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeEffectOrderAt(JNIEnv * /*env*/,
                                                                      jobject /*thiz*/,
                                                                      jint slot) {
    const auto order = engine().effects().order();
    if (slot < 0 || slot >= alto::EffectChain::kEffectCount) {
        return -1;
    }
    return static_cast<jint>(order[static_cast<size_t>(slot)]);
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeMoveEffect(JNIEnv * /*env*/,
                                                                   jobject /*thiz*/,
                                                                   jint type,
                                                                   jint delta) {
    engine().effects().moveEffect(static_cast<alto::EffectType>(type), delta);
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeResetEffectOrder(JNIEnv * /*env*/,
                                                                         jobject /*thiz*/) {
    engine().effects().resetOrder();
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetEffectOrder(JNIEnv *env,
                                                                       jobject /*thiz*/,
                                                                       jintArray order) {
    if (order == nullptr) {
        return;
    }
    const jsize length = env->GetArrayLength(order);
    jint values[alto::EffectChain::kEffectCount];
    const jsize count = (length < alto::EffectChain::kEffectCount)
                                ? length
                                : alto::EffectChain::kEffectCount;
    env->GetIntArrayRegion(order, 0, count, values);

    std::array<alto::EffectType, alto::EffectChain::kEffectCount> target{};
    for (int i = 0; i < alto::EffectChain::kEffectCount; ++i) {
        target[static_cast<size_t>(i)] = static_cast<alto::EffectType>(i);
    }
    // Hanya terapkan bila larik memuat permutasi lengkap yang valid.
    bool seen[alto::EffectChain::kEffectCount] = {};
    for (int i = 0; i < count; ++i) {
        const int value = values[i];
        if (value < 0 || value >= alto::EffectChain::kEffectCount || seen[value]) {
            return;
        }
        seen[value] = true;
        target[static_cast<size_t>(i)] = static_cast<alto::EffectType>(value);
    }
    if (count != alto::EffectChain::kEffectCount) {
        return;
    }
    engine().effects().setOrder(target);
}

// ---- Tuner / deteksi pitch (BAB 14) ----

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetTunerActive(JNIEnv * /*env*/,
                                                                       jobject /*thiz*/,
                                                                       jboolean active) {
    engine().pitch().setActive(active == JNI_TRUE);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeTunerFrequency(JNIEnv * /*env*/,
                                                                       jobject /*thiz*/) {
    return engine().pitch().frequency();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeTunerMidiNote(JNIEnv * /*env*/,
                                                                      jobject /*thiz*/) {
    return engine().pitch().midiNote();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeTunerCents(JNIEnv * /*env*/,
                                                                   jobject /*thiz*/) {
    return engine().pitch().cents();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeTunerConfidence(JNIEnv * /*env*/,
                                                                        jobject /*thiz*/) {
    return engine().pitch().confidence();
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetTunerReference(JNIEnv * /*env*/,
                                                                          jobject /*thiz*/,
                                                                          jfloat referenceHz) {
    engine().pitch().setReferenceHz(referenceHz);
}

// ---- Pengaturan audio, gain, limiter, error (BAB 15) ----

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetInputGainDb(JNIEnv * /*env*/,
                                                                       jobject /*thiz*/,
                                                                       jfloat db) {
    engine().inputGain().setTargetDb(db);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeInputGainDb(JNIEnv * /*env*/,
                                                                     jobject /*thiz*/) {
    return engine().inputGain().targetDb();
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetOutputGainDb(JNIEnv * /*env*/,
                                                                        jobject /*thiz*/,
                                                                        jfloat db) {
    engine().outputGain().setTargetDb(db);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeOutputGainDb(JNIEnv * /*env*/,
                                                                      jobject /*thiz*/) {
    return engine().outputGain().targetDb();
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetLimiterCeilingDb(JNIEnv * /*env*/,
                                                                            jobject /*thiz*/,
                                                                            jfloat db) {
    engine().setLimiterCeilingDb(db);
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeLimiterCeilingDb(JNIEnv * /*env*/,
                                                                         jobject /*thiz*/) {
    return engine().limiterCeilingDb();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeInputClipping(JNIEnv * /*env*/,
                                                                      jobject /*thiz*/) {
    return engine().inputClipping() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeOutputClipping(JNIEnv * /*env*/,
                                                                       jobject /*thiz*/) {
    return engine().outputClipping() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeLastError(JNIEnv *env,
                                                                  jobject /*thiz*/) {
    return stdToJstring(env, engine().lastError());
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeClearLastError(JNIEnv * /*env*/,
                                                                       jobject /*thiz*/) {
    engine().clearLastError();
}

// ---- Performa & latensi (BAB 16) ----

extern "C" JNIEXPORT jdouble JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeAverageCallbackMicros(JNIEnv * /*env*/,
                                                                              jobject /*thiz*/) {
    return engine().performance().averageMicros();
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativePeakCallbackMicros(JNIEnv * /*env*/,
                                                                           jobject /*thiz*/) {
    return engine().performance().peakMicros();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeAverageCpuLoad(JNIEnv * /*env*/,
                                                                       jobject /*thiz*/) {
    return engine().performance().averageLoad();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativePeakCpuLoad(JNIEnv * /*env*/,
                                                                    jobject /*thiz*/) {
    return engine().performance().peakLoad();
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeEstimatedLatencyMs(JNIEnv * /*env*/,
                                                                           jobject /*thiz*/) {
    return engine().estimatedLatencyMs();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeBufferCapacityFrames(JNIEnv * /*env*/,
                                                                             jobject /*thiz*/) {
    return engine().bufferCapacityFrames();
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeResetPerformance(JNIEnv * /*env*/,
                                                                         jobject /*thiz*/) {
    engine().performance().reset();
}

// ---- Reset efek (BAB 7) ----

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeResetEffects(JNIEnv * /*env*/,
                                                                     jobject /*thiz*/) {
    engine().effects().reset();
}

// ---- Ducking audio focus (BAB 15) ----

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetDucked(JNIEnv * /*env*/,
                                                                  jobject /*thiz*/,
                                                                  jboolean ducked) {
    engine().setDucked(ducked == JNI_TRUE);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeDucked(JNIEnv * /*env*/,
                                                               jobject /*thiz*/) {
    return engine().ducked() ? JNI_TRUE : JNI_FALSE;
}

// ---- Mute & bypass keseluruhan (blueprint: prinsip interaksi) ----

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetMuted(JNIEnv * /*env*/,
                                                                 jobject /*thiz*/,
                                                                 jboolean muted) {
    engine().outputGain().setMuted(muted == JNI_TRUE);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeMuted(JNIEnv * /*env*/,
                                                              jobject /*thiz*/) {
    return engine().outputGain().muted() ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeSetGlobalBypass(JNIEnv * /*env*/,
                                                                        jobject /*thiz*/,
                                                                        jboolean bypass) {
    engine().effects().setGlobalBypass(bypass == JNI_TRUE);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_altomedia_altoguitarfx_audio_NativeBridge_nativeGlobalBypass(JNIEnv * /*env*/,
                                                                     jobject /*thiz*/) {
    return engine().effects().globalBypass() ? JNI_TRUE : JNI_FALSE;
}
