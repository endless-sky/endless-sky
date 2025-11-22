#ifndef ALU_H
#define ALU_H

#include <bitset>

#include "aloptional.h"

struct ALCcontext;
struct ALCdevice;
struct EffectSlot;

enum class StereoEncoding : unsigned char;


constexpr float GainMixMax{1000.0f}; /* +60dB */


enum CompatFlags : uint8_t {
    ReverseX,
    ReverseY,
    ReverseZ,

    Count
};
using CompatFlagBitset = std::bitset<CompatFlags::Count>;

void aluInit(CompatFlagBitset flags);

/* aluInitRenderer
 *
 * Set up the appropriate panning method and mixing method given the device
 * properties.
 */
void aluInitRenderer(ALCdevice *device, int hrtf_id, al::optional<StereoEncoding> stereomode);

void aluInitEffectPanning(EffectSlot *slot, ALCcontext *context);

#endif
