#pragma once

#include <string>

#include <string>
#include <array>
#include <unordered_map>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <error.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <thread>


class Server {
  public: 
    static constexpr char const* NAME = "server";
    Server();
    void start();
    void threadExec();

  private:
    struct HttpConnection {
      HttpConnection(int fd) {
        this->fd = fd;
        state = HttpConnection::SocketState::READING;
        data = std::string(MAX_REQUEST_SIZE, 0);
        dataIndex = 0;
      }
      enum class SocketState {
        WRITING,
        READING,
        NONE,
      };
      SocketState state;
      std::string data;
      int32_t dataIndex;
      int fd;
    };
    static constexpr char const* PORT_NUMBER = "80";
    static constexpr uint32_t MAX_EVENTS = 100;
    static constexpr uint32_t BACKLOG = 10;
    static constexpr uint32_t MAX_REQUEST_SIZE = 10e6;
    static constexpr int EPOLL_FLAGS = EPOLLET|EPOLLONESHOT;
    static constexpr char const* RESOURCE_DIR = "resources";
    static constexpr uint32_t NUM_THREADS = 10;

    //file extension to content type header mapping
    static inline std::unordered_map<std::string, std::string> contentType{
      {"html", "text/html"},
      {"jpg", "image/jpg"},
    };
    
    std::array<epoll_event, MAX_EVENTS> events{};
    int epollfd;
    int listenerSocket;
    std::array<std::thread, NUM_THREADS> threads;
    std::unordered_map<std::string, std::string> resourcePaths;

    void getListenerSocket();
    void buildResponse(HttpConnection* connection);
    void sendResponse(HttpConnection* connection);
    void setNonBlocking(int fd);
    void rearmSocket(HttpConnection* connection);
    void addNewConnection(int fd);
    void removeConnection(HttpConnection* connection);
    void readRequest(HttpConnection* connection);
};

namespace HTTP {
  static constexpr char const* NEW_LINE = "\r\n";
  static constexpr char const* VERSION = "HTTP/1.1";
  static constexpr char const* MESSAGE_LENGTH_HEADER = "Content-Length";
  static constexpr char const* MESSAGE_ENCODING_HEADER = "Content-Encoding";
  static constexpr int NEW_LINE_SIZE = 2; 
};
