#ifndef EAX_EXCEPTION_INCLUDED
#define EAX_EXCEPTION_INCLUDED


#include <stdexcept>
#include <string>


class EaxException :
    public std::runtime_error
{
public:
    EaxException(
        const char* context,
        const char* message);


private:
    static std::string make_message(
        const char* context,
        const char* message);
}; // EaxException


#endif // !EAX_EXCEPTION_INCLUDED
