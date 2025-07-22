#pragma once

#include <string>
#include <unordered_map>

class HttpRequest {
public: 
  enum Method {
    NONE = 0,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
  };

HttpRequest() : method(Method::NONE), uri(""), majorVersion(0), minorVersion(0), headers({}), message(""), valid(false)  {}
HttpRequest(std::string& request);
void parse(std::string& request);
std::string toString() const;
inline Method getMethod() const {
  return method;
}
inline const std::string& getUri() const {
  return uri;
}
inline uint32_t getMajorVersion() const {
  return majorVersion;
}
inline uint32_t getMinorVersion() const {
  return minorVersion;
}
inline const std::unordered_map<std::string, std::string>& getHeaders() const {
  return headers;
}
inline const std::string& getMessage() const {
  return message;
}
inline bool isValid() const {
  return valid;
}

private:
  static inline const std::unordered_map<std::string, Method> methodMap{
    {"GET", Method::GET},
    {"HEAD", Method::HEAD},
    {"POST", Method::POST},
    {"PUT", Method::PUT},
    {"DELETE", Method::DELETE},
  };
  

  Method method;
  std::string uri;
  uint32_t majorVersion;
  uint32_t minorVersion;
  std::unordered_map<std::string, std::string> headers;
  std::string message;
  bool valid;
};
