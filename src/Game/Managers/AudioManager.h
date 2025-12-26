//-----------------------------------------------------------------------------
// AudioManager.h
// Audio management
//-----------------------------------------------------------------------------
#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "App.h"
#include <vector>
#include <string>
#include <algorithm>

class AudioManager {
private:
    struct Sound {
        std::string filename;
        bool isLooping;
    };
    std::vector<Sound> activeSounds;

public:
    void PlaySound(const char* filename, bool loop = false) {
        App::PlayAudio(filename, loop);
        activeSounds.push_back({ filename, loop });
    }

    void StopSound(const char* filename) {
        App::StopAudio(filename);
        activeSounds.erase(
            std::remove_if(activeSounds.begin(), activeSounds.end(),
                [filename](const Sound& s) { return s.filename == filename; }),
            activeSounds.end()
        );
    }

    void StopAllSounds() {
        for (const auto& sound : activeSounds) {
            App::StopAudio(sound.filename.c_str());
        }
        activeSounds.clear();
    }

    bool IsPlaying(const char* filename) const {
        return App::IsSoundPlaying(filename);
    }
};

#endif // AUDIO_MANAGER_H
