#include "config.h"

#include "eax_fx_slot_index.h"

#include "eax_exception.h"


namespace
{


class EaxFxSlotIndexException :
    public EaxException
{
public:
    explicit EaxFxSlotIndexException(
        const char* message)
        :
        EaxException{"EAX_FX_SLOT_INDEX", message}
    {
    }
}; // EaxFxSlotIndexException


} // namespace


void EaxFxSlotIndex::set(EaxFxSlotIndexValue index)
{
    if(index >= EaxFxSlotIndexValue{EAX_MAX_FXSLOTS})
        fail("Index out of range.");

    emplace(index);
}

void EaxFxSlotIndex::set(const GUID &guid)
{
    if (false)
    {
    }
    else if (guid == EAX_NULL_GUID)
    {
        reset();
    }
    else if (guid == EAXPROPERTYID_EAX40_FXSlot0 || guid == EAXPROPERTYID_EAX50_FXSlot0)
    {
        emplace(0u);
    }
    else if (guid == EAXPROPERTYID_EAX40_FXSlot1 || guid == EAXPROPERTYID_EAX50_FXSlot1)
    {
        emplace(1u);
    }
    else if (guid == EAXPROPERTYID_EAX40_FXSlot2 || guid == EAXPROPERTYID_EAX50_FXSlot2)
    {
        emplace(2u);
    }
    else if (guid == EAXPROPERTYID_EAX40_FXSlot3 || guid == EAXPROPERTYID_EAX50_FXSlot3)
    {
        emplace(3u);
    }
    else
    {
        fail("Unsupported GUID.");
    }
}

[[noreturn]]
void EaxFxSlotIndex::fail(const char* message)
{
    throw EaxFxSlotIndexException{message};
}
