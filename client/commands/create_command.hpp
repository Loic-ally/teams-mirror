#pragma once

namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleCreate(Client &clientData, ParsedInput &input);

} // namespace client::commands
