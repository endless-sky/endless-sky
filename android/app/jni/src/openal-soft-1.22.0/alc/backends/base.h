#ifndef ALC_BACKENDS_BASE_H
#define ALC_BACKENDS_BASE_H

#include <chrono>
#include <cstdarg>
#include <memory>
#include <ratio>
#include <string>

#include "albyte.h"
#include "core/device.h"
#include "core/except.h"


using uint = unsigned int;

struct ClockLatency {
    std::chrono::nanoseconds ClockTime;
    std::chrono::nanoseconds Latency;
};

struct BackendBase {
    virtual void open(const char *name) = 0;

    virtual bool reset();
    virtual void start() = 0;
    virtual void stop() = 0;

    virtual void captureSamples(al::byte *buffer, uint samples);
    virtual uint availableSamples();

    virtual ClockLatency getClockLatency();

    DeviceBase *const mDevice;

    BackendBase(DeviceBase *device) noexcept : mDevice{device} { }
    virtual ~BackendBase() = default;

protected:
    /** Sets the default channel order used by most non-WaveFormatEx-based APIs. */
    void setDefaultChannelOrder();
    /** Sets the default channel order used by WaveFormatEx. */
    void setDefaultWFXChannelOrder();

#ifdef _WIN32
    /** Sets the channel order given the WaveFormatEx mask. */
    void setChannelOrderFromWFXMask(uint chanmask);
#endif
};
using BackendPtr = std::unique_ptr<BackendBase>;

enum class BackendType {
    Playback,
    Capture
};


/* Helper to get the current clock time from the device's ClockBase, and
 * SamplesDone converted from the sample rate.
 */
inline std::chrono::nanoseconds GetDeviceClockTime(DeviceBase *device)
{
    using std::chrono::seconds;
    using std::chrono::nanoseconds;

    auto ns = nanoseconds{seconds{device->SamplesDone}} / device->Frequency;
    return device->ClockBase + ns;
}

/* Helper to get the device latency from the backend, including any fixed
 * latency from post-processing.
 */
inline ClockLatency GetClockLatency(DeviceBase *device, BackendBase *backend)
{
    ClockLatency ret{backend->getClockLatency()};
    ret.Latency += device->FixedLatency;
    return ret;
}


struct BackendFactory {
    virtual bool init() = 0;

    virtual bool querySupport(BackendType type) = 0;

    virtual std::string probe(BackendType type) = 0;

    virtual BackendPtr createBackend(DeviceBase *device, BackendType type) = 0;

protected:
    virtual ~BackendFactory() = default;
};

namespace al {

enum class backend_error {
    NoDevice,
    DeviceError,
    OutOfMemory
};

class backend_exception final : public base_exception {
    backend_error mErrorCode;

public:
#ifdef __USE_MINGW_ANSI_STDIO
    [[gnu::format(gnu_printf, 3, 4)]]
#else
    [[gnu::format(printf, 3, 4)]]
#endif
    backend_exception(backend_error code, const char *msg, ...) : mErrorCode{code}
    {
        std::va_list args;
        va_start(args, msg);
        setMessage(msg, args);
        va_end(args);
    }
    backend_error errorCode() const noexcept { return mErrorCode; }
};

} // namespace al

#endif /* ALC_BACKENDS_BASE_H */
