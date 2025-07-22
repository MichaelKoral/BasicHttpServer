#pragma once

#include <vector>
#include <cassert>

#include "Server.h"

class HttpResponseBuilder {
  public:
    inline HttpResponseBuilder() {
      response.append(HTTP::VERSION).append(" 000");
      statusCodeIndex = response.size()-3;
      response.append(HTTP::NEW_LINE);
      headerIndex = response.size();
      response.append(HTTP::NEW_LINE);
    }
    inline HttpResponseBuilder& addStatus(const std::string& statusCode) {
      assert(statusCode.size() == 3);
      response[statusCodeIndex] = statusCode[0];
      response[statusCodeIndex+1] = statusCode[1];
      response[statusCodeIndex+2] = statusCode[2];
      return *this;
    }
    inline HttpResponseBuilder& addStatus(const std::string&& statusCode) {
      assert(statusCode.size() == 3);
      response[statusCodeIndex] = statusCode[0];
      response[statusCodeIndex+1] = statusCode[1];
      response[statusCodeIndex+2] = statusCode[2];
      return *this;
    }
    inline HttpResponseBuilder& addHeader(const std::string& field, const std::string& value) {
      std::string headerLine;
      headerLine.append(field).append(": ").append(value).append(HTTP::NEW_LINE);
      response.insert(headerIndex, headerLine);
      return *this;
    }
    inline HttpResponseBuilder& addHeader(const std::string&& field, const std::string&& value) {
      std::string headerLine;
      headerLine.append(field).append(": ").append(value).append(HTTP::NEW_LINE);
      response.insert(headerIndex, headerLine);
      return *this;
    }
    inline HttpResponseBuilder& addResource(const std::string& resource) {
      response.append(resource.begin(), resource.end());
      return *this;
    }
    inline std::string create() {
      return response;
    }

  private:
    std::string response;
    size_t statusCodeIndex;
    size_t headerIndex; 
};
