#ifndef EFFECTS_BASE_H
#define EFFECTS_BASE_H

#include "core/effects/base.h"


EffectStateFactory *NullStateFactory_getFactory(void);
EffectStateFactory *ReverbStateFactory_getFactory(void);
EffectStateFactory *StdReverbStateFactory_getFactory(void);
EffectStateFactory *AutowahStateFactory_getFactory(void);
EffectStateFactory *ChorusStateFactory_getFactory(void);
EffectStateFactory *CompressorStateFactory_getFactory(void);
EffectStateFactory *DistortionStateFactory_getFactory(void);
EffectStateFactory *EchoStateFactory_getFactory(void);
EffectStateFactory *EqualizerStateFactory_getFactory(void);
EffectStateFactory *FlangerStateFactory_getFactory(void);
EffectStateFactory *FshifterStateFactory_getFactory(void);
EffectStateFactory *ModulatorStateFactory_getFactory(void);
EffectStateFactory *PshifterStateFactory_getFactory(void);
EffectStateFactory* VmorpherStateFactory_getFactory(void);

EffectStateFactory *DedicatedStateFactory_getFactory(void);

EffectStateFactory *ConvolutionStateFactory_getFactory(void);

#endif /* EFFECTS_BASE_H */
