#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "limits.hpp"

namespace myteams {

    enum CommandID : std::uint16_t
    {
        CMD_LOGIN           = 10,
        CMD_LOGOUT          = 11,
        CMD_USERS           = 20,
        CMD_USER_INFO       = 21,
        CMD_SEND            = 30,
        CMD_MESSAGES        = 31,
        CMD_SUBSCRIBE       = 40,
        CMD_UNSUBSCRIBE     = 41,
        CMD_SUBSCRIBED_LIST = 42,
        CMD_USE             = 50,
        CMD_CREATE          = 60,
        CMD_LIST            = 70,
        CMD_INFO            = 80
    };

    enum StatusCode : std::uint16_t
    {
        RPL_OK                  = 200,
        RPL_CREATED             = 201,
        RPL_USERS_LIST          = 202,
        RPL_USER_INFO           = 203,
        RPL_MESSAGES_LIST       = 204,
        RPL_TEAMS_LIST          = 205,
        RPL_CHANNELS_LIST       = 206,
        RPL_THREADS_LIST        = 207,
        RPL_REPLIES_LIST        = 208,
        RPL_SUBSCRIBED_LIST     = 209,

        ERR_BAD_REQUEST         = 400,
        ERR_UNAUTHORIZED        = 401,
        ERR_FORBIDDEN           = 403,
        ERR_NOT_FOUND           = 404,
        ERR_ALREADY_EXIST       = 409,

        ERR_SERVER_INTERNAL     = 500,

        EVT_LOGGED_IN           = 600,
        EVT_LOGGED_OUT          = 601,
        EVT_PRIVATE_MSG_RCVD    = 602,
        EVT_TEAM_CREATED        = 603,
        EVT_CHANNEL_CREATED     = 604,
        EVT_THREAD_CREATED      = 605,
        EVT_REPLY_CREATED       = 606
    };

    // le __attribute__((packed)) c'est utilisé en réseau pour éviter que
    // le compilateur ajoute des bites de padding qu'on voit pas
    struct __attribute__((packed))
    PacketHeader
    {
        std::uint16_t code;
        std::uint16_t payload_size;
    };


    //cli -> serv
    struct __attribute__((packed))
    PayloadReqLogin
    {
        char user_name[MAX_NAME_LENGTH];
    };

    //cli -> serv
    struct __attribute__((packed))
    PayloadReqTargetUser
    {
        char target_uuid[UUID_LENGTH];
    };

    //cli -> serv
    struct __attribute__((packed))
    PayloadReqSendMsg
    {
        char target_uuid[UUID_LENGTH];
        char message_body[MAX_BODY_LENGTH];
    };

    //cli -> serv
    struct __attribute__((packed))
    PayloadReqTeamTarget
    {
        char team_uuid[UUID_LENGTH];
    };

    //cli -> serv
    struct __attribute__((packed))
    PayloadReqCreateTeam
    {
        char team_name[MAX_NAME_LENGTH];
        char team_description[MAX_DESCRIPTION_LENGTH];
    };

    //cli -> serv
    struct __attribute__((packed))
    PayloadReqCreateChannel
    {
        char channel_name[MAX_NAME_LENGTH];
        char channel_description[MAX_DESCRIPTION_LENGTH];
    };

    //cli -> serv
    struct __attribute__((packed))
    PayloadReqCreateThread
    {
        char thread_title[MAX_NAME_LENGTH];
        char thread_body[MAX_BODY_LENGTH];
    };

    //cli -> serv
    struct __attribute__((packed))
    PayloadReqCreateReply
    {
        char reply_body[MAX_BODY_LENGTH];
    };


    //serv -> cli
    struct __attribute__((packed))
    PayloadRplUser
    {
        char user_uuid[UUID_LENGTH];
        char user_name[MAX_NAME_LENGTH];
        std::uint32_t user_status;
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadRplMessage
    {
        char sender_uuid[UUID_LENGTH];
        std::uint64_t message_timestamp;
        char message_body[MAX_BODY_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadRplTeam
    {
        char team_uuid[UUID_LENGTH];
        char team_name[MAX_NAME_LENGTH];
        char team_description[MAX_DESCRIPTION_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadRplChannel
    {
        char channel_uuid[UUID_LENGTH];
        char channel_name[MAX_NAME_LENGTH];
        char channel_description[MAX_DESCRIPTION_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadRplThread
    {
        char thread_uuid[UUID_LENGTH];
        char user_uuid[UUID_LENGTH];
        std::uint64_t thread_timestamp;
        char thread_title[MAX_NAME_LENGTH];
        char thread_body[MAX_BODY_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadRplReply
    {
        char thread_uuid[UUID_LENGTH];
        char user_uuid[UUID_LENGTH];
        std::uint64_t reply_timestamp;
        char reply_body[MAX_BODY_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadEvtUserConnection
    {
        char user_uuid[UUID_LENGTH];
        char user_name[MAX_NAME_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadEvtPrivateMsg
    {
        char sender_uuid[UUID_LENGTH];
        char message_body[MAX_BODY_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadEvtTeamCreated
    {
        char team_uuid[UUID_LENGTH];
        char team_name[MAX_NAME_LENGTH];
        char team_description[MAX_DESCRIPTION_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadEvtChannelCreated
    {
        char channel_uuid[UUID_LENGTH];
        char channel_name[MAX_NAME_LENGTH];
        char channel_description[MAX_DESCRIPTION_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadEvtThreadCreated
    {
        char thread_uuid[UUID_LENGTH];
        char user_uuid[UUID_LENGTH];
        std::uint64_t thread_timestamp;
        char thread_title[MAX_NAME_LENGTH];
        char thread_body[MAX_BODY_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadEvtReplyCreated
    {
        char team_uuid[UUID_LENGTH];
        char thread_uuid[UUID_LENGTH];
        char user_uuid[UUID_LENGTH];
        char reply_body[MAX_BODY_LENGTH];
    };

    //serv -> cli
    struct __attribute__((packed))
    PayloadErrUnknown
    {
        char unknown_uuid[UUID_LENGTH];
    };

} //namespace myteams

#endif //PROTOCOL_HPP
