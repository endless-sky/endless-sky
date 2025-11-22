#ifndef BACKENDS_SNDIO_H
#define BACKENDS_SNDIO_H

#include "base.h"

struct SndIOBackendFactory final : public BackendFactory {
public:
    bool init() override;

    bool querySupport(BackendType type) override;

    std::string probe(BackendType type) override;

    BackendPtr createBackend(DeviceBase *device, BackendType type) override;

    static BackendFactory &getFactory();
};

#endif /* BACKENDS_SNDIO_H */
