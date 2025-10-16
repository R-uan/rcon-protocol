#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <unistd.h>

int main() {
  // === Packet definition ===
  uint8_t packet[] = {// Size (18)
                      0x12, 0x00, 0x00, 0x00,
                      // ID (1)
                      0x01, 0x00, 0x00, 0x00,
                      // Type (2)
                      0x02, 0x00, 0x00, 0x00,
                      // Body: "password\0"
                      'p', 'a', 's', 's', 'w', 'o', 'r', 'd', 0x00,
                      // Empty string terminator
                      0x00};

  size_t packetSize = sizeof(packet);

  // === Socket setup ===
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("Socket creation failed");
    return 1;
  }

  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(3000); // port
  if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
    perror("Invalid address");
    close(sock);
    return 1;
  }

  // === Connect to server ===
  if (connect(sock, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    perror("Connection failed");
    close(sock);
    return 1;
  }

  std::cout << "Connected to server, sending packet..." << std::endl;

  // === Send packet ===
  ssize_t sent = send(sock, packet, packetSize, 0);
  if (sent < 0) {
    perror("Send failed");
  } else {
    std::cout << "Sent " << sent << " bytes." << std::endl;
  }

  close(sock);
  std::cout << "Connection closed." << std::endl;

  return 0;
}
