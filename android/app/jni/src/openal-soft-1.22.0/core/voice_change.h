#ifndef VOICE_CHANGE_H
#define VOICE_CHANGE_H

#include <atomic>

#include "almalloc.h"

struct Voice;

using uint = unsigned int;


enum class VChangeState {
    Reset,
    Stop,
    Play,
    Pause,
    Restart
};
struct VoiceChange {
    Voice *mOldVoice{nullptr};
    Voice *mVoice{nullptr};
    uint mSourceID{0};
    VChangeState mState{};

    std::atomic<VoiceChange*> mNext{nullptr};

    DEF_NEWDEL(VoiceChange)
};

#endif /* VOICE_CHANGE_H */
