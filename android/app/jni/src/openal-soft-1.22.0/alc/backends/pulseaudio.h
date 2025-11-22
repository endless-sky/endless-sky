#ifndef BACKENDS_PULSEAUDIO_H
#define BACKENDS_PULSEAUDIO_H

#include "base.h"

class PulseBackendFactory final : public BackendFactory {
public:
    bool init() override;

    bool querySupport(BackendType type) override;

    std::string probe(BackendType type) override;

    BackendPtr createBackend(DeviceBase *device, BackendType type) override;

    static BackendFactory &getFactory();
};

#endif /* BACKENDS_PULSEAUDIO_H */
