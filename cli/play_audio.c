#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "tpxl/type.h"
#include "tpxl/audio.h"

#include "cli.h"

int play_audio(const char* path) {

    TpxlResult result = TPXL_OK;
    
    TpxlAudio* audio = NULL;
    result = tpxl_open_audio(path, &audio);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        return EXIT_FAILURE;
    }

    TpxlAudioPlayer* player = NULL;
    result = tpxl_create_audio_player(&player, audio);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_audio(audio);
        return EXIT_FAILURE;
    }

    result = tpxl_play_audio(player);

    if (result != TPXL_OK) {
        printf("Error: %s\n", tpxl_result_to_string(result));
        tpxl_close_audio_player(player);
        tpxl_close_audio(audio);
        return EXIT_FAILURE;
    }

    while (tpxl_audio_playing(player)) {
        sleep(1);
    }

    tpxl_close_audio_player(player);
    tpxl_close_audio(audio);

    return EXIT_SUCCESS;
}
