#ifndef EXCEPTION_HPP
#define EXCEPTION_HPP

#include <stdexcept>
#include <string>

namespace andreasmusic
{
  class Exception : public std::runtime_error
  {
  public:
    explicit Exception(const std::string& msg):std::runtime_error(msg){};
  };
}

#endif
