#ifndef CORE_BSINC_DEFS_H
#define CORE_BSINC_DEFS_H

/* The number of distinct scale and phase intervals within the filter table. */
constexpr unsigned int BSincScaleBits{4};
constexpr unsigned int BSincScaleCount{1 << BSincScaleBits};
constexpr unsigned int BSincPhaseBits{5};
constexpr unsigned int BSincPhaseCount{1 << BSincPhaseBits};

#endif /* CORE_BSINC_DEFS_H */
