#ifndef BACKENDS_SOLARIS_H
#define BACKENDS_SOLARIS_H

#include "base.h"

struct SolarisBackendFactory final : public BackendFactory {
public:
    bool init() override;

    bool querySupport(BackendType type) override;

    std::string probe(BackendType type) override;

    BackendPtr createBackend(DeviceBase *device, BackendType type) override;

    static BackendFactory &getFactory();
};

#endif /* BACKENDS_SOLARIS_H */
