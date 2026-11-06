#ifndef ALTOGUITARFX_AUDIOENGINE_H
#define ALTOGUITARFX_AUDIOENGINE_H

#include <string>

namespace alto {

// Mesin audio native. Pada BAB 3 hanya menyediakan identitas/versi.
// Stream Oboe dan pemrosesan DSP ditambahkan pada BAB 4 dan seterusnya.
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    std::string version() const;
};

}  // namespace alto

#endif  // ALTOGUITARFX_AUDIOENGINE_H
