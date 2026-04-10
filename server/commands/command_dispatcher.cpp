#include "server/commands/command_dispatcher.hpp"
#include "server/commands/command_utils.hpp"
#include "server/commands/create_command.hpp"
#include "server/commands/help_command.hpp"
#include "server/commands/login_command.hpp"
#include "server/commands/logout_command.hpp"
#include "server/commands/subscription_commands.hpp"
#include "server/commands/use_command.hpp"
#include "server/core/client_manager/client_manager.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace server::commands {
namespace {

using CommandHandler = void (*)(CommandContext &);

struct CommandHandlerEntry {
    std::uint16_t code;
    CommandHandler handler;
};

constexpr std::array<CommandHandlerEntry, 8> COMMAND_HANDLERS {{
    {myteams::CMD_LOGIN, &handleLoginCommand},
    {myteams::CMD_LOGOUT, &handleLogoutCommand},
    {myteams::CMD_USE, &handleUseCommand},
    {myteams::CMD_CREATE, &handleCreateCommand},
    {myteams::CMD_SUBSCRIBE, &handleSubscribeCommand},
    {myteams::CMD_UNSUBSCRIBE, &handleUnsubscribeCommand},
    {myteams::CMD_SUBSCRIBED_LIST, &handleSubscribedListCommand},
    {myteams::CMD_INFO, &handleHelpCommand}
}};

void dispatchCommand(CommandContext &context, const std::uint16_t commandCode)
{
    const auto handlerIt = std::find_if(
        COMMAND_HANDLERS.begin(),
        COMMAND_HANDLERS.end(),
        [commandCode](const CommandHandlerEntry &entry) { return entry.code == commandCode; });

    if (handlerIt == COMMAND_HANDLERS.end()) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }
    handlerIt->handler(context);
}

} // namespace

void processClientIncomingPackets(
    ClientManager &clientManager,
    const std::int32_t clientFd,
    std::vector<myteams::User> &users,
    std::vector<myteams::Team> &teams,
    const ClientSockets &clientSockets,
    AuthenticatedUserByFd &authenticatedUsersByFd)
{
    for (;;) {
        const std::string &incomingBuffer = clientManager.getIncomingBuffer(clientFd);
        if (incomingBuffer.size() < sizeof(myteams::PacketHeader)) {
            return;
        }

        myteams::PacketHeader header {};
        std::memcpy(&header, incomingBuffer.data(), sizeof(header));
        const std::size_t packetSize = sizeof(header) + header.payload_size;
        if (incomingBuffer.size() < packetSize) {
            return;
        }

        CommandContext context {
            clientManager,
            clientFd,
            incomingBuffer.data() + sizeof(header),
            header.payload_size,
            users,
            teams,
            clientSockets,
            authenticatedUsersByFd
        };
        dispatchCommand(context, header.code);
        clientManager.consumeIncomingBuffer(clientFd, packetSize);
    }
}

} // namespace server::commands

