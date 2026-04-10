#!/usr/bin/env bash

set -u

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SERVER_BIN="${SERVER_BIN:-$ROOT_DIR/myteams_server}"
CLIENT_BIN="${CLIENT_BIN:-$ROOT_DIR/myteams_cli}"
HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-4242}"
USE_EXISTING_SERVER="${USE_EXISTING_SERVER:-0}"
STEP_DELAY="${STEP_DELAY:-0.25}"
WAIT_TIMEOUT="${WAIT_TIMEOUT:-6}"
SHUTDOWN_TIMEOUT="${SHUTDOWN_TIMEOUT:-2}"

RUN_ID="$(date +%Y%m%d_%H%M%S)"
LOG_DIR="$ROOT_DIR/tests/logs/e2e_$RUN_ID"
TMP_DIR="$ROOT_DIR/tests/.tmp_e2e_$RUN_ID"

mkdir -p "$LOG_DIR" "$TMP_DIR"

SERVER_LOG="$LOG_DIR/server.log"
A_LOG="$LOG_DIR/client_a.log"
B_LOG="$LOG_DIR/client_b.log"
SUMMARY_LOG="$LOG_DIR/summary.log"

FIFO_A="$TMP_DIR/client_a.in"
FIFO_B="$TMP_DIR/client_b.in"
mkfifo "$FIFO_A" "$FIFO_B"

PASS_COUNT=0
FAIL_COUNT=0

SERVER_PID=""
A_PID=""
B_PID=""
STARTED_SERVER=0

stop_pid() {
    local pid="$1"
    local timeout="$2"
    local start_ts now_ts

    if [[ -z "$pid" ]] || ! kill -0 "$pid" 2>/dev/null; then
        return 0
    fi

    kill -TERM "$pid" 2>/dev/null || true
    start_ts="$(date +%s)"
    while kill -0 "$pid" 2>/dev/null; do
        now_ts="$(date +%s)"
        if (( now_ts - start_ts >= timeout )); then
            kill -KILL "$pid" 2>/dev/null || true
            break
        fi
        sleep 0.1
    done
    wait "$pid" 2>/dev/null || true
}

cleanup() {
    exec 3>&- 2>/dev/null || true
    exec 4>&- 2>/dev/null || true

    stop_pid "$A_PID" "$SHUTDOWN_TIMEOUT"
    stop_pid "$B_PID" "$SHUTDOWN_TIMEOUT"
    if (( STARTED_SERVER == 1 )); then
        stop_pid "$SERVER_PID" "$SHUTDOWN_TIMEOUT"
    fi

    rm -rf "$TMP_DIR"
}

trap cleanup EXIT

say() {
    echo "$*" | tee -a "$SUMMARY_LOG"
}

pass() {
    PASS_COUNT=$((PASS_COUNT + 1))
    say "[PASS] $*"
}

fail() {
    FAIL_COUNT=$((FAIL_COUNT + 1))
    say "[FAIL] $*"
}

wait_for_regex() {
    local file="$1"
    local regex="$2"
    local timeout="$3"
    local start_ts now_ts
    start_ts="$(date +%s)"

    while true; do
        if grep -E -i -q "$regex" "$file" 2>/dev/null; then
            return 0
        fi
        now_ts="$(date +%s)"
        if (( now_ts - start_ts >= timeout )); then
            return 1
        fi
        sleep 0.1
    done
}

wait_for_regex_after() {
    local file="$1"
    local line_start="$2"
    local regex="$3"
    local timeout="$4"
    local start_ts now_ts slice

    start_ts="$(date +%s)"
    while true; do
        slice="$(tail -n +"$line_start" "$file" 2>/dev/null || true)"
        if [[ -n "$slice" ]] && printf '%s\n' "$slice" | grep -E -i -q "$regex"; then
            return 0
        fi
        now_ts="$(date +%s)"
        if (( now_ts - start_ts >= timeout )); then
            return 1
        fi
        sleep 0.1
    done
}

