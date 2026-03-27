#include "core/server_manager/server_manager.hpp"
#include "core/server_manager/server_exceptions.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace server {

constexpr std::string_view USAGE = "USAGE: ./myteams_server port";

bool
parsePort(const char *arg, std::uint16_t &outPort)
{
    if (arg == nullptr) {
        return false;
    }
    std::string_view portArg(arg);
    if (portArg.empty()) {
        return false;
    }
    unsigned long parsedPort = 0;
    const char *begin = portArg.data();
    const char *end = begin + portArg.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsedPort);
    if (ec != std::errc() || ptr != end || parsedPort == 0 || parsedPort > 65535) {
        return false;
    }
    outPort = static_cast<std::uint16_t>(parsedPort);
    return true;
}

std::int32_t
run(std::int32_t ac, char **av)
{
    std::uint16_t port = 0;
    if (ac != 2 || !parsePort(av[1], port)) {
        std::cout << USAGE << std::endl;
        return EXIT_FAILURE;
    }
    try {
        ServerManager serverManager(port);
        serverManager.initializeTcpListener();
        //poll
    } catch (const ServerException &exception) {
        std::cerr << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

} // namespace server

std::int32_t
main(std::int32_t ac, char **av)
{
    return server::run(ac, av);
}
