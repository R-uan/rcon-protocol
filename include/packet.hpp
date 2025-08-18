#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Packet Type: 32-bit little-endian integer, which indicates the purpose of the packet
// Its value will always be either 0, 2 or 3
enum PacketType {
  SERVERDATA_AUTH = 3,
  SERVERDATA_AUTH_RESPONSE = 2,
  SERVERDATA_EXECCOMAND = 2,
  SERVERDATA_RESPONSE_VALUE = 0
};

struct Packet {
  int32_t size;
  int32_t id;
  int32_t type;
  std::string body;
  int8_t terminator;

  std::vector<unsigned char> build_packet();
  static std::optional<Packet> parse_bytes(char* packet, int packetSize);
  static Packet create_packet(const int id, const int type, std::string body);

  Packet(int32_t size, int32_t id, int32_t type, std::string body) :
    size(size), id(id), type(type), body(body), terminator(0x00) { }
};


// Packet ID: 32-bit little-edian integer chosen by the client for each request.
// It may be set to any positive integer.
// The server response will have the same ID unless it's a failed SERVEDATA_AUTH_RESPONSE

