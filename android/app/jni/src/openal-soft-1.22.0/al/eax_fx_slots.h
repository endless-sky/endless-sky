#ifndef EAX_FX_SLOTS_INCLUDED
#define EAX_FX_SLOTS_INCLUDED


#include <array>

#include "al/auxeffectslot.h"

#include "eax_api.h"

#include "eax_fx_slot_index.h"


class EaxFxSlots
{
public:
    void initialize(
        ALCcontext& al_context);

    void uninitialize() noexcept;

    void commit()
    {
        for(auto& fx_slot : fx_slots_)
            fx_slot->eax_commit();
    }


    const ALeffectslot& get(
        EaxFxSlotIndex index) const;

    ALeffectslot& get(
        EaxFxSlotIndex index);

    void unlock_legacy() noexcept;


private:
    using Items = std::array<EaxAlEffectSlotUPtr, EAX_MAX_FXSLOTS>;


    Items fx_slots_{};


    [[noreturn]]
    static void fail(
        const char* message);

    void initialize_fx_slots(
        ALCcontext& al_context);
}; // EaxFxSlots


#endif // !EAX_FX_SLOTS_INCLUDED
