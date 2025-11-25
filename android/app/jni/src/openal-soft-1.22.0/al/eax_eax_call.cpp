#include "config.h"

#include "al/eax_eax_call.h"

#include "al/eax_exception.h"


namespace {

constexpr auto deferred_flag = 0x80000000U;

class EaxEaxCallException :
    public EaxException
{
public:
    explicit EaxEaxCallException(
        const char* message)
        :
        EaxException{"EAX_EAX_CALL", message}
    {
    }
}; // EaxEaxCallException

} // namespace


EaxEaxCall::EaxEaxCall(
    bool is_get,
    const GUID& property_set_guid,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_buffer,
    ALuint property_size)
    : is_get_{is_get}, version_{0}, property_set_id_{EaxEaxCallPropertySetId::none}
    , property_id_{property_id & ~deferred_flag}, property_source_id_{property_source_id}
    , property_buffer_{property_buffer}, property_size_{property_size}
{
    if (false)
    {
    }
    else if (property_set_guid == EAXPROPERTYID_EAX40_Context)
    {
        version_ = 4;
        property_set_id_ = EaxEaxCallPropertySetId::context;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX50_Context)
    {
        version_ = 5;
        property_set_id_ = EaxEaxCallPropertySetId::context;
    }
    else if (property_set_guid == DSPROPSETID_EAX20_ListenerProperties)
    {
        version_ = 2;
        fx_slot_index_ = 0u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot_effect;
        property_id_ = convert_eax_v2_0_listener_property_id(property_id_);
    }
    else if (property_set_guid == DSPROPSETID_EAX30_ListenerProperties)
    {
        version_ = 3;
        fx_slot_index_ = 0u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot_effect;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX40_FXSlot0)
    {
        version_ = 4;
        fx_slot_index_ = 0u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX50_FXSlot0)
    {
        version_ = 5;
        fx_slot_index_ = 0u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX40_FXSlot1)
    {
        version_ = 4;
        fx_slot_index_ = 1u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX50_FXSlot1)
    {
        version_ = 5;
        fx_slot_index_ = 1u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX40_FXSlot2)
    {
        version_ = 4;
        fx_slot_index_ = 2u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX50_FXSlot2)
    {
        version_ = 5;
        fx_slot_index_ = 2u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX40_FXSlot3)
    {
        version_ = 4;
        fx_slot_index_ = 3u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX50_FXSlot3)
    {
        version_ = 5;
        fx_slot_index_ = 3u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot;
    }
    else if (property_set_guid == DSPROPSETID_EAX20_BufferProperties)
    {
        version_ = 2;
        property_set_id_ = EaxEaxCallPropertySetId::source;
        property_id_ = convert_eax_v2_0_buffer_property_id(property_id_);
    }
    else if (property_set_guid == DSPROPSETID_EAX30_BufferProperties)
    {
        version_ = 3;
        property_set_id_ = EaxEaxCallPropertySetId::source;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX40_Source)
    {
        version_ = 4;
        property_set_id_ = EaxEaxCallPropertySetId::source;
    }
    else if (property_set_guid == EAXPROPERTYID_EAX50_Source)
    {
        version_ = 5;
        property_set_id_ = EaxEaxCallPropertySetId::source;
    }
    else if (property_set_guid == DSPROPSETID_EAX_ReverbProperties)
    {
        version_ = 1;
        fx_slot_index_ = 0u;
        property_set_id_ = EaxEaxCallPropertySetId::fx_slot_effect;
    }
    else if (property_set_guid == DSPROPSETID_EAXBUFFER_ReverbProperties)
    {
        version_ = 1;
        property_set_id_ = EaxEaxCallPropertySetId::source;
    }
    else
    {
        fail("Unsupported property set id.");
    }

    if (version_ < 1 || version_ > 5)
    {
        fail("EAX version out of range.");
    }

    if(!(property_id&deferred_flag))
    {
        if(property_set_id_ != EaxEaxCallPropertySetId::fx_slot && property_id_ != 0)
        {
            if (!property_buffer)
            {
                fail("Null property buffer.");
            }

            if (property_size == 0)
            {
                fail("Empty property.");
            }
        }
    }

    if(property_set_id_ == EaxEaxCallPropertySetId::source && property_source_id_ == 0)
    {
        fail("Null AL source id.");
    }

    if (property_set_id_ == EaxEaxCallPropertySetId::fx_slot)
    {
        if (property_id_ < EAXFXSLOT_NONE)
        {
            property_set_id_ = EaxEaxCallPropertySetId::fx_slot_effect;
        }
    }
}

[[noreturn]]
void EaxEaxCall::fail(
    const char* message)
{
    throw EaxEaxCallException{message};
}

