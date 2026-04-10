#include "server/commands/command_utils.hpp"
#include "server/core/client_manager/client_manager.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <optional>
#include <random>

namespace server::commands {

template <typename Container, typename Predicate>
static auto findByPredicate(Container &container, Predicate predicate)
    -> std::optional<std::reference_wrapper<typename Container::value_type>>
{
    const auto it = std::find_if(container.begin(), container.end(), predicate);
    if (it == container.end()) {
        return std::nullopt;
    }
    return std::ref(*it);
}

void copyPaddedString(char *destination, const std::size_t size, const std::string_view source)
{
    if (destination == nullptr || size == 0) {
        return;
    }
    const std::size_t copiedLength = source.size() < (size - 1) ? source.size() : (size - 1);
    std::memcpy(destination, source.data(), copiedLength);
    destination[copiedLength] = '\0';
}

std::string buildPacket(const std::uint16_t code, const std::string_view payload)
{
    const std::uint16_t payloadSize = static_cast<std::uint16_t>(payload.size());
    const myteams::PacketHeader header {code, payloadSize};
    std::string packet(sizeof(header) + payloadSize, '\0');

    std::memcpy(packet.data(), &header, sizeof(header));
    if (!payload.empty()) {
        std::memcpy(packet.data() + sizeof(header), payload.data(), payloadSize);
    }
    return packet;
}

void queuePacket(ClientManager &clientManager, const std::int32_t clientFd, const std::string &packet)
{
    clientManager.queueDataToSend(clientFd, packet);
}

void queueStatus(ClientManager &clientManager, const std::int32_t clientFd, const myteams::StatusCode status)
{
    queuePacket(clientManager, clientFd, buildPacket(static_cast<std::uint16_t>(status)));
}

void queueStatus(CommandContext &context, const myteams::StatusCode status)
{
    queueStatus(context.clientManager, context.clientFd, status);
}

void broadcastPacket(
    ClientManager &clientManager,
    const ClientSockets &clientSockets,
    const std::string &packet,
    const std::int32_t excludedClientFd)
{
    for (const auto &[socketFd, socket] : clientSockets) {
        (void)socket;
        if (socketFd == excludedClientFd) {
            continue;
        }
        clientManager.queueDataToSend(socketFd, packet);
    }
}

void broadcastPacket(CommandContext &context, const std::string &packet, const std::int32_t excludedClientFd)
{
    broadcastPacket(context.clientManager, context.clientSockets, packet, excludedClientFd);
}

std::string generateUuid()
{
    static thread_local std::mt19937 randomEngine(std::random_device {}());
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";
    static constexpr std::size_t UUID_CHAR_COUNT = 36;
    std::string uuid(UUID_CHAR_COUNT, '\0');
    std::uniform_int_distribution<int> distribution(0, 15);

    for (std::size_t index = 0; index < UUID_CHAR_COUNT; ++index) {
        if (index == 8 || index == 13 || index == 18 || index == 23) {
            uuid[index] = '-';
            continue;
        }
        uuid[index] = HEX_DIGITS[distribution(randomEngine)];
    }
    return uuid;
}

std::optional<UserRef> findUserByName(std::vector<myteams::User> &users, const std::string_view userName)
{
    return findByPredicate(users,
        [userName](const myteams::User &user) { return user.getName() == userName; });
}

std::optional<UserRef> findUserByUuid(std::vector<myteams::User> &users, const std::string_view userUuid)
{
    return findByPredicate(users,
        [userUuid](const myteams::User &user) { return user.getUuid() == userUuid; });
}

std::optional<TeamRef> findTeamByUuid(std::vector<myteams::Team> &teams, const std::string_view teamUuid)
{
    return findByPredicate(teams,
        [teamUuid](const myteams::Team &team) { return team.getUuid() == teamUuid; });
}

std::optional<ChannelRef> findChannelByUuid(myteams::Team &team, const std::string_view channelUuid)
{
    auto &channels = team.getChannels();
    return findByPredicate(channels,
        [channelUuid](const myteams::Channel &channel) { return channel.getUuid() == channelUuid; });
}

std::optional<ThreadRef> findThreadByUuid(myteams::Channel &channel, const std::string_view threadUuid)
{
    auto &threads = channel.getThreads();
    return findByPredicate(threads,
        [threadUuid](const myteams::Thread &thread) { return thread.getUuid() == threadUuid; });
}

std::optional<UserRef> getAuthenticatedUser(CommandContext &context)
{
    const auto authenticatedUserIt = context.authenticatedUsersByFd.find(context.clientFd);
    if (authenticatedUserIt == context.authenticatedUsersByFd.end()) {
        return std::nullopt;
    }
    return findUserByUuid(context.users, authenticatedUserIt->second);
}

bool isUuidFormatValid(const std::string_view uuid)
{
    if (uuid.size() != myteams::UUID_LENGTH - 1) {
        return false;
    }
    for (std::size_t index = 0; index < uuid.size(); ++index) {
        if (index == 8 || index == 13 || index == 18 || index == 23) {
            if (uuid[index] != '-') {
                return false;
            }
            continue;
        }
        if (!std::isxdigit(static_cast<unsigned char>(uuid[index]))) {
            return false;
        }
    }
    return true;
}

bool isContextCombinationValid(
    const std::string &teamUuid,
    const std::string &channelUuid,
    const std::string &threadUuid)
{
    if (teamUuid.empty() && (!channelUuid.empty() || !threadUuid.empty())) {
        return false;
    }
    if (channelUuid.empty() && !threadUuid.empty()) {
        return false;
    }
    return true;
}

bool extractFixedString(const char *rawData, const std::size_t rawSize, std::string &outString)
{
    if (rawData == nullptr || rawSize == 0) {
        return false;
    }
    const auto *end = static_cast<const char *>(std::memchr(rawData, '\0', rawSize));
    if (end == nullptr) {
        return false;
    }
    outString.assign(rawData, static_cast<std::size_t>(end - rawData));
    return true;
}

std::string buildUserConnectionEventPacket(
    const myteams::StatusCode eventCode,
    const std::string_view userUuid,
    const std::string_view userName)
{
    myteams::PayloadEvtUserConnection payload {};
    copyPaddedString(payload.user_uuid, sizeof(payload.user_uuid), userUuid);
    copyPaddedString(payload.user_name, sizeof(payload.user_name), userName);
    return buildPacket(static_cast<std::uint16_t>(eventCode), payload);
}

} // namespace server::commands

