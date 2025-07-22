#include <asm-generic/socket.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <array>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <error.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include <filesystem>
#include <thread>

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Server.h"

Server::Server() {
      size_t resourcePrefix = std::string(RESOURCE_DIR).size();
      std::filesystem::path resourcePath(RESOURCE_DIR);
      for(const auto& p : std::filesystem::directory_iterator(resourcePath)) {
        if(std::filesystem::is_regular_file(p)) {
          std::string relativePath = std::string(p.path().c_str()).substr(resourcePrefix);
          resourcePaths[relativePath] = p.path().c_str();
        }
      }

      getListenerSocket();
      if((epollfd = epoll_create(1)) == -1) {
        std::cout << "epoll_create error: " << strerror(errno) << std::endl;
        exit(1);
      }
      addNewConnection(listenerSocket);

    }

    void Server::start() {
      for(int i = 0; i < NUM_THREADS; ++i) {
        threads[i] = std::thread([this](){this->threadExec();});
      }
      threadExec();
    }
    void Server::threadExec() {
      while(true) {
        int n = epoll_wait(epollfd, events.data(), MAX_EVENTS, -1);
        if(n == -1) {
          std::cout << "epoll_wait error: " << strerror(errno) << std::endl;
          exit(1);
        }
        for(int i = 0; i < n; ++i) {
          //new connection
          HttpConnection* connection = static_cast<HttpConnection*>(events[i].data.ptr);
          if(connection->fd == listenerSocket) {
            sockaddr_storage incomingSock;
            socklen_t incomingSockSize = sizeof(incomingSock);
            while(true) {
              int newSockfd = accept(listenerSocket, (sockaddr*)(&incomingSock), &incomingSockSize);
              if(newSockfd < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                  break;
                }
                std::cout << "accept error: " << strerror(errno) << std::endl;
                exit(1);
              }
              std::cout << "New Connection" << std::endl;
              addNewConnection(newSockfd);
            }
            rearmSocket(connection);
          }
          else if(events[i].events&EPOLLIN) {
            std::cout << "Reading Connection\n";

            HttpRequest request;
            readRequest(connection);
            if(connection->state == HttpConnection::SocketState::NONE) {
              std::cout << "Removing Connection\n";
              removeConnection(connection);
              continue;
            }
            if(connection->state == HttpConnection::SocketState::WRITING) {
              buildResponse(connection);
            }
            rearmSocket(connection);
          }
          else if(events[i].events&EPOLLOUT) {
            sendResponse(connection);
            rearmSocket(connection);
          }
          //drop the connection
          else if(events[i].events&(EPOLLRDHUP|EPOLLHUP)) {
            std::cout << "Removing Connection\n";
            removeConnection(connection);
          }
        }
      }
    }


    //1.create addrinfo using getaddrinfo
    //2.create socket using addrinfo
    //3.bind the socket to the addr
    void Server::getListenerSocket() {
      addrinfo* listenerInfo{};
      addrinfo hints{};
      //ip4/ip6 agnostic
      hints.ai_family = AF_UNSPEC;
      //TCP
      hints.ai_socktype = SOCK_STREAM;
      //server
      hints.ai_flags = AI_PASSIVE;
      
      int status;
      if((status = getaddrinfo(nullptr, PORT_NUMBER, &hints, &listenerInfo)) != 0) {
        std::cout << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        exit(1);
      }

      if((listenerSocket = socket(listenerInfo->ai_family, listenerInfo->ai_socktype, listenerInfo->ai_protocol)) == -1) {
        std::cout << "socket error: " << strerror(errno) << std::endl;
        exit(1);
      }

      //fix addr already in use error
      int t = true;
      if(setsockopt(listenerSocket, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int)) == -1) {
        std::cout << "setsockopt error: " << strerror(errno) << std::endl;
        exit(1);
      }

      if(bind(listenerSocket, listenerInfo->ai_addr, listenerInfo->ai_addrlen) != 0) {
        std::cout << "bind error: " << strerror(errno) << std::endl;
        exit(1);
      }
      freeaddrinfo(listenerInfo);
      setNonBlocking(listenerSocket);
      listen(listenerSocket, BACKLOG);
    }
    void Server::buildResponse(HttpConnection* connection) {
      HttpRequest request(connection->data);
      if(!request.isValid()) {
        connection->data = HttpResponseBuilder()
          .addStatus("400")
          .addHeader("Host", Server::NAME)
          .create();
      }
      else if(request.getMethod() != HttpRequest::Method::GET && request.getMethod() != HttpRequest::Method::HEAD) {
        connection->data = HttpResponseBuilder()
          .addStatus("501")
          .addHeader("Host", Server::NAME)
          .create();
      }
      else {
        std::string resource = request.getUri();
        auto it = resourcePaths.find(resource.compare("/") ? resource : "/index.html");

        if(it == resourcePaths.end()) {
          connection->data = HttpResponseBuilder()
            .addStatus("404")
            .addHeader("Host", Server::NAME)
            .create();
        }
        else {
          std::ifstream file(it->second);
          if(file.fail()) { 
            connection->data = HttpResponseBuilder()
              .addStatus("404")
              .addHeader("Host", Server::NAME)
              .create();
          }
          else {
            std::stringstream strBuffer;
            strBuffer << file.rdbuf();
            connection->data = HttpResponseBuilder()
              .addStatus("200")
              .addHeader("Host", Server::NAME)
              .addHeader("Content-length", std::to_string(file.tellg()))
              .addHeader("Content-type", contentType[it->first.substr(it->first.find('.')+1)])
              .addResource(strBuffer.str())
              .create();
          }
        }
      }
      connection->dataIndex = 0;
      std::cout << "Response: " << connection->data.substr(9, 3) << "\n";
    }

    void Server::sendResponse(HttpConnection* connection) {
      while(true) {
        int bytesSent = send(connection->fd, connection->data.data()+connection->dataIndex, connection->data.size()-connection->dataIndex, 0);
        if(bytesSent < 0) {
          if(errno == EAGAIN || errno == EWOULDBLOCK) break;
          std::cout << "send error: " << strerror(errno) << std::endl;
          exit(1);
        }
        if(bytesSent == 0 || bytesSent+connection->dataIndex == connection->data.size()) {
          connection->state = HttpConnection::SocketState::READING;
          connection->data = std::string(MAX_REQUEST_SIZE, 0);
          connection->dataIndex = 0;
          break;
        }
        connection->dataIndex += bytesSent;
      }
    }

    void Server::setNonBlocking(int fd) {
      if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL,0)|O_NONBLOCK) == -1) {
        std::cout << "fcntl error: " << strerror(errno) << std::endl;
        exit(1);
      }
    }
    void Server::rearmSocket(HttpConnection* connection) {
      epoll_event event{};
      event.events = EPOLL_FLAGS;
      event.data.ptr = connection;
      if(connection->state == HttpConnection::SocketState::READING) {
        event.events |= EPOLLIN;
      }
      else if(connection->state == HttpConnection::SocketState::WRITING) {
        event.events |= EPOLLOUT;
      }
      epoll_ctl(epollfd, EPOLL_CTL_MOD, connection->fd, &event);
    }
    void Server::addNewConnection(int fd) {
      epoll_event event{};
      event.events = EPOLL_FLAGS|EPOLLIN;
      event.data.ptr = new HttpConnection(fd);
      setNonBlocking(fd);
      epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event); 
    }
    void Server::removeConnection(HttpConnection* connection) {
      epoll_ctl(epollfd, EPOLL_CTL_DEL, connection->fd, nullptr);
      close(connection->fd);
      delete connection;
    }
    
    void Server::readRequest(HttpConnection* connection) {
      while(true) {
        int bytesRecv = recv(connection->fd, connection->data.data()+connection->dataIndex, MAX_REQUEST_SIZE-connection->dataIndex, 0);
        if(bytesRecv < 0) {
          if(errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cout << "recv error: " << strerror(errno) << std::endl;
            exit(1);
          }
          //stopped recving data, checks for end of headers "\r\n\r\n"
          if(std::string::npos != connection->data.find(std::string(HTTP::NEW_LINE).append(HTTP::NEW_LINE),
              std::max(0, connection->dataIndex-2*HTTP::NEW_LINE_SIZE))) {
            connection->state = HttpConnection::SocketState::WRITING;
          }
          break;
        }
        //connection closed
        else if(bytesRecv == 0) {
          connection->state = HttpConnection::SocketState::NONE;
          break;
        }
        connection->dataIndex += bytesRecv;
      }
    }