ALuint EaxEaxCall::convert_eax_v2_0_listener_property_id(
    ALuint property_id)
{
    switch (property_id)
    {
        case DSPROPERTY_EAX20LISTENER_NONE:
            return EAXREVERB_NONE;

        case DSPROPERTY_EAX20LISTENER_ALLPARAMETERS:
            return EAXREVERB_ALLPARAMETERS;

        case DSPROPERTY_EAX20LISTENER_ROOM:
            return EAXREVERB_ROOM;

        case DSPROPERTY_EAX20LISTENER_ROOMHF:
            return EAXREVERB_ROOMHF;

        case DSPROPERTY_EAX20LISTENER_ROOMROLLOFFFACTOR:
            return EAXREVERB_ROOMROLLOFFFACTOR;

        case DSPROPERTY_EAX20LISTENER_DECAYTIME:
            return EAXREVERB_DECAYTIME;

        case DSPROPERTY_EAX20LISTENER_DECAYHFRATIO:
            return EAXREVERB_DECAYHFRATIO;

        case DSPROPERTY_EAX20LISTENER_REFLECTIONS:
            return EAXREVERB_REFLECTIONS;

        case DSPROPERTY_EAX20LISTENER_REFLECTIONSDELAY:
            return EAXREVERB_REFLECTIONSDELAY;

        case DSPROPERTY_EAX20LISTENER_REVERB:
            return EAXREVERB_REVERB;

        case DSPROPERTY_EAX20LISTENER_REVERBDELAY:
            return EAXREVERB_REVERBDELAY;

        case DSPROPERTY_EAX20LISTENER_ENVIRONMENT:
            return EAXREVERB_ENVIRONMENT;

        case DSPROPERTY_EAX20LISTENER_ENVIRONMENTSIZE:
            return EAXREVERB_ENVIRONMENTSIZE;

        case DSPROPERTY_EAX20LISTENER_ENVIRONMENTDIFFUSION:
            return EAXREVERB_ENVIRONMENTDIFFUSION;

        case DSPROPERTY_EAX20LISTENER_AIRABSORPTIONHF:
            return EAXREVERB_AIRABSORPTIONHF;

        case DSPROPERTY_EAX20LISTENER_FLAGS:
            return EAXREVERB_FLAGS;

        default:
            fail("Unsupported EAX 2.0 listener property id.");
    }
}

ALuint EaxEaxCall::convert_eax_v2_0_buffer_property_id(
    ALuint property_id)
{
    switch (property_id)
    {
        case DSPROPERTY_EAX20BUFFER_NONE:
            return EAXSOURCE_NONE;

        case DSPROPERTY_EAX20BUFFER_ALLPARAMETERS:
            return EAXSOURCE_ALLPARAMETERS;

        case DSPROPERTY_EAX20BUFFER_DIRECT:
            return EAXSOURCE_DIRECT;

        case DSPROPERTY_EAX20BUFFER_DIRECTHF:
            return EAXSOURCE_DIRECTHF;

        case DSPROPERTY_EAX20BUFFER_ROOM:
            return EAXSOURCE_ROOM;

        case DSPROPERTY_EAX20BUFFER_ROOMHF:
            return EAXSOURCE_ROOMHF;

        case DSPROPERTY_EAX20BUFFER_ROOMROLLOFFFACTOR:
            return EAXSOURCE_ROOMROLLOFFFACTOR;

        case DSPROPERTY_EAX20BUFFER_OBSTRUCTION:
            return EAXSOURCE_OBSTRUCTION;

        case DSPROPERTY_EAX20BUFFER_OBSTRUCTIONLFRATIO:
            return EAXSOURCE_OBSTRUCTIONLFRATIO;

        case DSPROPERTY_EAX20BUFFER_OCCLUSION:
            return EAXSOURCE_OCCLUSION;

        case DSPROPERTY_EAX20BUFFER_OCCLUSIONLFRATIO:
            return EAXSOURCE_OCCLUSIONLFRATIO;

        case DSPROPERTY_EAX20BUFFER_OCCLUSIONROOMRATIO:
            return EAXSOURCE_OCCLUSIONROOMRATIO;

        case DSPROPERTY_EAX20BUFFER_OUTSIDEVOLUMEHF:
            return EAXSOURCE_OUTSIDEVOLUMEHF;

        case DSPROPERTY_EAX20BUFFER_AIRABSORPTIONFACTOR:
            return EAXSOURCE_AIRABSORPTIONFACTOR;

        case DSPROPERTY_EAX20BUFFER_FLAGS:
            return EAXSOURCE_FLAGS;

        default:
            fail("Unsupported EAX 2.0 buffer property id.");
    }
}


EaxEaxCall create_eax_call(
    bool is_get,
    const GUID* property_set_id,
    ALuint property_id,
    ALuint property_source_id,
    ALvoid* property_buffer,
    ALuint property_size)
{
    if(!property_set_id)
        throw EaxEaxCallException{"Null property set ID."};

    return EaxEaxCall{
        is_get,
        *property_set_id,
        property_id,
        property_source_id,
        property_buffer,
        property_size
    };
}
