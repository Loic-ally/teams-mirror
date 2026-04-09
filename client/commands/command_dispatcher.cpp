#include "command_dispatcher.hpp"
#include "help_command.hpp"
#include "login_command.hpp"
#include "messages_command.hpp"
#include "logout_command.hpp"
#include "send_command.hpp"
#include "user_command.hpp"
#include "users_command.hpp"
#include "core/client.hpp"
#include "parser/parser.hpp"

#include <array>
#include <algorithm>
#include <stdexcept>
#include <string>

namespace client::commands {

namespace {

using CommandHandler = void (*)(Client &, ParsedInput &);

struct CommandEntry {
    const char *name;
    CommandHandler handler;
};

constexpr std::array<CommandEntry, 7> COMMAND_TABLE {{
    {"/help", &handleHelp},
    {"/login", &handleLogin},
    {"/logout", &handleLogout},
    {"/send", &handleSend},
    {"/messages", &handleMessages},
    {"/users", &handleUsers},
    {"/user", &handleUser}
}};

} // namespace

void dispatchCommand(Client &clientData, ParsedInput &input)
{
    const std::string &command = input.getCommand();

    if (command.empty()) {
        return;
    }

    const auto entryIt = std::find_if(
        COMMAND_TABLE.begin(),
        COMMAND_TABLE.end(),
        [&command](const CommandEntry &entry) { return command == entry.name; });

    if (entryIt != COMMAND_TABLE.end()) {
        entryIt->handler(clientData, input);
        return;
    }
    throw std::invalid_argument("Unknown command: " + command + ". Use /help.");
}

}
