
#include "Socket.hpp"
#include "System.hpp"
#include "commands/command_dispatcher.hpp"
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include "commands/packet_utils.hpp"
#include "core/client.hpp"
#include "parser/parser.hpp"
#include "protocol.hpp"
#include <sys/poll.h>
#include <thread>
#include <poll.h>
#include <utility>

std::unique_ptr<utils::Socket> createSocket(std::string adress,
                                            std::string port) {
  auto socket =
      std::make_unique<utils::Socket>(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  try {
    socket->connect(adress, std::stoi(port));
  } catch (...) {
    std::cout << "Cannot connect to the adress and port given" << std::endl;
    throw;
  }
  return socket;
}

bool canReadStdin() {
    struct pollfd config{
        0,
        POLLIN,
        0,
    };
    if (::poll(&config, 1, 0) == -1) {
        throw std::runtime_error("poll() failed");
    }
    return config.revents & POLLIN;
}

static void handlingInput(client::Client &clientData) {
    if (!canReadStdin()) {
        return;
    }
    client::ParsedInput input;

    try {
        client::commands::dispatchCommand(clientData, input);
    } catch (const std::exception &exception) {
        std::cerr << "Command error: " << exception.what() << std::endl;
    } catch (...) {
        std::cerr << "Command error: unknown error" << std::endl;
    }
}

static void handleEvent(client::Client &clientData) {
    while(clientData.socket->poll(POLLIN) & POLLIN) {
        myteams::PacketHeader header;
        std::string out;
        std::cout << header.code << " " << header.payload_size << "\n";
        client::commands::readServerPacket(*clientData.socket, header, out);
        client::commands::handleAsyncEventPacket(header.code, out);
    }
}

std::int32_t runLoop(std::unique_ptr<utils::Socket> socket) {
    client::Client clientData;
    clientData.socket = std::move(socket);

    while (clientData.running) {
        handlingInput(clientData);
        handleEvent(clientData);
    }
    return 0;
}

std::int32_t main(std::int32_t ac, char **av) {

  if (ac != 3) {
    std::cout
        << "USAGE: ./myteams_cli ip port\n"
        << "\tip is the server ip address on which the server socket listens\n"
        << "\tport is the port number on which the server socket listens\n";
    if (ac == 2 && std::string_view(av[1]) == "--help") {
      return 0;
    }
    return 84;
  }

  try {
    auto socket = createSocket(av[1], av[2]);
    return runLoop(std::move(socket));
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 84;
  }
}
