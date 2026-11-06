#ifndef ALTOGUITARFX_DELAY_H
#define ALTOGUITARFX_DELAY_H

#include "Effect.h"
#include "Modulation.h"

namespace alto {

/**
 * Delay (BAB 11): mengulang sinyal dengan jeda. Parameter: Time, Feedback,
 * Mix. Waktu tunda dihaluskan sehingga perubahan slider tidak menimbulkan
 * klik; pembacaan memakai interpolasi linear.
 */
class Delay : public Effect {
public:
    static constexpr float kMaxDelayMs = 1200.0f;
    static constexpr int32_t kMaxChannels = 2;

    Delay();

    void prepare(double sampleRate, int32_t channels) override;
    void reset() override;
    void process(float* buffer, int32_t frames, int32_t channels) override;

protected:
    void onParamChanged(int index, float value) override;

private:
    SmoothedParam timeMs_;
    SmoothedParam feedback_;
    SmoothedParam mix_;
    DelayLine delay_;
};

}  // namespace alto

#endif  // ALTOGUITARFX_DELAY_H
