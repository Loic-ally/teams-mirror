#pragma once

namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleList(Client &clientData, ParsedInput &input);

} // namespace client::commands
