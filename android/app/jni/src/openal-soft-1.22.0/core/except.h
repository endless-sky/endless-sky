#ifndef CORE_EXCEPT_H
#define CORE_EXCEPT_H

#include <cstdarg>
#include <exception>
#include <string>
#include <utility>


namespace al {

class base_exception : public std::exception {
    std::string mMessage;

protected:
    base_exception() = default;
    virtual ~base_exception();

    void setMessage(const char *msg, std::va_list args);

public:
    const char *what() const noexcept override { return mMessage.c_str(); }
};

} // namespace al

#define START_API_FUNC try

#define END_API_FUNC catch(...) { std::terminate(); }

#endif /* CORE_EXCEPT_H */
