#ifndef TPXL_AUDIO_H
#define TPXL_AUDIO_H

#include "tpxl/type.h"

typedef struct TpxlAudioImp TpxlAudio;

TpxlResult tpxl_open_audio(const char* path, TpxlAudio** audio);
void tpxl_close_audio(TpxlAudio* audio);

#endif
