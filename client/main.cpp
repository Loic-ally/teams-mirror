
#include "Socket.hpp"
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <span>
#include <string>
#include <string_view>
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
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 84;
  }
}
