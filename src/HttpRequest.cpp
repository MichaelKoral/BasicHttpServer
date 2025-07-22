#include "HttpRequest.h"
#include "Server.h"


static std::string versionPrefix = "HTTP/";

HttpRequest::HttpRequest(std::string& request) {
  parse(request);
}
// sp denotes space
// Request Line: METHOD||sp||RESOURCE||sp||VERSION||CRLF
// Headers: (HEADERKEY||:||sp||VALUE||CRLF)*
// Blank Line: CRLF
// Message: ...
void HttpRequest::parse(std::string& request) {
  valid = false;

  //method
  int index = 0;
  int endIndex = request.find(' ');
  if(endIndex == std::string::npos)
    return;
  auto it = methodMap.find(request.substr(index, endIndex-index));
  if(it == methodMap.end())
    return;
  method = it->second;

  //resource
  index = endIndex+1;
  endIndex = request.find(' ', index);
  if(endIndex == std::string::npos)
    return;
  uri = request.substr(index, endIndex-index);
  
  //majorVersion 
  index = endIndex+1;
  endIndex = request.find('.', index);
  //the '.' index should be at least one larger than the end of the "HTTP/" substr
  if(endIndex == std::string::npos || endIndex-index <= versionPrefix.size())
    return;
  try {
    majorVersion = std::stoi(request.substr(index+versionPrefix.size(), endIndex-index));
  }
  catch(std::exception& e) {
    return;
  }

  //minorVersion 
  index = endIndex+1;
  endIndex = request.find(HTTP::NEW_LINE, index);
  if(endIndex == std::string::npos) 
    return;
  try {
    minorVersion = std::stoi(request.substr(index, endIndex-index));
  }
  catch(std::exception& e) {
    return;
  }
  
  //headers
  int endOfHeadersIndex = request.find(std::string(HTTP::NEW_LINE).append(HTTP::NEW_LINE), index);
  index = endIndex+HTTP::NEW_LINE_SIZE;
  if(endOfHeadersIndex == std::string::npos) return;
  while(index < endOfHeadersIndex) {
    //fields should end with a colon and space before the value     
    endIndex = request.find(':', index);
    if(request[index] == ' ' || endIndex == std::string::npos || endIndex >= endOfHeadersIndex || request[endIndex+1] != ' ')
      return;
    std::string field = request.substr(index, endIndex-index);
    
    //2 for the colon and space
    index = endIndex+2;
    endIndex = request.find(HTTP::NEW_LINE, index);
    std::string value = request.substr(index, endIndex-index);
    
    headers[field] = value;
    index = endIndex+HTTP::NEW_LINE_SIZE;
  }

  
  int messageSize = 0;
  try {
    auto itHeader = headers.find(HTTP::MESSAGE_LENGTH_HEADER);
    if(itHeader != headers.end()) {
      messageSize = std::stoi(itHeader->second); 
    }
  }
  catch(std::exception& e) {
    return;
  }

  //succesfully parsed request
  valid = true;

  index = endOfHeadersIndex+HTTP::NEW_LINE_SIZE;
  if(messageSize != 0) {
    message = request.substr(index);
  }
}



std::string HttpRequest::toString() const {
  if(!valid) {
    return "Invalid Request";
  }

  std::string result{};
  switch(method) {
    case HttpRequest::Method::GET: 
      result.append("GET");
      break;
    case HttpRequest::Method::PUT: 
      result.append("PUT");
      break;
    case HttpRequest::Method::HEAD: 
      result.append("HEAD");
      break;
    case HttpRequest::Method::DELETE: 
      result.append("DELETE");
      break;
    case HttpRequest::Method::POST: 
      result.append("POST");
      break;
    default: 
      result.append("NONE");
  }

  result.append(1, ' ');
  result.append(uri);
  result.append(1, ' ');
  result.append(versionPrefix);
  result.append(std::to_string(majorVersion));
  result.append(1, '.');
  result.append(std::to_string(minorVersion));
  result.append(HTTP::NEW_LINE);

  for(auto it = headers.begin(); it != headers.end(); ++it) {
    result.append(it->first);
    result.append(": ");
    result.append(it->second);
    result.append(HTTP::NEW_LINE);
  }
  result.append(HTTP::NEW_LINE);
  result.append(message);

  return result;
}
