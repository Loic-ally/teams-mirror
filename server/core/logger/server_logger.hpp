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

        static void logTeamCreated(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view userUuid);

        static void logChannelCreated(
            std::string_view teamUuid,
            std::string_view channelUuid,
            std::string_view channelName);

        static void logThreadCreated(
            std::string_view channelUuid,
            std::string_view threadUuid,
            std::string_view userUuid,
            std::string_view threadTitle,
            std::string_view threadBody);

        static void logReplyCreated(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::string_view replyBody);

        static void logUserSubscribed(
            std::string_view teamUuid,
            std::string_view userUuid);

        static void logUserUnsubscribed(
            std::string_view teamUuid,
            std::string_view userUuid);

        static void logUserCreated(
            std::string_view userUuid,
            std::string_view userName);

        static void logUserLoaded(
            std::string_view userUuid,
            std::string_view userName);

        static void logUserLoggedIn(std::string_view userUuid);

        static void logUserLoggedOut(std::string_view userUuid);

        static void logPrivateMessageSent(
            std::string_view senderUuid,
            std::string_view receiverUuid,
            std::string_view messageBody);

        static void logInternalError(std::string_view message);
};

} // namespace server

#endif // SERVER_LOGGER_HPP
