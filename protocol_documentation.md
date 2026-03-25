```
Network Working Group                                     MyTeams Project
Request for Comments: XXXX                                    March 2025
Category: Informational


                 MyTeams Binary Communication Protocol (MTCP)
                            Specification v1.0
```

---

## Status of This Memo

This document defines the MyTeams Communication Protocol (MTCP), a binary
client/server protocol for a team messaging application. Distribution of
this memo is unlimited.

---

## Abstract

This document specifies the MyTeams Communication Protocol (MTCP), a
binary, length-prefixed protocol used for communication between MyTeams
clients and servers. It defines the packet framing, command identifiers,
status/event codes, and all associated payload structures.

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Conventions and Terminology](#2-conventions-and-terminology)
3. [Packet Framing](#3-packet-framing)
4. [Command Codes (Client → Server)](#4-command-codes-client--server)
5. [Status Codes (Server → Client)](#5-status-codes-server--client)
6. [Payload Structures](#6-payload-structures)
   - 6.1 [Client Request Payloads](#61-client-request-payloads)
   - 6.2 [Server Reply Payloads](#62-server-reply-payloads)
   - 6.3 [Server Event Payloads](#63-server-event-payloads)
   - 6.4 [Error Payloads](#64-error-payloads)
7. [Command/Response Reference](#7-commandresponse-reference)
8. [Constants and Limits](#8-constants-and-limits)
9. [Security Considerations](#9-security-considerations)

---

## 1. Introduction

MTCP is a compact binary protocol operating over a reliable, ordered
stream transport (e.g., TCP). All messages consist of a fixed-size header
followed by a variable-length payload. The protocol supports user
authentication, private messaging, team/channel/thread management, and
real-time event notification.

All structures use `__attribute__((packed))` to suppress compiler padding,
ensuring a consistent wire format across platforms and compilers.

---

## 2. Conventions and Terminology

The key words "MUST", "MUST NOT", "REQUIRED", "SHOULD", "MAY" in this
document are to be interpreted as described in RFC 2119.

**Definitions:**

| Term | Definition |
|---|---|
| Client | An endpoint initiating commands to the server |
| Server | The central entity processing commands and dispatching replies/events |
| UUID | A fixed-length identifier of `UUID_LENGTH` bytes |
| Command | A message sent from client to server |
| Reply | A message sent from server to client in response to a command |
| Event | An unsolicited message sent from server to client |

All integer fields are **unsigned 16-bit** (`uint16_t`) in network byte
order unless otherwise noted. String fields are **null-padded** fixed-length
byte arrays.

---

## 3. Packet Framing

Every MTCP message begins with a **PacketHeader**, immediately followed by
a payload of `payload_size` bytes.

### 3.1 PacketHeader

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|             code              |          payload_size         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field | Type | Size | Description |
|---|---|---|---|
| `code` | `uint16_t` | 2 bytes | Command ID or Status Code |
| `payload_size` | `uint16_t` | 2 bytes | Length of the following payload in bytes |

- The `code` field contains either a **CommandID** (client → server) or a
  **StatusCode** (server → client).
- A `payload_size` of `0` indicates a header-only packet (no payload follows).
- Receivers MUST read exactly `payload_size` bytes after the header before
  processing the next message.

---

## 4. Command Codes (Client → Server)

Commands are sent by the client to request an action from the server.

| Name | Code | Description |
|---|---|---|
| `CMD_LOGIN` | `10` | Authenticate with a username |
| `CMD_LOGOUT` | `11` | End the current session |
| `CMD_USERS` | `20` | List all known users |
| `CMD_USER_INFO` | `21` | Get information about a specific user |
| `CMD_SEND` | `30` | Send a private message to a user |
| `CMD_MESSAGES` | `31` | List messages with a specific user |
| `CMD_SUBSCRIBE` | `40` | Subscribe to a team |
| `CMD_UNSUBSCRIBE` | `41` | Unsubscribe from a team |
| `CMD_SUBSCRIBED_LIST` | `42` | List teams the current user is subscribed to |
| `CMD_USE` | `50` | Set the active team/channel/thread context |
| `CMD_CREATE` | `60` | Create a team, channel, thread, or reply |
| `CMD_LIST` | `70` | List teams, channels, or threads |
| `CMD_INFO` | `80` | Get info about the current context |

---

## 5. Status Codes (Server → Client)

Status codes are used in reply and event packets from the server.

### 5.1 Success Codes (2xx)

| Name | Code | Description |
|---|---|---|
| `RPL_OK` | `200` | Generic success |
| `RPL_CREATED` | `201` | Resource successfully created |
| `RPL_USERS_LIST` | `202` | Payload contains a user entry (repeated) |
| `RPL_USER_INFO` | `203` | Payload contains detailed user info |
| `RPL_MESSAGES_LIST` | `204` | Payload contains a message entry (repeated) |
| `RPL_TEAMS_LIST` | `205` | Payload contains a team entry (repeated) |
| `RPL_CHANNELS_LIST` | `206` | Payload contains a channel entry (repeated) |
| `RPL_THREADS_LIST` | `207` | Payload contains a thread entry (repeated) |
| `RPL_REPLIES_LIST` | `208` | Payload contains a reply entry (repeated) |
| `RPL_SUBSCRIBED_LIST` | `209` | Payload contains a subscribed team entry (repeated) |

> **Note on list replies:** For commands returning multiple items, the
> server MUST send one packet per item, all with the same status code,
> followed by a final `RPL_OK` packet with `payload_size = 0` to signal
> end of list.

### 5.2 Error Codes (4xx / 5xx)

| Name | Code | Description |
|---|---|---|
| `ERR_BAD_REQUEST` | `400` | Malformed command or invalid parameters |
| `ERR_UNAUTHORIZED` | `401` | Action requires authentication |
| `ERR_FORBIDDEN` | `403` | Authenticated user lacks permission |
| `ERR_NOT_FOUND` | `404` | Referenced resource does not exist |
| `ERR_ALREADY_EXIST` | `409` | Resource already exists |
| `ERR_SERVER_INTERNAL` | `500` | Unexpected server-side error |

### 5.3 Event Codes (6xx)

Events are sent asynchronously by the server to notify connected clients.

| Name | Code | Description |
|---|---|---|
| `EVT_LOGGED_IN` | `600` | A user has connected |
| `EVT_LOGGED_OUT` | `601` | A user has disconnected |
| `EVT_PRIVATE_MSG_RCVD` | `602` | A private message was received |
| `EVT_TEAM_CREATED` | `603` | A new team was created |
| `EVT_CHANNEL_CREATED` | `604` | A new channel was created |
| `EVT_THREAD_CREATED` | `605` | A new thread was created |
| `EVT_REPLY_CREATED` | `606` | A new reply was posted |

---

## 6. Payload Structures

All structures are packed (no padding). String fields are **null-padded**
fixed-length arrays. Integer fields use the host platform's native
representation for `int` and `time_t` — implementations MUST agree on
endianness and sizes for cross-platform compatibility.

### 6.1 Client Request Payloads

#### 6.1.1 PayloadReqLogin
*Used with:* `CMD_LOGIN`

| Field | Type | Description |
|---|---|---|
| `user_name` | `char[MAX_NAME_LENGTH]` | The username to log in as |

#### 6.1.2 PayloadReqTargetUser
*Used with:* `CMD_USER_INFO`, `CMD_SEND`, `CMD_MESSAGES`

| Field | Type | Description |
|---|---|---|
| `target_uuid` | `char[UUID_LENGTH]` | UUID of the target user |

#### 6.1.3 PayloadReqSendMsg
*Used with:* `CMD_SEND`

| Field | Type | Description |
|---|---|---|
| `target_uuid` | `char[UUID_LENGTH]` | UUID of the recipient |
| `message_body` | `char[MAX_BODY_LENGTH]` | Content of the private message |

#### 6.1.4 PayloadReqTeamTarget
*Used with:* `CMD_SUBSCRIBE`, `CMD_UNSUBSCRIBE`, `CMD_USE`

| Field | Type | Description |
|---|---|---|
| `team_uuid` | `char[UUID_LENGTH]` | UUID of the target team |

#### 6.1.5 PayloadReqCreateTeam
*Used with:* `CMD_CREATE` (team context)

| Field | Type | Description |
|---|---|---|
| `team_name` | `char[MAX_NAME_LENGTH]` | Name of the team to create |
| `team_description` | `char[MAX_DESCRIPTION_LENGTH]` | Description of the team |

#### 6.1.6 PayloadReqCreateChannel
*Used with:* `CMD_CREATE` (channel context)

| Field | Type | Description |
|---|---|---|
| `channel_name` | `char[MAX_NAME_LENGTH]` | Name of the channel to create |
| `channel_description` | `char[MAX_DESCRIPTION_LENGTH]` | Description of the channel |

#### 6.1.7 PayloadReqCreateThread
*Used with:* `CMD_CREATE` (thread context)

| Field | Type | Description |
|---|---|---|
| `thread_title` | `char[MAX_NAME_LENGTH]` | Title of the thread |
| `thread_body` | `char[MAX_BODY_LENGTH]` | Body/content of the opening message |

#### 6.1.8 PayloadReqCreateReply
*Used with:* `CMD_CREATE` (reply context)

| Field | Type | Description |
|---|---|---|
| `reply_body` | `char[MAX_BODY_LENGTH]` | Content of the reply |

---

### 6.2 Server Reply Payloads

#### 6.2.1 PayloadRplUser
*Used with:* `RPL_USERS_LIST`, `RPL_USER_INFO`

| Field | Type | Description |
|---|---|---|
| `user_uuid` | `char[UUID_LENGTH]` | Unique identifier of the user |
| `user_name` | `char[MAX_NAME_LENGTH]` | Display name of the user |
| `user_status` | `int` | Connection status (`0` = offline, non-zero = online) |

#### 6.2.2 PayloadRplMessage
*Used with:* `RPL_MESSAGES_LIST`

| Field | Type | Description |
|---|---|---|
| `sender_uuid` | `char[UUID_LENGTH]` | UUID of the message sender |
| `message_timestamp` | `time_t` | UNIX timestamp of the message |
| `message_body` | `char[MAX_BODY_LENGTH]` | Content of the message |

#### 6.2.3 PayloadRplTeam
*Used with:* `RPL_TEAMS_LIST`, `RPL_SUBSCRIBED_LIST`

| Field | Type | Description |
|---|---|---|
| `team_uuid` | `char[UUID_LENGTH]` | Unique identifier of the team |
| `team_name` | `char[MAX_NAME_LENGTH]` | Name of the team |
| `team_description` | `char[MAX_DESCRIPTION_LENGTH]` | Description of the team |

#### 6.2.4 PayloadRplChannel
*Used with:* `RPL_CHANNELS_LIST`

| Field | Type | Description |
|---|---|---|
| `channel_uuid` | `char[UUID_LENGTH]` | Unique identifier of the channel |
| `channel_name` | `char[MAX_NAME_LENGTH]` | Name of the channel |
| `channel_description` | `char[MAX_DESCRIPTION_LENGTH]` | Description of the channel |

#### 6.2.5 PayloadRplThread
*Used with:* `RPL_THREADS_LIST`

| Field | Type | Description |
|---|---|---|
| `thread_uuid` | `char[UUID_LENGTH]` | Unique identifier of the thread |
| `user_uuid` | `char[UUID_LENGTH]` | UUID of the thread author |
| `thread_timestamp` | `time_t` | UNIX timestamp of thread creation |
| `thread_title` | `char[MAX_NAME_LENGTH]` | Title of the thread |
| `thread_body` | `char[MAX_BODY_LENGTH]` | Opening message body |

#### 6.2.6 PayloadRplReply
*Used with:* `RPL_REPLIES_LIST`

| Field | Type | Description |
|---|---|---|
| `thread_uuid` | `char[UUID_LENGTH]` | UUID of the parent thread |
| `user_uuid` | `char[UUID_LENGTH]` | UUID of the reply author |
| `reply_timestamp` | `time_t` | UNIX timestamp of the reply |
| `reply_body` | `char[MAX_BODY_LENGTH]` | Content of the reply |

---

### 6.3 Server Event Payloads

#### 6.3.1 PayloadEvtUserConnection
*Used with:* `EVT_LOGGED_IN`, `EVT_LOGGED_OUT`

| Field | Type | Description |
|---|---|---|
| `user_uuid` | `char[UUID_LENGTH]` | UUID of the connecting/disconnecting user |
| `user_name` | `char[MAX_NAME_LENGTH]` | Name of the user |

#### 6.3.2 PayloadEvtPrivateMsg
*Used with:* `EVT_PRIVATE_MSG_RCVD`

| Field | Type | Description |
|---|---|---|
| `sender_uuid` | `char[UUID_LENGTH]` | UUID of the message sender |
| `message_body` | `char[MAX_BODY_LENGTH]` | Content of the private message |

#### 6.3.3 PayloadEvtTeamCreated
*Used with:* `EVT_TEAM_CREATED`

| Field | Type | Description |
|---|---|---|
| `team_uuid` | `char[UUID_LENGTH]` | UUID of the newly created team |
| `team_name` | `char[MAX_NAME_LENGTH]` | Name of the team |
| `team_description` | `char[MAX_DESCRIPTION_LENGTH]` | Description of the team |

#### 6.3.4 PayloadEvtChannelCreated
*Used with:* `EVT_CHANNEL_CREATED`

| Field | Type | Description |
|---|---|---|
| `channel_uuid` | `char[UUID_LENGTH]` | UUID of the newly created channel |
| `channel_name` | `char[MAX_NAME_LENGTH]` | Name of the channel |
| `channel_description` | `char[MAX_DESCRIPTION_LENGTH]` | Description of the channel |

#### 6.3.5 PayloadEvtThreadCreated
*Used with:* `EVT_THREAD_CREATED`

| Field | Type | Description |
|---|---|---|
| `thread_uuid` | `char[UUID_LENGTH]` | UUID of the new thread |
| `user_uuid` | `char[UUID_LENGTH]` | UUID of the thread author |
| `thread_timestamp` | `time_t` | UNIX timestamp of creation |
| `thread_title` | `char[MAX_NAME_LENGTH]` | Title of the thread |
| `thread_body` | `char[MAX_BODY_LENGTH]` | Opening message body |

#### 6.3.6 PayloadEvtReplyCreated
*Used with:* `EVT_REPLY_CREATED`

| Field | Type | Description |
|---|---|---|
| `team_uuid` | `char[UUID_LENGTH]` | UUID of the team containing the thread |
| `thread_uuid` | `char[UUID_LENGTH]` | UUID of the parent thread |
| `user_uuid` | `char[UUID_LENGTH]` | UUID of the reply author |
| `reply_body` | `char[MAX_BODY_LENGTH]` | Content of the reply |

---

### 6.4 Error Payloads

#### 6.4.1 PayloadErrUnknown
*Used with:* `ERR_NOT_FOUND` (when the unknown entity has a UUID)

| Field | Type | Description |
|---|---|---|
| `unknown_uuid` | `char[UUID_LENGTH]` | UUID of the resource that was not found |

---

## 7. Command/Response Reference

This section summarizes the expected request payload and possible server
responses for each command.

| Command | Request Payload | Success Response(s) | Error Response(s) |
|---|---|---|---|
| `CMD_LOGIN` | `PayloadReqLogin` | `RPL_OK` | `ERR_ALREADY_EXIST`, `ERR_BAD_REQUEST` |
| `CMD_LOGOUT` | *(none)* | `RPL_OK` | `ERR_UNAUTHORIZED` |
| `CMD_USERS` | *(none)* | `RPL_USERS_LIST` × N, `RPL_OK` | `ERR_UNAUTHORIZED` |
| `CMD_USER_INFO` | `PayloadReqTargetUser` | `RPL_USER_INFO` | `ERR_NOT_FOUND`, `ERR_UNAUTHORIZED` |
| `CMD_SEND` | `PayloadReqSendMsg` | `RPL_OK` | `ERR_NOT_FOUND`, `ERR_UNAUTHORIZED` |
| `CMD_MESSAGES` | `PayloadReqTargetUser` | `RPL_MESSAGES_LIST` × N, `RPL_OK` | `ERR_NOT_FOUND`, `ERR_UNAUTHORIZED` |
| `CMD_SUBSCRIBE` | `PayloadReqTeamTarget` | `RPL_OK` | `ERR_NOT_FOUND`, `ERR_ALREADY_EXIST` |
| `CMD_UNSUBSCRIBE` | `PayloadReqTeamTarget` | `RPL_OK` | `ERR_NOT_FOUND`, `ERR_FORBIDDEN` |
| `CMD_SUBSCRIBED_LIST` | *(none)* | `RPL_SUBSCRIBED_LIST` × N, `RPL_OK` | `ERR_UNAUTHORIZED` |
| `CMD_USE` | `PayloadReqTeamTarget` | `RPL_OK` | `ERR_NOT_FOUND`, `ERR_FORBIDDEN` |
| `CMD_CREATE` (team) | `PayloadReqCreateTeam` | `RPL_CREATED` + `EVT_TEAM_CREATED` → all | `ERR_ALREADY_EXIST` |
| `CMD_CREATE` (channel) | `PayloadReqCreateChannel` | `RPL_CREATED` + `EVT_CHANNEL_CREATED` → subscribers | `ERR_ALREADY_EXIST`, `ERR_FORBIDDEN` |
| `CMD_CREATE` (thread) | `PayloadReqCreateThread` | `RPL_CREATED` + `EVT_THREAD_CREATED` → subscribers | `ERR_FORBIDDEN` |
| `CMD_CREATE` (reply) | `PayloadReqCreateReply` | `RPL_CREATED` + `EVT_REPLY_CREATED` → subscribers | `ERR_FORBIDDEN` |
| `CMD_LIST` (teams) | *(none)* | `RPL_TEAMS_LIST` × N, `RPL_OK` | `ERR_UNAUTHORIZED` |
| `CMD_LIST` (channels) | *(none)* | `RPL_CHANNELS_LIST` × N, `RPL_OK` | `ERR_FORBIDDEN` |
| `CMD_LIST` (threads) | *(none)* | `RPL_THREADS_LIST` × N, `RPL_OK` | `ERR_FORBIDDEN` |
| `CMD_INFO` | *(none)* | Context-dependent reply | `ERR_BAD_REQUEST` |

---

## 8. Constants and Limits

The following constants are defined in `limits.hpp` and govern all field
sizes in this protocol. Implementations MUST use identical values on both
client and server.

| Constant | Description |
|---|---|
| `UUID_LENGTH` | Fixed byte length of all UUID strings |
| `MAX_NAME_LENGTH` | Maximum length of name fields (teams, channels, users) |
| `MAX_DESCRIPTION_LENGTH` | Maximum length of description fields |
| `MAX_BODY_LENGTH` | Maximum length of message/thread/reply body fields |

> The exact numeric values of these constants are defined in `limits.hpp`
> and are not specified here to allow future revision without updating
> this document.

---

## 9. Security Considerations

- MTCP does not include built-in encryption. It is RECOMMENDED to run MTCP
  over TLS to protect message confidentiality and integrity.
- Authentication (`CMD_LOGIN`) relies solely on a username with no
  password or challenge-response mechanism. Deployments SHOULD add
  authentication at the transport layer.
- The server MUST validate `payload_size` before reading payload bytes to
  prevent buffer overflows.
- Fixed-length string fields MUST be null-terminated within the buffer;
  receivers MUST NOT assume null-termination beyond `payload_size`.

---
