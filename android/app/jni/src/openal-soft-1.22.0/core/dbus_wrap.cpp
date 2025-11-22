
#include "config.h"

#include "dbus_wrap.h"

#ifdef HAVE_DYNLOAD

#include <mutex>
#include <type_traits>

#include "logging.h"


void *dbus_handle{nullptr};
#define DECL_FUNC(x) decltype(p##x) p##x{};
DBUS_FUNCTIONS(DECL_FUNC)
#undef DECL_FUNC

void PrepareDBus()
{
    static constexpr char libname[] = "libdbus-1.so.3";

    auto load_func = [](auto &f, const char *name) -> void
    { f = reinterpret_cast<std::remove_reference_t<decltype(f)>>(GetSymbol(dbus_handle, name)); };
#define LOAD_FUNC(x) do {                         \
    load_func(p##x, #x);                          \
    if(!p##x)                                     \
    {                                             \
        WARN("Failed to load function %s\n", #x); \
        CloseLib(dbus_handle);                    \
        dbus_handle = nullptr;                    \
        return;                                   \
    }                                             \
} while(0);

    dbus_handle = LoadLib(libname);
    if(!dbus_handle)
    {
        WARN("Failed to load %s\n", libname);
        return;
    }

DBUS_FUNCTIONS(LOAD_FUNC)
#undef LOAD_FUNC
}
#endif
