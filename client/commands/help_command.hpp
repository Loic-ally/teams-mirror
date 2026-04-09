#pragma once

namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleHelp(Client &clientData, ParsedInput &input);

}
