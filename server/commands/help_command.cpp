#include "server/commands/help_command.hpp"
#include "server/commands/command_utils.hpp"

namespace server::commands {

void handleHelpCommand(CommandContext &context)
{
    if (context.payloadSize != 0) {
        queueStatus(context, myteams::ERR_BAD_REQUEST);
        return;
    }
    queueStatus(context, myteams::RPL_OK);
}

} // namespace server::commands

