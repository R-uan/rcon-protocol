// stress_test_reuse_sockets.cpp
// Multi-client stress tester (authorized use only). Each client authenticates
// once then reuses the same socket to send many echo messages and optionally
// reads responses. Compile: g++ -std=c++17 -pthread
// stress_test_reuse_sockets.cpp -o stress_test_reuse_sockets

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

std::atomic<int> activeConnections{0};
std::atomic<long long> totalSentBytes{0};
std::atomic<long long> totalMessages{0};
std::mutex outMu;

// The packets from the user's original example
static const uint8_t AUTH_PACKET[] = {
    0x12, 0x00, 0x00, 0x00,                           // size 18
    0x01, 0x00, 0x00, 0x00,                           // id 1
    0x02, 0x00, 0x00, 0x00,                           // type 2
    'p',  'a',  's',  's',  'w', 'o', 'r', 'd', 0x00, // body "password\0"
    0x00                                              // empty string terminator
};
static const size_t AUTH_PACKET_SIZE = sizeof(AUTH_PACKET);

// Factory to build an echo packet for a given string body (null-terminated body
// + empty terminator)
static std::vector<uint8_t> make_echo_packet(const std::string &body) {
  // size = 4(size) + 4(id) + 4(type) + body_len + 1 (body null) + 1 (empty
  // terminator)
  uint32_t size = 4 + 4 + 4 + static_cast<uint32_t>(body.size()) + 1 + 1;
  std::vector<uint8_t> buf;
  buf.reserve(size);
  // size little-endian
  buf.push_back(static_cast<uint8_t>(size & 0xFF));
  buf.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
  buf.push_back(0x00);
  buf.push_back(0x00);
  // id = 1
  buf.push_back(0x01);
  buf.push_back(0x00);
  buf.push_back(0x00);
  buf.push_back(0x00);
  // type = 2
  buf.push_back(0x02);
  buf.push_back(0x00);
  buf.push_back(0x00);
  buf.push_back(0x00);
  // body characters
  buf.insert(buf.end(), body.begin(), body.end());
  // null terminator for body
  buf.push_back(0x00);
  // empty string terminator
  buf.push_back(0x00);
  return buf;
}

// Set recv timeout on socket (milliseconds)
static void set_recv_timeout(int sock, int ms) {
  struct timeval tv;
  tv.tv_sec = ms / 1000;
  tv.tv_usec = (ms % 1000) * 1000;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
}

