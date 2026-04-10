#include "server/core/logger/server_logger.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

extern "C" {
    #include "../../../libs/myteams/logging_server.h"
}

namespace server {

static std::string makeCString(std::string_view value)
{
    return std::string(value);
}

static void ensureLibCallSucceeded(const int rawResult)
{
    if (rawResult < 0) {
        throw std::runtime_error(
            "libmyteams server call failed with return code " + std::to_string(rawResult));
    }
}

void ServerLogger::logTeamCreated(
    const std::string_view teamUuid,
    const std::string_view teamName,
    const std::string_view userUuid)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string teamNameBuffer = makeCString(teamName);
    const std::string userUuidBuffer = makeCString(userUuid);

    ensureLibCallSucceeded(server_event_team_created(
        teamUuidBuffer.c_str(),
        teamNameBuffer.c_str(),
        userUuidBuffer.c_str()));
}

void ServerLogger::logChannelCreated(
    const std::string_view teamUuid,
    const std::string_view channelUuid,
    const std::string_view channelName)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string channelNameBuffer = makeCString(channelName);

    ensureLibCallSucceeded(server_event_channel_created(
        teamUuidBuffer.c_str(),
        channelUuidBuffer.c_str(),
        channelNameBuffer.c_str()));
}

void ServerLogger::logThreadCreated(
    const std::string_view channelUuid,
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::string_view threadTitle,
    const std::string_view threadBody)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string threadTitleBuffer = makeCString(threadTitle);
    const std::string threadBodyBuffer = makeCString(threadBody);

    ensureLibCallSucceeded(server_event_thread_created(
        channelUuidBuffer.c_str(),
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        threadTitleBuffer.c_str(),
        threadBodyBuffer.c_str()));
}

void ServerLogger::logReplyCreated(
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::string_view replyBody)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string replyBodyBuffer = makeCString(replyBody);

    ensureLibCallSucceeded(server_event_reply_created(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        replyBodyBuffer.c_str()));
}

void ServerLogger::logUserSubscribed(
    const std::string_view teamUuid,
    const std::string_view userUuid)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string userUuidBuffer = makeCString(userUuid);

    ensureLibCallSucceeded(server_event_user_subscribed(
        teamUuidBuffer.c_str(),
        userUuidBuffer.c_str()));
}

void ServerLogger::logUserUnsubscribed(
    const std::string_view teamUuid,
    const std::string_view userUuid)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string userUuidBuffer = makeCString(userUuid);

    ensureLibCallSucceeded(server_event_user_unsubscribed(
        teamUuidBuffer.c_str(),
        userUuidBuffer.c_str()));
}

void ServerLogger::logUserCreated(
    const std::string_view userUuid,
    const std::string_view userName)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    ensureLibCallSucceeded(server_event_user_created(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str()));
}

void ServerLogger::logUserLoaded(
    const std::string_view userUuid,
    const std::string_view userName)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    ensureLibCallSucceeded(server_event_user_loaded(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str()));
}

void ServerLogger::logUserLoggedIn(const std::string_view userUuid)
{
    const std::string userUuidBuffer = makeCString(userUuid);

    ensureLibCallSucceeded(server_event_user_logged_in(userUuidBuffer.c_str()));
}

void ServerLogger::logUserLoggedOut(const std::string_view userUuid)
{
    const std::string userUuidBuffer = makeCString(userUuid);

    ensureLibCallSucceeded(server_event_user_logged_out(userUuidBuffer.c_str()));
}

void ServerLogger::logPrivateMessageSent(
    const std::string_view senderUuid,
    const std::string_view receiverUuid,
    const std::string_view messageBody)
{
    const std::string senderUuidBuffer = makeCString(senderUuid);
    const std::string receiverUuidBuffer = makeCString(receiverUuid);
    const std::string messageBodyBuffer = makeCString(messageBody);

    ensureLibCallSucceeded(server_event_private_message_sended(
        senderUuidBuffer.c_str(),
        receiverUuidBuffer.c_str(),
        messageBodyBuffer.c_str()));
}

void ServerLogger::logInternalError(const std::string_view message)
{
    std::cout << "[Server][Internal] " << message << '\n';
}

} // namespace server
