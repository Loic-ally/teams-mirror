#include "client_manager.hpp"

namespace server {

void
ClientManager::addClient(std::int32_t socketFd)
{
	_clients.emplace(socketFd, Client {});
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
ClientManager::appendToIncomingBuffer(std::int32_t socketFd, const std::string_view data)
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end() || data.empty())
		return;
	it->second.incomingBuffer.append(data);
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
ClientManager::queueDataToSend(std::int32_t socketFd, const std::string_view data)
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end() || data.empty())
		return;
	it->second.outgoingBuffer.append(data);
}

std::string_view
ClientManager::getQueuedData(std::int32_t socketFd) const noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end() || it->second.outgoingBuffer.empty())
		return {};
	return it->second.outgoingBuffer;
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

void
ClientManager::setContext(
	std::int32_t socketFd,
	const std::string_view teamUuid,
	const std::string_view channelUuid,
	const std::string_view threadUuid) noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end()) {
		return;
	}
	it->second.setContext(teamUuid, channelUuid, threadUuid);
}

std::string
ClientManager::getContextTeamUuid(std::int32_t socketFd) const noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end()) {
		return {};
	}
	return it->second.getContextTeamUuid();
}

std::string
ClientManager::getContextChannelUuid(std::int32_t socketFd) const noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end()) {
		return {};
	}
	return it->second.getContextChannelUuid();
}

std::string
ClientManager::getContextThreadUuid(std::int32_t socketFd) const noexcept
{
	auto it = _clients.find(socketFd);
	if (it == _clients.end()) {
		return {};
	}
	return it->second.getContextThreadUuid();
}

} // namespace server
