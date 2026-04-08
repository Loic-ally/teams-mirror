#include "command_dispatcher.hpp"
#include "help_command.hpp"
#include "login_command.hpp"
#include "logout_command.hpp"
#include "core/client.hpp"
#include "parser/parser.hpp"

#include <iostream>
#include <string>

namespace client::commands {

void dispatchCommand(Client &clientData, ParsedInput &input)
{
    const std::string &command = input.getCommand();

    if (command.empty()) {
        return;
    }
	if (command == "/help") {
		handleHelp(clientData, input);
		return;
	}
    if (command == "/login") {
        handleLogin(clientData, input);
        return;
    }
    if (command == "/logout") {
        handleLogout(clientData, input);
        return;
    }
    std::cout << "Unknown command: " << command << ". Use /help." << std::endl;
}

}
