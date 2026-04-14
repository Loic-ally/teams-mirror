#include "command_dispatcher.hpp"
#include "help_command.hpp"
#include "info_command.hpp"
#include "list_command.hpp"
#include "login_command.hpp"
#include "logout_command.hpp"
#include "create_command.hpp"
#include "subscription_commands.hpp"
#include "use_command.hpp"
#include "user_command.hpp"
#include "users_command.hpp"
#include "core/client.hpp"
#include "parser/parser.hpp"

#include <array>
#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace client::commands {

using CommandHandler = std::function<void(Client &, ParsedInput &)>;
using CommandEntry = std::pair<std::string_view, CommandHandler>;

static const std::array<CommandEntry, 12> COMMAND_TABLE {{
    {"/help", &handleHelp},
    {"/info", &handleInfo},
    {"/login", &handleLogin},
    {"/logout", &handleLogout},
    {"/use", &handleUse},
    {"/create", &handleCreate},
    {"/list", &handleList},
    {"/subscribe", &handleSubscribe},
    {"/unsubscribe", &handleUnsubscribe},
    {"/subscribed", &handleSubscribedList},
    {"/user", &handleUser},
    {"/users", &handleUsers}
}};

void dispatchCommand(Client &clientData, ParsedInput &input)
{
    const std::string &command = input.getCommand();

    if (command.empty()) {
        return;
    }

    const auto entryIt = std::find_if(
        COMMAND_TABLE.begin(),
        COMMAND_TABLE.end(),
        [&command](const CommandEntry &entry) { return command == entry.first; });

    if (entryIt != COMMAND_TABLE.end()) {
        entryIt->second(clientData, input);
        return;
    }
    std::cout << "Unknown command: " << command << ". Use /help." << std::endl;
}

}
