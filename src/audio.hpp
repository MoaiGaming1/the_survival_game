#pragma once

#include "audio/miniaudio.h"

#include <iostream>

namespace audio {
	ma_engine audioEngine;
	
	void init() {
		if (ma_engine_init(nullptr, &audioEngine) != MA_SUCCESS) {
			std::cerr << "ERROR INITING AUDIO ENGINE!\n";
		}
	}

	void playAudio(const char* filename) {
		ma_engine_play_sound(&audioEngine, filename, nullptr);
	}
}
