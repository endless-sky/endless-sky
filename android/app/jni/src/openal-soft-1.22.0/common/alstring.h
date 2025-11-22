#ifndef AL_STRING_H
#define AL_STRING_H

#include <cstddef>


namespace al {

/* These would be better served by using a string_view-like span/view with
 * case-insensitive char traits.
 */
int strcasecmp(const char *str0, const char *str1) noexcept;
int strncasecmp(const char *str0, const char *str1, std::size_t len) noexcept;

} // namespace al

#endif /* AL_STRING_H */