assert_contains() {
    local file="$1"
    local regex="$2"
    local label="$3"

    if wait_for_regex "$file" "$regex" "$WAIT_TIMEOUT"; then
        pass "$label"
    else
        fail "$label (regex not found: $regex)"
    fi
}

assert_not_contains() {
    local file="$1"
    local regex="$2"
    local label="$3"

    if grep -E -q "$regex" "$file" 2>/dev/null; then
        fail "$label (unexpected regex present: $regex)"
    else
        pass "$label"
    fi
}

assert_contains_after() {
    local file="$1"
    local line_start="$2"
    local regex="$3"
    local label="$4"

    if wait_for_regex_after "$file" "$line_start" "$regex" "$WAIT_TIMEOUT"; then
        pass "$label"
    else
        fail "$label (regex not found after line $line_start: $regex)"
    fi
}

assert_not_contains_after() {
    local file="$1"
    local line_start="$2"
    local regex="$3"
    local label="$4"
    local slice

    slice="$(tail -n +"$line_start" "$file" 2>/dev/null || true)"
    if [[ -n "$slice" ]] && printf '%s\n' "$slice" | grep -E -i -q "$regex"; then
        fail "$label (unexpected regex present after line $line_start: $regex)"
    else
        pass "$label"
    fi
}

send_a() {
    local cmd="$1"
    say ">> A $cmd"
    printf '%s\n' "$cmd" >&3
    sleep "$STEP_DELAY"
}

send_b() {
    local cmd="$1"
    say ">> B $cmd"
    printf '%s\n' "$cmd" >&4
    sleep "$STEP_DELAY"
}

extract_uuid_after() {
    local file="$1"
    local line_start="$2"
    local timeout="$3"
    local mode="$4"
    local start_ts now_ts slice uuid

    start_ts="$(date +%s)"
    while true; do
        slice="$(tail -n +"$line_start" "$file" 2>/dev/null || true)"
        if [[ -n "$slice" ]]; then
            if [[ "$mode" == "first" ]]; then
                uuid="$(printf '%s\n' "$slice" | grep -E -o '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}' | head -n 1 || true)"
            else
                uuid="$(printf '%s\n' "$slice" | grep -E -o '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}' | tail -n 1 || true)"
            fi
            if [[ -n "$uuid" ]]; then
                printf '%s\n' "$uuid"
                return 0
            fi
        fi

        now_ts="$(date +%s)"
        if (( now_ts - start_ts >= timeout )); then
            return 1
        fi
        sleep 0.1
    done
}

extract_uuid_by_marker_after() {
    local file="$1"
    local line_start="$2"
    local timeout="$3"
    local marker="$4"
    local start_ts now_ts slice line uuid

    start_ts="$(date +%s)"
    while true; do
        slice="$(tail -n +"$line_start" "$file" 2>/dev/null || true)"
        if [[ -n "$slice" ]]; then
            line="$(printf '%s\n' "$slice" | grep -E "$marker" | tail -n 1 || true)"
            if [[ -n "$line" ]]; then
                uuid="$(printf '%s\n' "$line" | grep -E -o '[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}' | head -n 1 || true)"
                if [[ -n "$uuid" ]]; then
                    printf '%s\n' "$uuid"
                    return 0
                fi
            fi
        fi

        now_ts="$(date +%s)"
        if (( now_ts - start_ts >= timeout )); then
            return 1
        fi
        sleep 0.1
    done
}

line_count() {
    wc -l < "$1" 2>/dev/null | tr -d ' '
}

short_id() {
    printf '%s' "$RUN_ID" | tr -cd '0-9' | tail -c 7
}

if [[ ! -x "$SERVER_BIN" ]]; then
    echo "Server binary not executable: $SERVER_BIN"
    exit 2
fi
if [[ ! -x "$CLIENT_BIN" ]]; then
    echo "Client binary not executable: $CLIENT_BIN"
    exit 2
fi

say "Logs directory: $LOG_DIR"
if [[ "$USE_EXISTING_SERVER" == "1" ]]; then
    say "Using existing server on $HOST:$PORT"
