#include "packet.hpp"

int main() {
  std::string body = "Hello World!";
  auto testPacket = Packet::create_packet(1, PacketType::SERVERDATA_EXECCOMAND, body);
  auto bytes = testPacket.build_packet();
  return 0;
}
