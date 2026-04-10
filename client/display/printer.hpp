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

        static void eventLoggedIn(
            std::string_view userUuid,
            std::string_view userName);

        static void eventLoggedOut(
            std::string_view userUuid,
            std::string_view userName);

        static void eventPrivateMessageReceived(
            std::string_view userUuid,
            std::string_view messageBody);

        static void eventThreadReplyReceived(
            std::string_view teamUuid,
            std::string_view threadUuid,
            std::string_view userUuid,
            std::string_view replyBody);

        static void eventTeamCreated(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view teamDescription);

        static void eventChannelCreated(
            std::string_view channelUuid,
            std::string_view channelName,
            std::string_view channelDescription);

        static void eventThreadCreated(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t threadTimestamp,
            std::string_view threadTitle,
            std::string_view threadBody);

        static void printUsers(
            std::string_view userUuid,
            std::string_view userName,
            int userStatus);

        static void printTeams(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view teamDescription);

        static void printTeamChannels(
            std::string_view channelUuid,
            std::string_view channelName,
            std::string_view channelDescription);

        static void printChannelThreads(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t threadTimestamp,
            std::string_view threadTitle,
            std::string_view threadBody);

        static void printThreadReplies(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t replyTimestamp,
            std::string_view replyBody);

        static void printPrivateMessages(
            std::string_view senderUuid,
            std::time_t messageTimestamp,
            std::string_view messageBody);

        static void printUser(
            std::string_view userUuid,
            std::string_view userName,
            int userStatus);

        static void printTeam(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view teamDescription);

        static void printChannel(
            std::string_view channelUuid,
            std::string_view channelName,
            std::string_view channelDescription);

        static void printThread(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t threadTimestamp,
            std::string_view threadTitle,
            std::string_view threadBody);

        static void printTeamCreated(
            std::string_view teamUuid,
            std::string_view teamName,
            std::string_view teamDescription);

        static void printChannelCreated(
            std::string_view channelUuid,
            std::string_view channelName,
            std::string_view channelDescription);

        static void printThreadCreated(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t threadTimestamp,
            std::string_view threadTitle,
            std::string_view threadBody);

        static void printReplyCreated(
            std::string_view threadUuid,
            std::string_view userUuid,
            std::time_t replyTimestamp,
            std::string_view replyBody);

        static void printSubscribed(
            std::string_view userUuid,
            std::string_view teamUuid);

        static void printUnsubscribed(
            std::string_view userUuid,
            std::string_view teamUuid);

        // Error responses.
        static void errorUnknownTeam(std::string_view teamUuid);
        static void errorUnknownChannel(std::string_view channelUuid);
        static void errorUnknownThread(std::string_view threadUuid);
        static void errorUnknownUser(std::string_view userUuid);
        static void errorUnauthorized();
        static void errorAlreadyExist();
};

#endif // CLIENT_DISPLAY_PRINTER_HPP
