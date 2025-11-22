
#include "config.h"

#include "alstring.h"

#include <cctype>
#include <string>


namespace {

int to_upper(const char ch)
{
    using char8_traits = std::char_traits<char>;
    return std::toupper(char8_traits::to_int_type(ch));
}

} // namespace

namespace al {

int strcasecmp(const char *str0, const char *str1) noexcept
{
    do {
        const int diff{to_upper(*str0) - to_upper(*str1)};
        if(diff < 0) return -1;
        if(diff > 0) return 1;
    } while(*(str0++) && *(str1++));
    return 0;
}

int strncasecmp(const char *str0, const char *str1, std::size_t len) noexcept
{
    if(len > 0)
    {
        do {
            const int diff{to_upper(*str0) - to_upper(*str1)};
            if(diff < 0) return -1;
            if(diff > 0) return 1;
        } while(--len && *(str0++) && *(str1++));
    }
    return 0;
}

} // namespace al
