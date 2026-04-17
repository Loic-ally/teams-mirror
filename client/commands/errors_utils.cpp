#include "errors_utils.hpp"

namespace client::commands {
    void handleNotFound(Client &clientData) {
        if (!clientData.contextThreadUuid.empty()) {
            Printer::errorUnknownThread(clientData.contextThreadUuid);
            return;
        }
        if (!clientData.contextChannelUuid.empty()) {
            Printer::errorUnknownChannel(clientData.contextChannelUuid);
            return;
        }
        if (!clientData.contextTeamUuid.empty()) {
            Printer::errorUnknownTeam(clientData.contextTeamUuid);
            return;
        }
    }
}
