#include "client/display/printer.hpp"

#include <stdexcept>
#include <string>
#include <string_view>

extern "C" {
    #include "../../libs/myteams/logging_client.h"
}

static std::string makeCString(const std::string_view value)
{
    return std::string(value);
}

static void ensureLibCallSucceeded(const int rawResult)
{
    if (rawResult < 0) {
        throw std::runtime_error(
            "libmyteams client call failed with return code " + std::to_string(rawResult));
    }
}

void Printer::eventLoggedIn(
    const std::string_view userUuid,
    const std::string_view userName)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    ensureLibCallSucceeded(client_event_logged_in(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str()));
}

void Printer::eventLoggedOut(
    const std::string_view userUuid,
    const std::string_view userName)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    ensureLibCallSucceeded(client_event_logged_out(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str()));
}

void Printer::eventPrivateMessageReceived(
    const std::string_view userUuid,
    const std::string_view messageBody)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string messageBodyBuffer = makeCString(messageBody);

    ensureLibCallSucceeded(client_event_private_message_received(
        userUuidBuffer.c_str(),
        messageBodyBuffer.c_str()));
}

void Printer::eventThreadReplyReceived(
    const std::string_view teamUuid,
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::string_view replyBody)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string replyBodyBuffer = makeCString(replyBody);

    ensureLibCallSucceeded(client_event_thread_reply_received(
        teamUuidBuffer.c_str(),
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        replyBodyBuffer.c_str()));
}

void Printer::eventTeamCreated(
    const std::string_view teamUuid,
    const std::string_view teamName,
    const std::string_view teamDescription)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string teamNameBuffer = makeCString(teamName);
    const std::string teamDescriptionBuffer = makeCString(teamDescription);

    ensureLibCallSucceeded(client_event_team_created(
        teamUuidBuffer.c_str(),
        teamNameBuffer.c_str(),
        teamDescriptionBuffer.c_str()));
}

void Printer::eventChannelCreated(
    const std::string_view channelUuid,
    const std::string_view channelName,
    const std::string_view channelDescription)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string channelNameBuffer = makeCString(channelName);
    const std::string channelDescriptionBuffer = makeCString(channelDescription);

    ensureLibCallSucceeded(client_event_channel_created(
        channelUuidBuffer.c_str(),
        channelNameBuffer.c_str(),
        channelDescriptionBuffer.c_str()));
}

void Printer::eventThreadCreated(
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

    ensureLibCallSucceeded(client_event_thread_created(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        threadTimestamp,
        threadTitleBuffer.c_str(),
        threadBodyBuffer.c_str()));
}

void Printer::printUsers(
    const std::string_view userUuid,
    const std::string_view userName,
    const int userStatus)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    ensureLibCallSucceeded(client_print_users(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str(),
        userStatus));
}

void Printer::printTeams(
    const std::string_view teamUuid,
    const std::string_view teamName,
    const std::string_view teamDescription)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string teamNameBuffer = makeCString(teamName);
    const std::string teamDescriptionBuffer = makeCString(teamDescription);

    ensureLibCallSucceeded(client_print_teams(
        teamUuidBuffer.c_str(),
        teamNameBuffer.c_str(),
        teamDescriptionBuffer.c_str()));
}

void Printer::printTeamChannels(
    const std::string_view channelUuid,
    const std::string_view channelName,
    const std::string_view channelDescription)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string channelNameBuffer = makeCString(channelName);
    const std::string channelDescriptionBuffer = makeCString(channelDescription);

    ensureLibCallSucceeded(client_team_print_channels(
        channelUuidBuffer.c_str(),
        channelNameBuffer.c_str(),
        channelDescriptionBuffer.c_str()));
}

void Printer::printChannelThreads(
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

    ensureLibCallSucceeded(client_channel_print_threads(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        threadTimestamp,
        threadTitleBuffer.c_str(),
        threadBodyBuffer.c_str()));
}

void Printer::printThreadReplies(
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::time_t replyTimestamp,
    const std::string_view replyBody)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string replyBodyBuffer = makeCString(replyBody);

    ensureLibCallSucceeded(client_thread_print_replies(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        replyTimestamp,
        replyBodyBuffer.c_str()));
}

void Printer::printPrivateMessages(
    const std::string_view senderUuid,
    const std::time_t messageTimestamp,
    const std::string_view messageBody)
{
    const std::string senderUuidBuffer = makeCString(senderUuid);
    const std::string messageBodyBuffer = makeCString(messageBody);

    ensureLibCallSucceeded(client_private_message_print_messages(
        senderUuidBuffer.c_str(),
        messageTimestamp,
        messageBodyBuffer.c_str()));
}

