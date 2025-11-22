#include "config.h"

#include "eax_utils.h"

#include <cassert>
#include <exception>

#include "core/logging.h"


void eax_log_exception(
    const char* message) noexcept
{
    const auto exception_ptr = std::current_exception();

    assert(exception_ptr);

    if (message)
    {
        ERR("%s\n", message);
    }

    try
    {
        std::rethrow_exception(exception_ptr);
    }
    catch (const std::exception& ex)
    {
        const auto ex_message = ex.what();
        ERR("%s\n", ex_message);
    }
    catch (...)
    {
        ERR("%s\n", "Generic exception.");
    }
}
