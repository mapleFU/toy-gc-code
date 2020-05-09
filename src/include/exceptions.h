//
// Created by 付旭炜 on 2019/12/29.
//

#ifndef TOY_GC_LEARNING_EXCEPTIONS_H
#define TOY_GC_LEARNING_EXCEPTIONS_H

#include <exception>
#include <string>

#include <fmt/format.h>

class NotImplementedException : public std::logic_error {
  public:
    NotImplementedException(unsigned line, const std::string &file)
        : std::logic_error(fmt::format(
              "[Unimplemented Error] File: {}, Line: {}", file, line)){};
};

#define NotImplemented()                                                       \
    { throw NotImplementedException(__LINE__, __FILE__); }

#endif // TOY_GC_LEARNING_EXCEPTIONS_H
