#include "server.hpp"

int main() {
  auto server = RconServer::create_instance(3000);
  server.listen();
  return 0;
}
