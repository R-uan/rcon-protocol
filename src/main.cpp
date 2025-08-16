#include "packet.hpp"
#include <iostream>
#include <optional>
#include <ostream>

int main() {
  char testPacket[] = {
    0x16, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, // id = 1
    0x02, 0x00, 0x00, 0x00, // type = 2
    'H', 'e', 'l', 'l', 'o', '!', ' ', 'W', 'o', 'r', 'l', 'd', 0x00, // payload = "Hello!" (null-terminated)
    0x00 // null terminator
  };

  int packetSize = sizeof(testPacket); // 20 bytes
  auto packet = Packet::parse_packet(testPacket, packetSize);
  if (packet == std::nullopt) std::cout << "Null";
  else {
    std::cout << packet.value().body << std::endl;  
  }
  
  return 0;
}