else
    say "Starting server on $HOST:$PORT"
    "$SERVER_BIN" "$PORT" >"$SERVER_LOG" 2>&1 &
    SERVER_PID=$!
    STARTED_SERVER=1
    sleep 0.4

    if ! kill -0 "$SERVER_PID" 2>/dev/null; then
        say "Server failed to start on port $PORT"
        if [[ -f "$SERVER_LOG" ]]; then
            say "Server startup log:"
            tail -n 20 "$SERVER_LOG" | tee -a "$SUMMARY_LOG"
        fi
        exit 2
    fi
fi

say "Starting client A"
"$CLIENT_BIN" "$HOST" "$PORT" <"$FIFO_A" >"$A_LOG" 2>&1 &
A_PID=$!
exec 3>"$FIFO_A"

say "Starting client B"
"$CLIENT_BIN" "$HOST" "$PORT" <"$FIFO_B" >"$B_LOG" 2>&1 &
B_PID=$!
exec 4>"$FIFO_B"

sleep 0.4

SID="$(short_id)"
USER_A="QA_A_$SID"
USER_B="QA_B_$SID"
TEAM_1_NAME="Team_QA_$SID"
TEAM_2_NAME="Team_EVT_$SID"
CHANNEL_1_NAME="Channel_QA_$SID"
CHANNEL_2_NAME="Channel_EVT_$SID"
THREAD_1_TITLE="Thread_QA_$SID"
THREAD_2_TITLE="Thread_EVT_$SID"
REPLY_1_BODY="Reply_QA_$SID"
REPLY_2_BODY="Reply_EVT_$SID"

UNKNOWN_UUID="00000000-0000-0000-0000-000000000000"

# 0) unknown command
start_line_a="$(line_count "$A_LOG")"
send_a '/unknown_command'
assert_contains_after "$A_LOG" "$start_line_a" 'Unknown command:' 'unknown command is rejected'

# 1) help
start_line_a="$(line_count "$A_LOG")"
send_a '/help'
assert_contains_after "$A_LOG" "$start_line_a" '/help: display this help' 'help prints command list'

start_line_a="$(line_count "$A_LOG")"
send_a '/help "extra"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /help' 'help usage validation with extra arg'

# 2) login argument validation before authentication
start_line_a="$(line_count "$A_LOG")"
send_a '/login'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /login "user_name"' 'login usage missing arg'

start_line_a="$(line_count "$A_LOG")"
send_a '/login ""'
assert_contains_after "$A_LOG" "$start_line_a" 'Username cannot be empty' 'login empty username rejected'

start_line_a="$(line_count "$A_LOG")"
send_a '/login "x" "y"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /login "user_name"' 'login usage extra arg'

# 3) unauthorized paths before login
start_line_a="$(line_count "$A_LOG")"
send_a '/create "X" "Y"'
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'create denied when not logged in'

start_line_a="$(line_count "$A_LOG")"
send_a '/subscribed'
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'subscribed denied when not logged in'

start_line_a="$(line_count "$A_LOG")"
send_a '/logout'
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'logout denied when not logged in'

start_line_a="$(line_count "$A_LOG")"
send_a '/use'
assert_contains_after "$A_LOG" "$start_line_a" 'must be logged in|Unauthorized' 'use denied when not logged in'

start_line_a="$(line_count "$A_LOG")"
send_a '/list'
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'list denied when not logged in'

start_line_a="$(line_count "$A_LOG")"
send_a '/info'
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'info denied when not logged in'

start_line_a="$(line_count "$A_LOG")"
send_a "/subscribe \"$UNKNOWN_UUID\""
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'subscribe denied when not logged in'

start_line_a="$(line_count "$A_LOG")"
send_a "/unsubscribe \"$UNKNOWN_UUID\""
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'unsubscribe denied when not logged in'

# 4) login flows and async login/logout events
start_line_a="$(line_count "$A_LOG")"
send_a "/login \"$USER_A\""
assert_contains_after "$A_LOG" "$start_line_a" 'logged in' 'A login success'

