#pragma once

namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleSend(Client &clientData, ParsedInput &input);

} // namespace client::commands
