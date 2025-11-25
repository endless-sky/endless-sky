#ifndef CORE_EVENT_H
#define CORE_EVENT_H

#include "almalloc.h"

struct EffectState;

using uint = unsigned int;


struct AsyncEvent {
    enum : uint {
        /* End event thread processing. */
        KillThread = 0,

        /* User event types. */
        SourceStateChange = 1<<0,
        BufferCompleted   = 1<<1,
        Disconnected      = 1<<2,

        /* Internal events. */
        ReleaseEffectState = 65536,
    };

    enum class SrcState {
        Reset,
        Stop,
        Play,
        Pause
    };

    uint EnumType{0u};
    union {
        char dummy;
        struct {
            uint id;
            SrcState state;
        } srcstate;
        struct {
            uint id;
            uint count;
        } bufcomp;
        struct {
            char msg[244];
        } disconnect;
        EffectState *mEffectState;
    } u{};

    AsyncEvent() noexcept = default;
    constexpr AsyncEvent(uint type) noexcept : EnumType{type} { }

    DISABLE_ALLOC()
};

#endif