start_line_a="$(line_count "$A_LOG")"
send_a '/list "extra"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /list' 'list usage extra arg'

start_line_a="$(line_count "$A_LOG")"
send_a '/info "extra"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /info' 'info usage extra arg'

start_line_a="$(line_count "$A_LOG")"
send_a '/info'
assert_contains_after "$A_LOG" "$start_line_a" "$USER_A" 'info in empty context displays current user details'

start_line_a="$(line_count "$A_LOG")"
start_line_b="$(line_count "$B_LOG")"
send_b "/login \"$USER_B\""
assert_contains_after "$B_LOG" "$start_line_b" 'logged in' 'B login success'
send_a '/info'
assert_contains_after "$A_LOG" "$start_line_a" "$USER_B" 'A receives async login event for B'

start_line_b="$(line_count "$B_LOG")"
send_b '/logout'
assert_contains_after "$B_LOG" "$start_line_b" 'logged out' 'B logout success (pre-server-already-exist test)'

start_line_b="$(line_count "$B_LOG")"
send_b "/login \"$USER_A\""
assert_contains_after "$B_LOG" "$start_line_b" 'Already exist|already exist' 'server rejects login when username already logged in elsewhere'

start_line_a="$(line_count "$A_LOG")"
start_line_b="$(line_count "$B_LOG")"
send_b "/login \"$USER_B\""
assert_contains_after "$B_LOG" "$start_line_b" 'logged in' 'B login success after conflict test'
send_a '/info'
assert_contains_after "$A_LOG" "$start_line_a" "$USER_B" 'A receives async login event for B after relog'

start_line_a="$(line_count "$A_LOG")"
send_a "/login \"$USER_A\""
assert_contains_after "$A_LOG" "$start_line_a" 'already logged in as' 'client blocks duplicate self login'

# 5) create usage matrix in each context
start_line_a="$(line_count "$A_LOG")"
send_a '/use'
assert_not_contains_after "$A_LOG" "$start_line_a" 'Server returned unexpected status' 'use empty context succeeds while logged in'

start_line_a="$(line_count "$A_LOG")"
send_a '/create'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /create "name" "description" or /create "reply_body"' 'create usage with no args in team context'

start_line_a="$(line_count "$A_LOG")"
send_a '/create "only_one_arg"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /create "name" "description" or /create "reply_body"' 'create usage with one arg in team context'

start_line_a="$(line_count "$A_LOG")"
send_a '/create "" "desc"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /create "name" "description" or /create "reply_body"' 'create usage with empty name in team context'

# 6) create team/channel/thread/reply and event routing
start_line_a="$(line_count "$A_LOG")"
start_line_b="$(line_count "$B_LOG")"
send_a "/create \"$TEAM_1_NAME\" \"Team created by script\""
assert_contains_after "$A_LOG" "$start_line_a" '\[Team [0-9a-f-]{36}\] Created' 'team_1 created response printed'
TEAM_1_UUID="$(extract_uuid_by_marker_after "$A_LOG" "$start_line_a" "$WAIT_TIMEOUT" '\[Team [0-9a-f-]{36}\] Created' || true)"
if [[ -n "$TEAM_1_UUID" ]]; then
    pass "captured TEAM_1_UUID=$TEAM_1_UUID"
else
    fail 'could not capture TEAM_1_UUID after team creation'
fi
send_b '/info'
assert_contains_after "$B_LOG" "$start_line_b" "$TEAM_1_NAME" 'B receives async team created event'

