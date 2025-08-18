#include "packet.hpp"
#include "tools.hpp"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

std::optional<Packet> Packet::parse_bytes(char* packet, int packetSize) {
  if (packetSize < 14) return std::nullopt;
  int32_t size = *((int32_t*)packet);
  int32_t id = *((int32_t*)packet + 4);
  int32_t type = *((int32_t*) packet + 8);
  std::string body(packet + 12, packetSize - 12); 
  return Packet(size, id, type, body);
}

Packet Packet::create_packet(const int32_t id, const int32_t type, std::string body) {
  int32_t size = body.length() + 9;
  return Packet(size, id, type, body);
}

std::vector<unsigned char> Packet::build_packet() {
  unsigned char bytes[4];
  std::vector<unsigned char> packet; 
  auto bodyBytes = string_to_bytes(body);

  int_to_bytes(bodyBytes.size() + 9, bytes);
  packet.insert(packet.end(), bytes, bytes + 4);
  
  int_to_bytes(id, bytes);  
  packet.insert(packet.end(), bytes, bytes + 4);

  int_to_bytes(type,bytes);
  packet.insert(packet.end(), bytes, bytes + 4);

  packet.insert(packet.end(), bodyBytes.begin(), bodyBytes.end());
  packet.push_back(0x00);

  return packet;  
}
