#pragma once

namespace client {
class Client;
class ParsedInput;
}

namespace client::commands {

void handleSubscribe(Client &clientData, ParsedInput &input);
void handleUnsubscribe(Client &clientData, ParsedInput &input);
void handleSubscribedList(Client &clientData, ParsedInput &input);

} // namespace client::commands