if [[ -n "$TEAM_1_UUID" ]]; then
    start_line_a="$(line_count "$A_LOG")"
    send_a '/use'
    send_a "/create \"$TEAM_2_NAME\" \"Second team for list tests\""
    assert_contains_after "$A_LOG" "$start_line_a" '\[Team [0-9a-f-]{36}\] Created' 'team_2 created for list tests'

    start_line_a="$(line_count "$A_LOG")"
    send_a '/list'
    assert_contains_after "$A_LOG" "$start_line_a" "$TEAM_1_NAME" 'list in empty context includes team_1'
    assert_contains_after "$A_LOG" "$start_line_a" "$TEAM_2_NAME" 'list in empty context includes team_2'

    start_line_b="$(line_count "$B_LOG")"
    send_b "/use \"$TEAM_1_UUID\""
    start_line_b2="$(line_count "$B_LOG")"
    send_b '/list'
    assert_contains_after "$B_LOG" "$start_line_b2" 'Unauthorized' 'list denied when B is not subscribed to team context'

    start_line_b2="$(line_count "$B_LOG")"
    send_b '/create "Denied_Channel" "Denied because not subscribed"'
    assert_contains_after "$B_LOG" "$start_line_b2" 'Unauthorized' 'channel creation denied when user not subscribed to team'

    start_line_a="$(line_count "$A_LOG")"
    send_a "/use \"$UNKNOWN_UUID\""
    start_line_a2="$(line_count "$A_LOG")"
    send_a '/info'
    assert_contains_after "$A_LOG" "$start_line_a2" 'Requested context entity does not exist' 'info returns not found with unknown team context'

    start_line_a2="$(line_count "$A_LOG")"
    send_a '/list'
    assert_contains_after "$A_LOG" "$start_line_a2" 'Requested context entity does not exist' 'list returns not found with unknown team context'

    start_line_a2="$(line_count "$A_LOG")"
    send_a '/create "NotFound_Channel" "team missing"'
    assert_contains_after "$A_LOG" "$start_line_a2" 'Requested context entity does not exist' 'create channel not found with unknown team context'

    send_a "/use \"$TEAM_1_UUID\""

    start_line_a="$(line_count "$A_LOG")"
    send_a '/info'
    assert_contains_after "$A_LOG" "$start_line_a" "$TEAM_1_NAME" 'info in team context displays selected team details'

    start_line_a="$(line_count "$A_LOG")"
    start_line_b="$(line_count "$B_LOG")"
    send_a "/create \"$CHANNEL_1_NAME\" \"Channel created by script\""
    assert_contains_after "$A_LOG" "$start_line_a" '\[Channel [0-9a-f-]{36}\] Created' 'channel_1 created response printed'
    CHANNEL_1_UUID="$(extract_uuid_by_marker_after "$A_LOG" "$start_line_a" "$WAIT_TIMEOUT" '\[Channel [0-9a-f-]{36}\] Created' || true)"
    if [[ -n "$CHANNEL_1_UUID" ]]; then
        pass "captured CHANNEL_1_UUID=$CHANNEL_1_UUID"
    else
        fail 'could not capture CHANNEL_1_UUID after channel creation'
    fi
    send_b '/info'
    assert_not_contains_after "$B_LOG" "$start_line_b" "$CHANNEL_1_NAME" 'B does not receive channel event before subscribing'

    start_line_b="$(line_count "$B_LOG")"
    send_b "/subscribe \"$TEAM_1_UUID\""
    assert_contains_after "$B_LOG" "$start_line_b" 'Subscribed|subscribed' 'B subscribe success on team_1'

    start_line_b="$(line_count "$B_LOG")"
    send_b "/subscribe \"$TEAM_1_UUID\""
    assert_contains_after "$B_LOG" "$start_line_b" 'Already exist|already exist' 'duplicate subscribe blocked'

    start_line_a="$(line_count "$A_LOG")"
    start_line_b="$(line_count "$B_LOG")"
    send_a "/create \"$CHANNEL_2_NAME\" \"Channel event test\""
    assert_contains_after "$A_LOG" "$start_line_a" '\[Channel [0-9a-f-]{36}\] Created' 'channel_2 created response printed'
    send_b '/info'
    assert_contains_after "$B_LOG" "$start_line_b" "$CHANNEL_2_NAME" 'B receives channel created event after subscribing'

    if [[ -n "${CHANNEL_1_UUID:-}" ]]; then
        start_line_a="$(line_count "$A_LOG")"
        send_a '/list'
        assert_contains_after "$A_LOG" "$start_line_a" "$CHANNEL_1_NAME" 'list in team context includes channel_1'

        start_line_a="$(line_count "$A_LOG")"
        send_a "/use \"$TEAM_1_UUID\" \"$UNKNOWN_UUID\""
        start_line_a2="$(line_count "$A_LOG")"
        send_a '/info'
        assert_contains_after "$A_LOG" "$start_line_a2" 'Requested context entity does not exist' 'info returns not found with unknown channel context'

        start_line_a2="$(line_count "$A_LOG")"
        send_a '/list'
        assert_contains_after "$A_LOG" "$start_line_a2" 'Requested context entity does not exist' 'list returns not found with unknown channel context'

        send_a "/use \"$TEAM_1_UUID\" \"$CHANNEL_1_UUID\""

        start_line_a="$(line_count "$A_LOG")"
        send_a '/info'
        assert_contains_after "$A_LOG" "$start_line_a" "$CHANNEL_1_NAME" 'info in channel context displays selected channel details'

        start_line_a="$(line_count "$A_LOG")"
        send_a '/create "only_one_thread_arg"'
        assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /create "name" "description" or /create "reply_body"' 'create usage with one arg in thread context'

        start_line_a="$(line_count "$A_LOG")"
        start_line_b="$(line_count "$B_LOG")"
        send_a "/create \"$THREAD_1_TITLE\" \"Thread body from script\""
        assert_contains_after "$A_LOG" "$start_line_a" '\[Thread [0-9a-f-]{36}\] Created' 'thread_1 created response printed'
        THREAD_1_UUID="$(extract_uuid_by_marker_after "$A_LOG" "$start_line_a" "$WAIT_TIMEOUT" '\[Thread [0-9a-f-]{36}\] Created' || true)"
        if [[ -n "$THREAD_1_UUID" ]]; then
            pass "captured THREAD_1_UUID=$THREAD_1_UUID"
        else
            fail 'could not capture THREAD_1_UUID after thread creation'
        fi
        send_b '/info'
        assert_contains_after "$B_LOG" "$start_line_b" "$THREAD_1_TITLE" 'B receives thread created event'

        start_line_a="$(line_count "$A_LOG")"
        send_a '/list'
        assert_contains_after "$A_LOG" "$start_line_a" "$THREAD_1_TITLE" 'list in channel context includes thread_1'

        if [[ -n "$THREAD_1_UUID" ]]; then
            start_line_a="$(line_count "$A_LOG")"
            send_a "/use \"$TEAM_1_UUID\" \"$CHANNEL_1_UUID\" \"$UNKNOWN_UUID\""
            start_line_a2="$(line_count "$A_LOG")"
            send_a '/info'
            assert_contains_after "$A_LOG" "$start_line_a2" 'Requested context entity does not exist' 'info returns not found with unknown thread context'

            start_line_a2="$(line_count "$A_LOG")"
            send_a '/list'
            assert_contains_after "$A_LOG" "$start_line_a2" 'Requested context entity does not exist' 'list returns not found with unknown thread context'

            send_a "/use \"$TEAM_1_UUID\" \"$CHANNEL_1_UUID\" \"$THREAD_1_UUID\""

            start_line_a="$(line_count "$A_LOG")"
            send_a '/info'
            assert_contains_after "$A_LOG" "$start_line_a" "$THREAD_1_TITLE" 'info in thread context displays selected thread details'

            start_line_a="$(line_count "$A_LOG")"
            send_a '/create "wrong" "two args in reply context"'
            assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /create "reply_body"' 'reply context rejects 2-arg create'

            start_line_a="$(line_count "$A_LOG")"
            start_line_b="$(line_count "$B_LOG")"
            send_a "/create \"$REPLY_1_BODY\""
            assert_contains_after "$A_LOG" "$start_line_a" '\[Reply [0-9a-f-]{36}\]|reply' 'reply_1 created response printed'
            assert_not_contains_after "$A_LOG" "$start_line_a" 'requested context entity does not exist|not found|unauthorized' 'reply_1 creation did not fail'
            send_b '/info'
            assert_contains_after "$B_LOG" "$start_line_b" "$REPLY_1_BODY|Reply" 'B receives reply created event'

            start_line_a="$(line_count "$A_LOG")"
            send_a '/list'
            assert_contains_after "$A_LOG" "$start_line_a" "$REPLY_1_BODY" 'list in thread context includes reply_1'
        fi
    fi
