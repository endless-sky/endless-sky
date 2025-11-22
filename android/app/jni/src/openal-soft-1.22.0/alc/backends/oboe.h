#ifndef BACKENDS_OBOE_H
#define BACKENDS_OBOE_H

#include "base.h"

struct OboeBackendFactory final : public BackendFactory {
public:
    bool init() override;

    bool querySupport(BackendType type) override;

    std::string probe(BackendType type) override;

    BackendPtr createBackend(DeviceBase *device, BackendType type) override;

    static BackendFactory &getFactory();
};

#endif /* BACKENDS_OBOE_H */
