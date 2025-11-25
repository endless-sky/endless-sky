#ifndef BACKENDS_OSS_H
#define BACKENDS_OSS_H

#include "base.h"

struct OSSBackendFactory final : public BackendFactory {
public:
    bool init() override;

    bool querySupport(BackendType type) override;

    std::string probe(BackendType type) override;

    BackendPtr createBackend(DeviceBase *device, BackendType type) override;

    static BackendFactory &getFactory();
};

#endif /* BACKENDS_OSS_H */
