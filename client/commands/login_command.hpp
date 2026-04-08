#pragma once

namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleLogin(Client &clientData, ParsedInput &input);

}

