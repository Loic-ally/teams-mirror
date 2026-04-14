#ifndef PARSER_HPP
#define PARSER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>

namespace client {

class ParsedInput {
  std::stringstream _arguments = {};
  std::string _command = {};
  bool _failArgs = false;

public:
  ParsedInput();

  const std::string &getCommand() const { return _command; };

  bool fail() const { return _failArgs; };
  bool hasRemainingArgs() {
    _arguments >> std::ws;
    return _arguments.peek() != std::char_traits<char>::eof();
  }

  template <typename T> T getArg() {
    if (_failArgs) {
      return T{};
    }

    std::string resNoQuote;
    T res;

    _arguments >> std::ws;

    int nextChar = _arguments.peek();

    if (nextChar != '"') {
        _failArgs = true;
        _arguments.setstate(std::ios::failbit);
        return T{};
    }
    if (!(_arguments >> std::quoted(resNoQuote))) {
      _failArgs = true;
      return T{};
    }

    if (std::is_same<T, std::string>::value) {
        return resNoQuote;
    }

    std::stringstream convertor(resNoQuote);
    if (!(convertor >> res) || !(convertor >> std::ws).eof()) {
      _failArgs = true;
      return T{};
    }
    return res;
  };

  template <typename T> bool operator>>(T &other) {
    other = getArg<T>();
    return !_failArgs;
  }
};

} // namespace client

#endif //PARSER_HPP
