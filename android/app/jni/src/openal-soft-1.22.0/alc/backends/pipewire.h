#ifndef BACKENDS_PIPEWIRE_H
#define BACKENDS_PIPEWIRE_H

#include <string>

#include "base.h"

struct DeviceBase;

struct PipeWireBackendFactory final : public BackendFactory {
public:
    bool init() override;

    bool querySupport(BackendType type) override;

    std::string probe(BackendType type) override;

    BackendPtr createBackend(DeviceBase *device, BackendType type) override;

    static BackendFactory &getFactory();
};

#endif /* BACKENDS_PIPEWIRE_H */
