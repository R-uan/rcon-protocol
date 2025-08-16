#include "../include/packet.hpp"
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>

std::optional<Packet> Packet::parse_packet(char* packet, int packetSize) {
  if (packetSize < 14) return std::nullopt;
  int32_t size = *((int32_t*)packet);
  int32_t id = *((int32_t*)packet + 4);
  int32_t type = *((int32_t*) packet + 8);
  std::string body(packet + 12, packetSize - 12); 
  return Packet(size, id, type, body);
}
