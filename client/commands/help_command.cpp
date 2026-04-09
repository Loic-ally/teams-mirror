#include "help_command.hpp"
#include "core/client.hpp"
#include "parser/parser.hpp"

#include <iostream>
#include <stdexcept>

namespace client::commands {

void handleHelp(Client &clientData, ParsedInput &input)
{
    (void)clientData;
    if (input.hasRemainingArgs()) {
        throw std::invalid_argument("Usage: /help");
    }
    std::cout
        << "/help: display this help\n"
        << "/login \"user_name\": log in with a username\n"
        << "/logout: log out from the current session\n"
        << "/send \"user_uuid\" \"message_body\": send a private message to a user\n"
        << "/messages \"user_uuid\": list private messages with a user\n"
        << "/users: list all users\n"
        << "/user \"user_uuid\": display information about one user"
        << std::endl;
}

}
