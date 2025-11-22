#ifndef BACKENDS_PORTAUDIO_H
#define BACKENDS_PORTAUDIO_H

#include "base.h"

struct PortBackendFactory final : public BackendFactory {
public:
    bool init() override;

    bool querySupport(BackendType type) override;

    std::string probe(BackendType type) override;

    BackendPtr createBackend(DeviceBase *device, BackendType type) override;

    static BackendFactory &getFactory();
};

#endif /* BACKENDS_PORTAUDIO_H */
