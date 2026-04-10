#pragma once

namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleInfo(Client &clientData, ParsedInput &input);

} // namespace client::commands
