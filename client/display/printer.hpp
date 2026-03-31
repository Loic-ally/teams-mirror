#ifndef CLIENT_DISPLAY_PRINTER_HPP
#define CLIENT_DISPLAY_PRINTER_HPP

#ifdef _WIN32
#pragma once
#endif

#include <ctime>
#include <string_view>

class Printer {
    public:
        Printer() = delete;
        ~Printer() = delete;

        // Asynchronous events received from the server.
        static int eventLoggedIn(
            std::string_view userUuid,
            std::string_view userName);

        static int eventLoggedOut(
            std::string_view userUuid,
            std::string_view userName);

        static int eventPrivateMessageReceived(
            std::string_view userUuid,
            std::string_view messageBody);

        static int eventThreadReplyReceived(
            std::string_view teamUuid,
            std::string_view threadUuid,
            std::string_view userUuid,
            std::string_view replyBody);

        static int eventTeamCreated(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view teamDescription);

        static int eventChannelCreated(
            std::string_view channelUuid,
            std::string_view channelName,
            std::string_view channelDescription);

        static int eventThreadCreated(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t threadTimestamp,
            std::string_view threadTitle,
            std::string_view threadBody);

        // Synchronous responses for lists and entity details.
        static int printUsers(
            std::string_view userUuid,
            std::string_view userName,
            int userStatus);

        static int printTeams(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view teamDescription);

        static int printTeamChannels(
            std::string_view channelUuid,
            std::string_view channelName,
            std::string_view channelDescription);

        static int printChannelThreads(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t threadTimestamp,
            std::string_view threadTitle,
            std::string_view threadBody);

        static int printThreadReplies(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t replyTimestamp,
            std::string_view replyBody);

        static int printPrivateMessages(
            std::string_view senderUuid,
            std::time_t messageTimestamp,
            std::string_view messageBody);

        static int printUser(
            std::string_view userUuid,
            std::string_view userName,
            int userStatus);

        static int printTeam(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view teamDescription);

        static int printChannel(
            std::string_view channelUuid,
            std::string_view channelName,
            std::string_view channelDescription);

        static int printThread(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t threadTimestamp,
            std::string_view threadTitle,
            std::string_view threadBody);

        static int printTeamCreated(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view teamDescription);

        static int printChannelCreated(
            std::string_view channelUuid,
            std::string_view channelName,
            std::string_view channelDescription);

        static int printThreadCreated(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t threadTimestamp,
            std::string_view threadTitle,
            std::string_view threadBody);

        static int printReplyCreated(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t replyTimestamp,
            std::string_view replyBody);

        static int printSubscribed(
            std::string_view userUuid,
            std::string_view teamUuid);

        static int printUnsubscribed(
            std::string_view userUuid,
            std::string_view teamUuid);

        // Error responses.
        static int errorUnknownTeam(std::string_view teamUuid);
        static int errorUnknownChannel(std::string_view channelUuid);
        static int errorUnknownThread(std::string_view threadUuid);
        static int errorUnknownUser(std::string_view userUuid);
        static int errorUnauthorized();
        static int errorAlreadyExist();
};

#endif // CLIENT_DISPLAY_PRINTER_HPP