void Printer::printUser(
    const std::string_view userUuid,
    const std::string_view userName,
    const int userStatus)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string userNameBuffer = makeCString(userName);

    ensureLibCallSucceeded(client_print_user(
        userUuidBuffer.c_str(),
        userNameBuffer.c_str(),
        userStatus));
}

void Printer::printTeam(
    const std::string_view teamUuid,
    const std::string_view teamName,
    const std::string_view teamDescription)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string teamNameBuffer = makeCString(teamName);
    const std::string teamDescriptionBuffer = makeCString(teamDescription);

    ensureLibCallSucceeded(client_print_team(
        teamUuidBuffer.c_str(),
        teamNameBuffer.c_str(),
        teamDescriptionBuffer.c_str()));
}

void Printer::printChannel(
    const std::string_view channelUuid,
    const std::string_view channelName,
    const std::string_view channelDescription)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string channelNameBuffer = makeCString(channelName);
    const std::string channelDescriptionBuffer = makeCString(channelDescription);

    ensureLibCallSucceeded(client_print_channel(
        channelUuidBuffer.c_str(),
        channelNameBuffer.c_str(),
        channelDescriptionBuffer.c_str()));
}

void Printer::printThread(
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

    ensureLibCallSucceeded(client_print_thread(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        threadTimestamp,
        threadTitleBuffer.c_str(),
        threadBodyBuffer.c_str()));
}

void Printer::printTeamCreated(
    const std::string_view teamUuid,
    const std::string_view teamName,
    const std::string_view teamDescription)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);
    const std::string teamNameBuffer = makeCString(teamName);
    const std::string teamDescriptionBuffer = makeCString(teamDescription);

    ensureLibCallSucceeded(client_print_team_created(
        teamUuidBuffer.c_str(),
        teamNameBuffer.c_str(),
        teamDescriptionBuffer.c_str()));
}

void Printer::printChannelCreated(
    const std::string_view channelUuid,
    const std::string_view channelName,
    const std::string_view channelDescription)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);
    const std::string channelNameBuffer = makeCString(channelName);
    const std::string channelDescriptionBuffer = makeCString(channelDescription);

    ensureLibCallSucceeded(client_print_channel_created(
        channelUuidBuffer.c_str(),
        channelNameBuffer.c_str(),
        channelDescriptionBuffer.c_str()));
}

void Printer::printThreadCreated(
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

    ensureLibCallSucceeded(client_print_thread_created(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        threadTimestamp,
        threadTitleBuffer.c_str(),
        threadBodyBuffer.c_str()));
}

void Printer::printReplyCreated(
    const std::string_view threadUuid,
    const std::string_view userUuid,
    const std::time_t replyTimestamp,
    const std::string_view replyBody)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string replyBodyBuffer = makeCString(replyBody);

    ensureLibCallSucceeded(client_print_reply_created(
        threadUuidBuffer.c_str(),
        userUuidBuffer.c_str(),
        replyTimestamp,
        replyBodyBuffer.c_str()));
}

void Printer::printSubscribed(
    const std::string_view userUuid,
    const std::string_view teamUuid)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string teamUuidBuffer = makeCString(teamUuid);

    ensureLibCallSucceeded(client_print_subscribed(
        userUuidBuffer.c_str(),
        teamUuidBuffer.c_str()));
}

void Printer::printUnsubscribed(
    const std::string_view userUuid,
    const std::string_view teamUuid)
{
    const std::string userUuidBuffer = makeCString(userUuid);
    const std::string teamUuidBuffer = makeCString(teamUuid);

    ensureLibCallSucceeded(client_print_unsubscribed(
        userUuidBuffer.c_str(),
        teamUuidBuffer.c_str()));
}

void Printer::errorUnknownTeam(const std::string_view teamUuid)
{
    const std::string teamUuidBuffer = makeCString(teamUuid);

    ensureLibCallSucceeded(client_error_unknown_team(teamUuidBuffer.c_str()));
}

void Printer::errorUnknownChannel(const std::string_view channelUuid)
{
    const std::string channelUuidBuffer = makeCString(channelUuid);

    ensureLibCallSucceeded(client_error_unknown_channel(channelUuidBuffer.c_str()));
}

void Printer::errorUnknownThread(const std::string_view threadUuid)
{
    const std::string threadUuidBuffer = makeCString(threadUuid);

    ensureLibCallSucceeded(client_error_unknown_thread(threadUuidBuffer.c_str()));
}

void Printer::errorUnknownUser(const std::string_view userUuid)
{
    const std::string userUuidBuffer = makeCString(userUuid);

    ensureLibCallSucceeded(client_error_unknown_user(userUuidBuffer.c_str()));
}

void Printer::errorUnauthorized()
{
    ensureLibCallSucceeded(client_error_unauthorized());
}

void Printer::errorAlreadyExist()
{
    ensureLibCallSucceeded(client_error_already_exist());
}