void client_thread(const std::string &host, uint16_t port,
                   int messages_per_connection,
                   int interval_ms_between_messages, bool read_responses,
                   int id, int reconnect_backoff_ms) {
  std::vector<uint8_t> echoPacketTemplate = make_echo_packet("echo hi");

  int sock = -1;
  int attempts = 0;

  auto connect_socket = [&](int &out_sock) -> bool {
    if (out_sock >= 0) {
      close(out_sock);
      out_sock = -1;
    }
    out_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (out_sock < 0) {
      std::lock_guard<std::mutex> lock(outMu);
      std::cerr << "[client " << id << "] socket() failed: " << strerror(errno)
                << "\n";
      return false;
    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
      std::lock_guard<std::mutex> lock(outMu);
      std::cerr << "[client " << id << "] invalid address: " << host << "\n";
      close(out_sock);
      out_sock = -1;
      return false;
    }

    if (connect(out_sock, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
      std::lock_guard<std::mutex> lock(outMu);
      std::cerr << "[client " << id << "] connect() failed: " << strerror(errno)
                << "\n";
      close(out_sock);
      out_sock = -1;
      return false;
    }
    // set a modest recv timeout so reads don't block forever
    set_recv_timeout(out_sock, 500);
    return true;
  };

  // Try to connect and authenticate once, with retries
  while (true) {
    if (!connect_socket(sock)) {
      ++attempts;
      std::this_thread::sleep_for(
          std::chrono::milliseconds(reconnect_backoff_ms));
      if (attempts > 10) {
        std::lock_guard<std::mutex> lock(outMu);
        std::cerr << "[client " << id
                  << "] giving up after multiple connect attempts\n";
        return;
      }
      continue;
    }

    // send auth packet
    ssize_t s = send(sock, AUTH_PACKET, AUTH_PACKET_SIZE, 0);
    if (s <= 0) {
      std::lock_guard<std::mutex> lock(outMu);
      std::cerr << "[client " << id << "] auth send failed: " << strerror(errno)
                << "\n";
      close(sock);
      sock = -1;
      std::this_thread::sleep_for(
          std::chrono::milliseconds(reconnect_backoff_ms));
      continue;
    }
    totalSentBytes.fetch_add(s);
    totalMessages.fetch_add(1);

    // Optionally read an auth response (non-blocking due to timeout). Not
    // required.
    if (read_responses) {
      char rbuf[512];
      ssize_t r = recv(sock, rbuf, sizeof(rbuf), 0);
      if (r > 0) {
        std::lock_guard<std::mutex> lock(outMu);
        std::cout << "[client " << id << "] auth response len=" << r << "\n";
      }
    }

    break; // authenticated (or at least sent auth). Proceed to reuse socket.
  }

  activeConnections.fetch_add(1);

  // Now reuse the same socket for multiple echo messages
  for (int i = 0; i < messages_per_connection; ++i) {
    // Build a message; if you want variety, change the body here
    std::string body = "echo hi ";
    body += std::to_string(i);
    std::vector<uint8_t> pkt = make_echo_packet(body);

    ssize_t sent = send(sock, pkt.data(), pkt.size(), 0);
    if (sent <= 0) {
      std::lock_guard<std::mutex> lock(outMu);
      std::cerr << "[client " << id << "] send failed at iteration " << i
                << ": " << strerror(errno) << "\n";
      // attempt to reconnect and re-authenticate then continue
      close(sock);
      sock = -1;
      activeConnections.fetch_sub(1);
      std::this_thread::sleep_for(
          std::chrono::milliseconds(reconnect_backoff_ms));
      if (!connect_socket(sock)) {
        std::lock_guard<std::mutex> lock2(outMu);
        std::cerr << "[client " << id
                  << "] reconnect failed, stopping thread\n";
        return;
      }
      // resend auth
      ssize_t s = send(sock, AUTH_PACKET, AUTH_PACKET_SIZE, 0);
      if (s <= 0) {
        std::lock_guard<std::mutex> lock3(outMu);
        std::cerr << "[client " << id
                  << "] auth resend failed after reconnect: " << strerror(errno)
                  << "\n";
        close(sock);
        sock = -1;
        return;
      }
      totalSentBytes.fetch_add(s);
      totalMessages.fetch_add(1);
      activeConnections.fetch_add(1);

      // try sending the original packet once more
      sent = send(sock, pkt.data(), pkt.size(), 0);
      if (sent <= 0) {
        std::lock_guard<std::mutex> lock4(outMu);
        std::cerr << "[client " << id
                  << "] send failed after reconnect: " << strerror(errno)
                  << "\n";
        return;
      }
    }

    totalSentBytes.fetch_add(sent);
    totalMessages.fetch_add(1);

    if (read_responses) {
      // read reply (non-blocking due to recv timeout)
      char rbuf[1024];
      ssize_t r = recv(sock, rbuf, sizeof(rbuf) - 1, 0);
      if (r > 0) {
        rbuf[r] = '\0';
        std::lock_guard<std::mutex> lock(outMu);
        std::cout << "[client " << id << "] recv(" << r << ")='" << rbuf
                  << "'\n";
      }
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(interval_ms_between_messages));
  }

  // Keep connection open optionally longer? For now close when done
  if (sock >= 0) {
    close(sock);
    sock = -1;
  }
  activeConnections.fetch_sub(1);

  std::lock_guard<std::mutex> lock(outMu);
  std::cout << "[client " << id
            << "] finished, messages sent: " << messages_per_connection << "\n";
}

void stats_printer(std::atomic<bool> &running) {
  while (running.load()) {
    {
      std::lock_guard<std::mutex> lock(outMu);
      std::cout << "[stats] active: " << activeConnections.load()
                << " | total messages: " << totalMessages.load()
                << " | total bytes: " << totalSentBytes.load() << "\n";
    }
    std::this_thread::sleep_for(1s);
  }
}

int main(int argc, char **argv) {
  if (argc < 7) {
    std::cerr << "Usage: " << argv[0]
              << " <host> <port> <num_clients> <messages_per_conn> "
                 "<interval_ms> <read_responses:0|1>\n";
    std::cerr << "Example: " << argv[0] << " 127.0.0.1 3000 100 100 50 1\n";
    return 1;
  }

  std::string host = argv[1];
  uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
  int numClients = std::stoi(argv[3]);
  int messagesPerConn = std::stoi(argv[4]);
  int intervalMs = std::stoi(argv[5]);
  bool readResponses = (std::stoi(argv[6]) != 0);

  int reconnect_backoff_ms = 200; // ms

  std::vector<std::thread> threads;
  threads.reserve(numClients);

  std::atomic<bool> running{true};
  std::thread statsThread(stats_printer, std::ref(running));

  for (int i = 0; i < numClients; ++i) {
    threads.emplace_back(client_thread, host, port, messagesPerConn, intervalMs,
                         readResponses, i, reconnect_backoff_ms);
    std::this_thread::sleep_for(std::chrono::milliseconds(2)); // tiny stagger
  }

  for (auto &t : threads)
    if (t.joinable())
      t.join();

  running.store(false);
  if (statsThread.joinable())
    statsThread.join();

  std::cout << "All clients finished. total messages: " << totalMessages.load()
            << " total bytes: " << totalSentBytes.load() << "\n";
  return 0;
}