fi

# 7) subscribe/unsubscribe usage and error matrix
start_line_b="$(line_count "$B_LOG")"
send_b '/subscribe'
assert_contains_after "$B_LOG" "$start_line_b" 'Usage: /subscribe "team_uuid"' 'subscribe usage missing arg'

start_line_b="$(line_count "$B_LOG")"
send_b '/subscribe "x" "y"'
assert_contains_after "$B_LOG" "$start_line_b" 'Usage: /subscribe "team_uuid"' 'subscribe usage extra arg'

start_line_b="$(line_count "$B_LOG")"
send_b '/subscribe "not-a-uuid"'
assert_contains_after "$B_LOG" "$start_line_b" 'Invalid team UUID format' 'subscribe invalid UUID format rejected'

start_line_b="$(line_count "$B_LOG")"
send_b "/subscribe \"$UNKNOWN_UUID\""
assert_contains_after "$B_LOG" "$start_line_b" 'Unknown team|unknown team' 'subscribe unknown team rejected'

start_line_b="$(line_count "$B_LOG")"
send_b '/unsubscribe'
assert_contains_after "$B_LOG" "$start_line_b" 'Usage: /unsubscribe "team_uuid"' 'unsubscribe usage missing arg'

start_line_b="$(line_count "$B_LOG")"
send_b '/unsubscribe "x" "y"'
assert_contains_after "$B_LOG" "$start_line_b" 'Usage: /unsubscribe "team_uuid"' 'unsubscribe usage extra arg'

