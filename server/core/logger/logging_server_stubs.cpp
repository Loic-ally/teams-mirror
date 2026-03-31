#if defined(__APPLE__)

extern "C" {

#include "../../../libs/myteams/logging_server.h"

int server_event_team_created(char const *, char const *, char const *)
{
    return 0;
}

int server_event_channel_created(char const *, char const *, char const *)
{
    return 0;
}

int server_event_thread_created(char const *, char const *, char const *, char const *, char const *)
{
    return 0;
}

int server_event_reply_created(char const *, char const *, char const *)
{
    return 0;
}

int server_event_user_subscribed(char const *, char const *)
{
    return 0;
}

int server_event_user_unsubscribed(char const *, char const *)
{
    return 0;
}

int server_event_user_created(char const *, char const *)
{
    return 0;
}

int server_event_user_loaded(char const *, char const *)
{
    return 0;
}

int server_event_user_logged_in(char const *)
{
    return 0;
}

int server_event_user_logged_out(char const *)
{
    return 0;
}

int server_event_private_message_sended(char const *, char const *, char const *)
{
    return 0;
}

} // extern "C"

#endif // defined(__APPLE__)
