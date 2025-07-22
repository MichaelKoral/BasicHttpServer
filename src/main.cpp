#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <error.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "Server.h"

constexpr char const* PORT = "80";
constexpr char const* CONNECTION = "Connection: close";
constexpr int BACKLOG = 10;

int main() {
  Server server;
  server.start();
}
