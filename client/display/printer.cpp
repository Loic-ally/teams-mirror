#include "client/display/printer.hpp"

#include <string>
#include <string_view>

extern "C" {
    #include "../../libs/myteams/logging_client.h"
}

namespace {

std::string makeCString(const std::string_view value)
{
    return std::string(value);
}

} // namespace

int Printer::eventLoggedIn(
    const std::string_view userUuid,
    const std::string_view userName)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    return client_event_logged_in(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str());
}

int Printer::eventLoggedOut(
    const std::string_view userUuid,
    const std::string_view userName)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    return client_event_logged_out(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str());
}

int Printer::eventPrivateMessageReceived(
    const std::string_view userUuid,
    const std::string_view messageBody)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string messageBodyBuffer = makeCString(messageBody);

    return client_event_private_message_received(
        userUuidBuffer.c_str(),
        messageBodyBuffer.c_str());
}

int Printer::eventThreadReplyReceived(
    const std::string_view teamUuid,
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::string_view replyBody)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string replyBodyBuffer = makeCString(replyBody);

    return client_event_thread_reply_received(
        teamUuidBuffer.c_str(),
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        replyBodyBuffer.c_str());
}

int Printer::eventTeamCreated(
    const std::string_view teamUuid,
    const std::string_view teamName,
    const std::string_view teamDescription)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string teamNameBuffer = makeCString(teamName);
    const std::string teamDescriptionBuffer = makeCString(teamDescription);

    return client_event_team_created(
        teamUuidBuffer.c_str(),
        teamNameBuffer.c_str(),
        teamDescriptionBuffer.c_str());
}

int Printer::eventChannelCreated(
    const std::string_view channelUuid,
    const std::string_view channelName,
    const std::string_view channelDescription)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string channelNameBuffer = makeCString(channelName);
    const std::string channelDescriptionBuffer = makeCString(channelDescription);

    return client_event_channel_created(
        channelUuidBuffer.c_str(),
        channelNameBuffer.c_str(),
        channelDescriptionBuffer.c_str());
}

int Printer::eventThreadCreated(
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::time_t threadTimestamp,
    const std::string_view threadTitle,
    const std::string_view threadBody)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string threadTitleBuffer = makeCString(threadTitle);
    const std::string threadBodyBuffer = makeCString(threadBody);

    return client_event_thread_created(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        threadTimestamp,
        threadTitleBuffer.c_str(),
        threadBodyBuffer.c_str());
}

int Printer::printUsers(
    const std::string_view userUuid,
    const std::string_view userName,
    const int userStatus)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    return client_print_users(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str(),
        userStatus);
}

int Printer::printTeams(
    const std::string_view teamUuid,
    const std::string_view teamName,
    const std::string_view teamDescription)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string teamNameBuffer = makeCString(teamName);
    const std::string teamDescriptionBuffer = makeCString(teamDescription);

    return client_print_teams(
        teamUuidBuffer.c_str(),
        teamNameBuffer.c_str(),
        teamDescriptionBuffer.c_str());
}

int Printer::printTeamChannels(
    const std::string_view channelUuid,
    const std::string_view channelName,
    const std::string_view channelDescription)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string channelNameBuffer = makeCString(channelName);
    const std::string channelDescriptionBuffer = makeCString(channelDescription);

    return client_team_print_channels(
        channelUuidBuffer.c_str(),
        channelNameBuffer.c_str(),
        channelDescriptionBuffer.c_str());
}

int Printer::printChannelThreads(
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::time_t threadTimestamp,
    const std::string_view threadTitle,
    const std::string_view threadBody)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string threadTitleBuffer = makeCString(threadTitle);
    const std::string threadBodyBuffer = makeCString(threadBody);

    return client_channel_print_threads(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        threadTimestamp,
        threadTitleBuffer.c_str(),
        threadBodyBuffer.c_str());
}

int Printer::printThreadReplies(
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::time_t replyTimestamp,
    const std::string_view replyBody)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string replyBodyBuffer = makeCString(replyBody);

    return client_thread_print_replies(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        replyTimestamp,
        replyBodyBuffer.c_str());
}

int Printer::printPrivateMessages(
    const std::string_view senderUuid,
    const std::time_t messageTimestamp,
    const std::string_view messageBody)
{
    const std::string senderUuidBuffer = makeCString(senderUuid);
    const std::string messageBodyBuffer = makeCString(messageBody);

    return client_private_message_print_messages(
        senderUuidBuffer.c_str(),
        messageTimestamp,
        messageBodyBuffer.c_str());
}

