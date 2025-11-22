#ifndef CORE_FPU_CTRL_H
#define CORE_FPU_CTRL_H

class FPUCtl {
#if defined(HAVE_SSE_INTRINSICS) || (defined(__GNUC__) && defined(HAVE_SSE))
    unsigned int sse_state{};
#endif
    bool in_mode{};

public:
    FPUCtl() noexcept { enter(); in_mode = true; }
    ~FPUCtl() { if(in_mode) leave(); }

    FPUCtl(const FPUCtl&) = delete;
    FPUCtl& operator=(const FPUCtl&) = delete;

    void enter() noexcept;
    void leave() noexcept;
};

#endif /* CORE_FPU_CTRL_H */
