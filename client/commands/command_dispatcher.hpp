#pragma once

namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void dispatchCommand(Client &clientData, ParsedInput &input);

}