int Printer::printUser(
    const std::string_view userUuid,
    const std::string_view userName,
    const int userStatus)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    return client_print_user(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str(),
        userStatus);
}

int Printer::printTeam(
    const std::string_view teamUuid,
    const std::string_view teamName,
    const std::string_view teamDescription)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string teamNameBuffer = makeCString(teamName);
    const std::string teamDescriptionBuffer = makeCString(teamDescription);

    return client_print_team(
        teamUuidBuffer.c_str(),
        teamNameBuffer.c_str(),
        teamDescriptionBuffer.c_str());
}

int Printer::printChannel(
    const std::string_view channelUuid,
    const std::string_view channelName,
    const std::string_view channelDescription)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string channelNameBuffer = makeCString(channelName);
    const std::string channelDescriptionBuffer = makeCString(channelDescription);

    return client_print_channel(
        channelUuidBuffer.c_str(),
        channelNameBuffer.c_str(),
        channelDescriptionBuffer.c_str());
}

int Printer::printThread(
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::time_t threadTimestamp,
    const std::string_view threadTitle,
    const std::string_view threadBody)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string threadTitleBuffer = makeCString(threadTitle);
    const std::string threadBodyBuffer = makeCString(threadBody);

    return client_print_thread(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        threadTimestamp,
        threadTitleBuffer.c_str(),
        threadBodyBuffer.c_str());
}

int Printer::printTeamCreated(
    const std::string_view teamUuid,
    const std::string_view teamName,
    const std::string_view teamDescription)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string teamNameBuffer = makeCString(teamName);
    const std::string teamDescriptionBuffer = makeCString(teamDescription);

    return client_print_team_created(
        teamUuidBuffer.c_str(),
        teamNameBuffer.c_str(),
        teamDescriptionBuffer.c_str());
}

int Printer::printChannelCreated(
    const std::string_view channelUuid,
    const std::string_view channelName,
    const std::string_view channelDescription)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string channelNameBuffer = makeCString(channelName);
    const std::string channelDescriptionBuffer = makeCString(channelDescription);

    return client_print_channel_created(
        channelUuidBuffer.c_str(),
        channelNameBuffer.c_str(),
        channelDescriptionBuffer.c_str());
}

int Printer::printThreadCreated(
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::time_t threadTimestamp,
    const std::string_view threadTitle,
    const std::string_view threadBody)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string threadTitleBuffer = makeCString(threadTitle);
    const std::string threadBodyBuffer = makeCString(threadBody);

    return client_print_thread_created(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        threadTimestamp,
        threadTitleBuffer.c_str(),
        threadBodyBuffer.c_str());
}

int Printer::printReplyCreated(
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::time_t replyTimestamp,
    const std::string_view replyBody)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string replyBodyBuffer = makeCString(replyBody);

    return client_print_reply_created(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        replyTimestamp,
        replyBodyBuffer.c_str());
}

int Printer::printSubscribed(
    const std::string_view userUuid,
    const std::string_view teamUuid)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string teamUuidBuffer = makeCString(teamUuid);

    return client_print_subscribed(
        userUuidBuffer.c_str(),
        teamUuidBuffer.c_str());
}

int Printer::printUnsubscribed(
    const std::string_view userUuid,
    const std::string_view teamUuid)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string teamUuidBuffer = makeCString(teamUuid);

    return client_print_unsubscribed(
        userUuidBuffer.c_str(),
        teamUuidBuffer.c_str());
}

int Printer::errorUnknownTeam(const std::string_view teamUuid)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);

    return client_error_unknown_team(teamUuidBuffer.c_str());
}

int Printer::errorUnknownChannel(const std::string_view channelUuid)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);

    return client_error_unknown_channel(channelUuidBuffer.c_str());
}

int Printer::errorUnknownThread(const std::string_view threadUuid)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);

    return client_error_unknown_thread(threadUuidBuffer.c_str());
}

int Printer::errorUnknownUser(const std::string_view userUuid)
{
    const std::string userUuidBuffer = makeCString(userUuid);

    return client_error_unknown_user(userUuidBuffer.c_str());
}

int Printer::errorUnauthorized()
{
    return client_error_unauthorized();
}

int Printer::errorAlreadyExist()
{
    return client_error_already_exist();
}
