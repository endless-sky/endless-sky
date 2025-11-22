#include "config.h"

#include "eax_exception.h"

#include <cassert>

#include <string>


EaxException::EaxException(
    const char* context,
    const char* message)
    :
    std::runtime_error{make_message(context, message)}
{
}

std::string EaxException::make_message(
    const char* context,
    const char* message)
{
    const auto context_size = (context ? std::string::traits_type::length(context) : 0);
    const auto has_contex = (context_size > 0);

    const auto message_size = (message ? std::string::traits_type::length(message) : 0);
    const auto has_message = (message_size > 0);

    if (!has_contex && !has_message)
    {
        return std::string{};
    }

    static constexpr char left_prefix[] = "[";
    const auto left_prefix_size = std::string::traits_type::length(left_prefix);

    static constexpr char right_prefix[] = "] ";
    const auto right_prefix_size = std::string::traits_type::length(right_prefix);

    const auto what_size =
        (
            has_contex ?
            left_prefix_size + context_size + right_prefix_size :
            0) +
        message_size +
        1;

    auto what = std::string{};
    what.reserve(what_size);

    if (has_contex)
    {
        what.append(left_prefix, left_prefix_size);
        what.append(context, context_size);
        what.append(right_prefix, right_prefix_size);
    }

    if (has_message)
    {
        what.append(message, message_size);
    }

    return what;
}