start_line_b="$(line_count "$B_LOG")"
send_b '/unsubscribe "not-a-uuid"'
assert_contains_after "$B_LOG" "$start_line_b" 'Invalid team UUID format' 'unsubscribe invalid UUID format rejected'

start_line_b="$(line_count "$B_LOG")"
send_b "/unsubscribe \"$UNKNOWN_UUID\""
assert_contains_after "$B_LOG" "$start_line_b" 'Unknown team|unknown team' 'unsubscribe unknown team rejected'

if [[ -n "${TEAM_1_UUID:-}" ]]; then
    start_line_b="$(line_count "$B_LOG")"
    send_b "/unsubscribe \"$TEAM_1_UUID\""
    assert_contains_after "$B_LOG" "$start_line_b" 'Unsubscribed|unsubscribed' 'unsubscribe success'

    start_line_b="$(line_count "$B_LOG")"
    send_b "/unsubscribe \"$TEAM_1_UUID\""
    assert_contains_after "$B_LOG" "$start_line_b" 'not subscribed|forbidden' 'double unsubscribe rejected'
fi

# 8) subscribed usage and error matrix
start_line_b="$(line_count "$B_LOG")"
send_b '/subscribed "x" "y"'
assert_contains_after "$B_LOG" "$start_line_b" 'Usage: /subscribed \["team_uuid"\]' 'subscribed usage extra args'

start_line_b="$(line_count "$B_LOG")"
send_b '/subscribed "not-a-uuid"'
assert_contains_after "$B_LOG" "$start_line_b" 'Invalid team UUID format' 'subscribed invalid UUID format'

start_line_b="$(line_count "$B_LOG")"
send_b "/subscribed \"$UNKNOWN_UUID\""
assert_contains_after "$B_LOG" "$start_line_b" 'Unknown team|unknown team' 'subscribed unknown team rejected'

