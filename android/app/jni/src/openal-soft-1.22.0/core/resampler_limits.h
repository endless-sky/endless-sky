#ifndef CORE_RESAMPLER_LIMITS_H
#define CORE_RESAMPLER_LIMITS_H

/* Maximum number of samples to pad on the ends of a buffer for resampling.
 * Note that the padding is symmetric (half at the beginning and half at the
 * end)!
 */
constexpr int MaxResamplerPadding{48};

constexpr int MaxResamplerEdge{MaxResamplerPadding >> 1};

#endif /* CORE_RESAMPLER_LIMITS_H */
