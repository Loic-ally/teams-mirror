#include "client_manager.hpp"

namespace server {

void
ClientManager::addClient(std::int32_t socketFd)
{
	_clients.emplace(socketFd, ClientState {});
}

void
ClientManager::removeClient(std::int32_t socketFd) noexcept
{
	_clients.erase(socketFd);
}

bool
ClientManager::hasClient(std::int32_t socketFd) const noexcept
{
	return _clients.find(socketFd) != _clients.end();
}

bool
ClientManager::hasPendingWrite(std::int32_t socketFd) const noexcept
{
	auto it = _clients.find(socketFd);
	return it != _clients.end() && !it->second.outgoingBuffer.empty();
}

void
ClientManager::appendToIncomingBuffer(std::int32_t socketFd, const char *data, std::size_t size)
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end() || data == nullptr || size == 0)
		return;
	it->second.incomingBuffer.append(data, size);
}

const std::string &
ClientManager::getIncomingBuffer(std::int32_t socketFd) const noexcept
{
	static const std::string EMPTY_BUFFER;
	auto it = _clients.find(socketFd);
	if (it == _clients.end())
		return EMPTY_BUFFER;
	return it->second.incomingBuffer;
}

void
ClientManager::clearIncomingBuffer(std::int32_t socketFd) noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end())
		return;
	it->second.incomingBuffer.clear();
}

void
ClientManager::consumeIncomingBuffer(std::int32_t socketFd, std::size_t consumedBytes) noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end())
		return;
	std::string &incomingBuffer = it->second.incomingBuffer;
	if (consumedBytes >= incomingBuffer.size()) {
		incomingBuffer.clear();
		return;
	}
	incomingBuffer.erase(0, consumedBytes);
}

void
ClientManager::queueDataToSend(std::int32_t socketFd, const char *data, std::size_t size)
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end() || data == nullptr || size == 0)
		return;
	it->second.outgoingBuffer.append(data, size);
}

const char *
ClientManager::getQueuedData(std::int32_t socketFd) const noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end() || it->second.outgoingBuffer.empty())
		return nullptr;
	return it->second.outgoingBuffer.data();
}

std::size_t
ClientManager::getQueuedSize(std::int32_t socketFd) const noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end())
		return 0;
	return it->second.outgoingBuffer.size();
}

void
ClientManager::consumeQueuedData(std::int32_t socketFd, std::size_t consumedBytes) noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end())
		return;
	std::string &outgoingBuffer = it->second.outgoingBuffer;
	if (consumedBytes >= outgoingBuffer.size()) {
		outgoingBuffer.clear();
		return;
	}
	outgoingBuffer.erase(0, consumedBytes);
}

} // namespace server