start_line_a="$(line_count "$A_LOG")"
send_a '/subscribed'
assert_contains_after "$A_LOG" "$start_line_a" 'Team|team' 'subscribed without arg returns teams list'

if [[ -n "${TEAM_1_UUID:-}" ]]; then
    start_line_a="$(line_count "$A_LOG")"
    send_a "/subscribed \"$TEAM_1_UUID\""
    assert_contains_after "$A_LOG" "$start_line_a" "$USER_A|User" 'subscribed with team returns users'
fi

# 9) use validation matrix
start_line_a="$(line_count "$A_LOG")"
send_a '/use "bad"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /use' 'use invalid team uuid rejected'

start_line_a="$(line_count "$A_LOG")"
send_a '/use "11111111-1111-1111-1111-111111111111" "bad"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /use' 'use invalid channel uuid rejected'

start_line_a="$(line_count "$A_LOG")"
send_a '/use "11111111-1111-1111-1111-111111111111" "22222222-2222-2222-2222-222222222222" "bad"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /use' 'use invalid thread uuid rejected'

start_line_a="$(line_count "$A_LOG")"
send_a '/use "11111111-1111-1111-1111-111111111111" "22222222-2222-2222-2222-222222222222" "33333333-3333-3333-3333-333333333333" "extra"'
assert_contains_after "$A_LOG" "$start_line_a" 'Usage: /use' 'use extra arg rejected'

# 10) logout usage and final auth checks
start_line_b="$(line_count "$B_LOG")"
send_b '/logout "extra"'
assert_contains_after "$B_LOG" "$start_line_b" 'Usage: /logout' 'logout usage extra arg'

start_line_a="$(line_count "$A_LOG")"
start_line_b="$(line_count "$B_LOG")"
send_b '/logout'
assert_contains_after "$B_LOG" "$start_line_b" 'logged out' 'B logout success'
send_a '/info'
assert_contains_after "$A_LOG" "$start_line_a" "$USER_B" 'A receives async logout event for B'

start_line_b="$(line_count "$B_LOG")"
send_b '/subscribed'
assert_contains_after "$B_LOG" "$start_line_b" 'Unauthorized' 'B unauthorized after logout'

start_line_b="$(line_count "$B_LOG")"
send_b '/list'
assert_contains_after "$B_LOG" "$start_line_b" 'Unauthorized' 'B list unauthorized after logout'

start_line_b="$(line_count "$B_LOG")"
send_b '/info'
assert_contains_after "$B_LOG" "$start_line_b" 'Unauthorized' 'B info unauthorized after logout'

start_line_b="$(line_count "$B_LOG")"
send_b '/logout'
assert_contains_after "$B_LOG" "$start_line_b" 'Unauthorized' 'B second logout denied'

start_line_a="$(line_count "$A_LOG")"
send_a '/logout'
assert_contains_after "$A_LOG" "$start_line_a" 'logged out' 'A logout success'

start_line_a="$(line_count "$A_LOG")"
send_a '/create "X" "Y"'
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'A unauthorized after logout'

start_line_a="$(line_count "$A_LOG")"
send_a '/list'
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'A list unauthorized after logout'

start_line_a="$(line_count "$A_LOG")"
send_a '/info'
assert_contains_after "$A_LOG" "$start_line_a" 'Unauthorized' 'A info unauthorized after logout'

# Global checks
assert_not_contains "$A_LOG" 'Server returned unexpected status' 'A has no unexpected status'
assert_not_contains "$B_LOG" 'Server returned unexpected status' 'B has no unexpected status'
assert_not_contains "$A_LOG" 'Malformed .* payload' 'A has no malformed payload'
assert_not_contains "$B_LOG" 'Malformed .* payload' 'B has no malformed payload'

say ""
say "Summary: PASS=$PASS_COUNT FAIL=$FAIL_COUNT"
say "Server log: $SERVER_LOG"
say "Client A log: $A_LOG"
say "Client B log: $B_LOG"

if (( FAIL_COUNT > 0 )); then
    exit 1
fi
exit 0
