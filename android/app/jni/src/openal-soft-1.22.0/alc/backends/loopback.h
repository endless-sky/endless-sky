#ifndef BACKENDS_LOOPBACK_H
#define BACKENDS_LOOPBACK_H

#include "base.h"

struct LoopbackBackendFactory final : public BackendFactory {
public:
    bool init() override;

    bool querySupport(BackendType type) override;

    std::string probe(BackendType type) override;

    BackendPtr createBackend(DeviceBase *device, BackendType type) override;

    static BackendFactory &getFactory();
};

#endif /* BACKENDS_LOOPBACK_H */
