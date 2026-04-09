#pragma once

namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleLogout(Client &clientData, ParsedInput &input);

}

