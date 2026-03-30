#ifndef SERVER_LOGGER_HPP
#define SERVER_LOGGER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <string_view>

namespace server {

class ServerLogger {
    public:
        ServerLogger() = delete;
        ~ServerLogger() = delete;

        static int logTeamCreated(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view userUuid);

        static int logChannelCreated(
            std::string_view teamUuid,
            std::string_view channelUuid,
            std::string_view channelName);

        static int logThreadCreated(
            std::string_view channelUuid,
            std::string_view threadUuid,
            std::string_view userUuid,
            std::string_view threadTitle,
            std::string_view threadBody);

        static int logReplyCreated(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::string_view replyBody);

        static int logUserSubscribed(
            std::string_view teamUuid,
            std::string_view userUuid);

        static int logUserUnsubscribed(
            std::string_view teamUuid,
            std::string_view userUuid);

        static int logUserCreated(
            std::string_view userUuid,
            std::string_view userName);

        static int logUserLoaded(
            std::string_view userUuid,
            std::string_view userName);

        static int logUserLoggedIn(std::string_view userUuid);

        static int logUserLoggedOut(std::string_view userUuid);

        static int logPrivateMessageSent(
            std::string_view senderUuid,
            std::string_view receiverUuid,
            std::string_view messageBody);

        static void logInternalError(std::string_view message);
};

} // namespace server

#endif // SERVER_LOGGER_HPP
