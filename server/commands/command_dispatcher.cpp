#include "server/commands/command_dispatcher.hpp"
#include "commands/command_context.hpp"
#include "commands/messages_command.hpp"
#include "commands/send_command.hpp"
#include "models/message/message.hpp"
#include "protocol.hpp"
#include "server/commands/command_utils.hpp"
#include "server/commands/create_command.hpp"
#include "server/commands/info_command.hpp"
#include "server/commands/list_command.hpp"
#include "server/commands/login_command.hpp"
#include "server/commands/logout_command.hpp"
#include "server/commands/subscription_commands.hpp"
#include "server/commands/user_command.hpp"
#include "server/commands/users_command.hpp"
#include "server/commands/use_command.hpp"
#include "server/core/client_manager/client_manager.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <utility>

namespace server::commands {

using CommandHandler = std::function<void(CommandContext &)>;
using CommandHandlerEntry = std::pair<std::uint16_t, CommandHandler>;

static const std::array<CommandHandlerEntry, 13> COMMAND_HANDLERS {{
    {myteams::CMD_LOGIN, &handleLoginCommand},
    {myteams::CMD_USERS, &handleUsersCommand},
    {myteams::CMD_USER_INFO, &handleUserCommand},
    {myteams::CMD_INFO, &handleInfoCommand},
    {myteams::CMD_LOGOUT, &handleLogoutCommand},
    {myteams::CMD_USE, &handleUseCommand},
    {myteams::CMD_CREATE, &handleCreateCommand},
    {myteams::CMD_LIST, &handleListCommand},
    {myteams::CMD_SUBSCRIBE, &handleSubscribeCommand},
    {myteams::CMD_UNSUBSCRIBE, &handleUnsubscribeCommand},
    {myteams::CMD_SUBSCRIBED_LIST, &handleSubscribedListCommand},
    {myteams::CMD_MESSAGES, &handleMessagesCommand},
    {myteams::CMD_SEND, &handleSendCommand},
}};

static void dispatchCommand(CommandContext &context, const std::uint16_t commandCode)
{
    const auto handlerIt = std::find_if(
        COMMAND_HANDLERS.begin(),
        COMMAND_HANDLERS.end(),
        [commandCode](const CommandHandlerEntry &entry) { return entry.first == commandCode; });

    if (handlerIt == COMMAND_HANDLERS.end()) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }
    handlerIt->second(context);
}

void processClientIncomingPackets(
    ClientManager &clientManager,
    const std::int32_t clientFd,
    std::vector<myteams::User> &users,
    std::vector<myteams::Team> &teams,
    std::vector<myteams::Message> &messages,
    const ClientSockets &clientSockets,
    AuthenticatedUserByFd &authenticatedUsersByFd,
    AuthenticatedUserByUUID &authenticatedUsersByUUID
)
{
    while(true) {
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
            std::string_view(incomingBuffer.data() + sizeof(header), header.payload_size),
            header.payload_size,
            users,
            teams,
            messages,
            clientSockets,
            authenticatedUsersByFd,
            authenticatedUsersByUUID
        };
        dispatchCommand(context, header.code);
        clientManager.consumeIncomingBuffer(clientFd, packetSize);
    }
}

} // namespace server::commands

