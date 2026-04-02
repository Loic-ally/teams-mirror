#include "Socket.hpp"
#include <memory>

namespace client {
    class Client {
        public:
            std::unique_ptr<utils::Socket> socket;
            bool running = true;
            bool connected = false;
    };
}
