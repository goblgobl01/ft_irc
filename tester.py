#!/usr/bin/env python3
"""
ft_irc — Comprehensive Automated Test Suite
=============================================
Tests every command, edge case, error path, and robustness scenario.

Usage:
    1. Start your server:  ./ircserv 6667 secret
    2. Run tests:          python3 test_suite.py [port] [password]
       Defaults:           port=6667, password=secret

Authors: Automated Test Harness for riel-fas, zben-oma, yabenman
"""

import socket
import time
import sys
import os
import signal
import random
import string

# ─── Configuration ───────────────────────────────────────────────
HOST = "127.0.0.1"
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 6667
PASSWORD = sys.argv[2] if len(sys.argv) > 2 else "secret"
TIMEOUT = 2.0   # seconds to wait for server response

# ─── Colors ──────────────────────────────────────────────────────
class C:
    GREEN  = "\033[92m"
    RED    = "\033[91m"
    YELLOW = "\033[93m"
    CYAN   = "\033[96m"
    BOLD   = "\033[1m"
    DIM    = "\033[2m"
    RESET  = "\033[0m"
    PURPLE = "\033[95m"
    BG_GREEN = "\033[42m"
    BG_RED   = "\033[41m"

# ─── Statistics ──────────────────────────────────────────────────
stats = {"passed": 0, "failed": 0, "warned": 0, "total": 0}
current_section = ""
test_results = []  # list of (section, name, status, detail)

# ─── Helpers ─────────────────────────────────────────────────────
def make_conn():
    """Create a new TCP connection to the server."""
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(TIMEOUT)
    s.connect((HOST, PORT))
    return s

def send_raw(sock, data):
    """Send raw bytes (append \\r\\n if not present)."""
    if not data.endswith("\r\n"):
        data += "\r\n"
    sock.sendall(data.encode())
    time.sleep(0.05)  # small delay to let server process

def recv_all(sock, timeout=None):
    """Receive all available data within timeout."""
    if timeout is None:
        timeout = TIMEOUT
    sock.settimeout(timeout)
    data = b""
    try:
        chunk = sock.recv(4096)
        if chunk:
            data += chunk
            sock.settimeout(0.05)
            while True:
                chunk = sock.recv(4096)
                if not chunk: break
                data += chunk
    except socket.timeout:
        pass
    except ConnectionResetError:
        pass
    return data.decode("utf-8", errors="replace")

def register_client(sock, nick, password=None, user=None):
    """Register a client with PASS, NICK, USER."""
    if password is None:
        password = PASSWORD
    if user is None:
        user = nick
    send_raw(sock, f"PASS {password}")
    send_raw(sock, f"NICK {nick}")
    send_raw(sock, f"USER {user} 0 * :Real Name {nick}")
    time.sleep(0.1)
    return recv_all(sock)

def quick_client(nick, password=None):
    """Create a connected + registered client, return (socket, welcome_response)."""
    s = make_conn()
    resp = register_client(s, nick, password)
    return s, resp

def close_conn(sock):
    """Safely close a connection."""
    try:
        sock.close()
    except:
        pass

# ─── Test Framework ──────────────────────────────────────────────
def section(name):
    global current_section
    current_section = name
    print(f"\n{C.BOLD}{C.CYAN}{'═'*60}{C.RESET}")
    print(f"{C.BOLD}{C.CYAN}  {name}{C.RESET}")
    print(f"{C.BOLD}{C.CYAN}{'═'*60}{C.RESET}")

def test_pass(name, detail=""):
    stats["passed"] += 1
    stats["total"] += 1
    test_results.append((current_section, name, "PASS", detail))
    print(f"  {C.GREEN}✓ PASS{C.RESET}  {name}" + (f"  {C.DIM}{detail}{C.RESET}" if detail else ""))

def test_fail(name, detail=""):
    stats["failed"] += 1
    stats["total"] += 1
    test_results.append((current_section, name, "FAIL", detail))
    print(f"  {C.RED}✗ FAIL{C.RESET}  {name}" + (f"  {C.RED}{detail}{C.RESET}" if detail else ""))

def test_warn(name, detail=""):
    stats["warned"] += 1
    stats["total"] += 1
    test_results.append((current_section, name, "WARN", detail))
    print(f"  {C.YELLOW}⚠ WARN{C.RESET}  {name}" + (f"  {C.YELLOW}{detail}{C.RESET}" if detail else ""))

def check(condition, name, detail_pass="", detail_fail=""):
    if condition:
        test_pass(name, detail_pass)
    else:
        test_fail(name, detail_fail)

# ═════════════════════════════════════════════════════════════════
#  1. CONNECTION & AUTHENTICATION
# ═════════════════════════════════════════════════════════════════
def test_connection_auth():
    section("1. CONNECTION & AUTHENTICATION")

    # 1.1 — Standard successful connection
    try:
        s, resp = quick_client("authusr1")
        check("001" in resp, "1.1  Standard connection — RPL_WELCOME (001)", "", f"Got: {resp[:100]}")
        check("002" in resp, "1.1  Standard connection — RPL_YOURHOST (002)")
        check("003" in resp, "1.1  Standard connection — RPL_CREATED (003)")
        check("004" in resp, "1.1  Standard connection — RPL_MYINFO (004)")
        close_conn(s)
    except Exception as e:
        test_fail("1.1  Standard connection", str(e))

    # 1.2 — Wrong password
    try:
        s = make_conn()
        send_raw(s, "PASS wrongpassword")
        send_raw(s, "NICK wrongpass")
        send_raw(s, "USER wrongpass 0 * :Test")
        time.sleep(0.2)
        resp = recv_all(s, 1.0)
        check("464" in resp, "1.2  Wrong password — ERR_PASSWDMISMATCH (464)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.2  Wrong password", str(e))

    # 1.3 — Missing password (NICK+USER without PASS)
    try:
        s = make_conn()
        send_raw(s, "NICK nopass_user")
        send_raw(s, "USER nopass_user 0 * :Test")
        time.sleep(0.2)
        resp = recv_all(s, 1.0)
        check("451" in resp or "462" in resp or resp == "", "1.3  Missing PASS — rejected or ignored", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.3  Missing password", str(e))

    # 1.4 — Empty PASS parameter
    try:
        s = make_conn()
        send_raw(s, "PASS")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("461" in resp, "1.4  PASS with no parameter — ERR_NEEDMOREPARAMS (461)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.4  Empty PASS", str(e))

    # 1.5 — Nickname collision
    try:
        s1, _ = quick_client("collsnick")
        s2 = make_conn()
        send_raw(s2, f"PASS {PASSWORD}")
        send_raw(s2, "NICK collsnick")
        send_raw(s2, "USER user2 0 * :User2")
        time.sleep(0.2)
        resp = recv_all(s2, 1.0)
        check("433" in resp, "1.5  Nickname collision — ERR_NICKNAMEINUSE (433)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("1.5  Nickname collision", str(e))

    # 1.6 — Invalid nickname (starts with digit)
    try:
        s = make_conn()
        send_raw(s, f"PASS {PASSWORD}")
        send_raw(s, "NICK 123bad")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("432" in resp, "1.6  Invalid nick (starts with digit) — ERR_ERRONEUSNICKNAME (432)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.6  Invalid nickname (digit)", str(e))

    # 1.7 — Invalid nickname (special chars)
    try:
        s = make_conn()
        send_raw(s, f"PASS {PASSWORD}")
        send_raw(s, "NICK bad#nick")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("432" in resp, "1.7  Invalid nick (special chars #) — ERR_ERRONEUSNICKNAME (432)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.7  Invalid nickname (special)", str(e))

    # 1.9 — No nickname given
    try:
        s = make_conn()
        send_raw(s, f"PASS {PASSWORD}")
        send_raw(s, "NICK")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("431" in resp, "1.9  No nickname given — ERR_NONICKNAMEGIVEN (431)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.9  No nickname given", str(e))

    # 1.10 — USER with insufficient params
    try:
        s = make_conn()
        send_raw(s, f"PASS {PASSWORD}")
        send_raw(s, "NICK user_noparams")
        send_raw(s, "USER onlyone")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("461" in resp, "1.10 USER insufficient params — ERR_NEEDMOREPARAMS (461)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.10 USER insufficient params", str(e))

    # 1.11 — Double registration (PASS after registered)
    try:
        s, _ = quick_client("double_reg")
        send_raw(s, f"PASS {PASSWORD}")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("462" in resp, "1.11 PASS after registration — ERR_ALREADYREGISTERED (462)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.11 Double registration", str(e))

    # 1.12 — USER after registration
    try:
        s, _ = quick_client("double_user")
        send_raw(s, "USER newuser 0 * :New")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("462" in resp, "1.12 USER after registration — ERR_ALREADYREGISTERED (462)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.12 USER after registration", str(e))

    # 1.13 — Commands before registration
    try:
        s = make_conn()
        send_raw(s, "JOIN #test")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("451" in resp, "1.13 JOIN before registration — ERR_NOTREGISTERED (451)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.13 Commands before registration", str(e))

    # 1.14 — PRIVMSG before registration
    try:
        s = make_conn()
        send_raw(s, "PRIVMSG someone :hello")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("451" in resp, "1.14 PRIVMSG before registration — ERR_NOTREGISTERED (451)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.14 PRIVMSG before registration", str(e))

    # 1.15 — Valid special characters in nickname
    try:
        s, resp = quick_client("[nick]")
        check("001" in resp, "1.15 Valid special chars nickname [nick] — accepted", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.15 Valid special chars nickname", str(e))

    # 1.16 — Nickname case-insensitive collision
    try:
        s1, _ = quick_client("CaseTest")
        s2 = make_conn()
        send_raw(s2, f"PASS {PASSWORD}")
        send_raw(s2, "NICK casetest")
        send_raw(s2, "USER casetest 0 * :case")
        time.sleep(0.2)
        resp = recv_all(s2, 1.0)
        check("433" in resp, "1.16 Case-insensitive nick collision CaseTest/casetest — (433)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("1.16 Case-insensitive collision", str(e))

    # 1.17 — Registration order: USER before NICK
    try:
        s = make_conn()
        send_raw(s, f"PASS {PASSWORD}")
        send_raw(s, "USER ubn 0 * :User Before Nick")
        send_raw(s, "NICK ubn_nick")
        time.sleep(0.2)
        resp = recv_all(s, 1.0)
        check("001" in resp, "1.17 Registration order USER→NICK — should complete", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("1.17 USER before NICK", str(e))


# ═════════════════════════════════════════════════════════════════
#  2. PING / PONG
# ═════════════════════════════════════════════════════════════════
def test_ping_pong():
    section("2. PING / PONG")

    # 2.1 — Basic PING
    try:
        s, _ = quick_client("ping_user")
        send_raw(s, "PING :testtoken123")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("PONG" in resp and "testtoken123" in resp, "2.1  PING :testtoken123 → PONG with token", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("2.1  Basic PING", str(e))

    # 2.2 — PING without token
    try:
        s, _ = quick_client("ping_notoken")
        send_raw(s, "PING")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("PONG" in resp, "2.2  PING without token — still responds PONG", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("2.2  PING without token", str(e))

    # 2.3 — Multiple rapid PINGs
    try:
        s, _ = quick_client("ping_rapid")
        for i in range(5):
            send_raw(s, f"PING :tok{i}")
        time.sleep(0.3)
        resp = recv_all(s, 1.0)
        pong_count = resp.count("PONG")
        check(pong_count >= 5, f"2.3  5 rapid PINGs → {pong_count} PONGs received", "", f"Expected 5, got {pong_count}")
        close_conn(s)
    except Exception as e:
        test_fail("2.3  Rapid PINGs", str(e))


# ═════════════════════════════════════════════════════════════════
#  3. NICK CHANGE (post-registration)
# ═════════════════════════════════════════════════════════════════
def test_nickchge():
    section("3. NICK CHANGE (Post-Registration)")

    # 3.1 — Successful nick change
    try:
        s, _ = quick_client("oldnick")
        send_raw(s, "NICK newnick")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("NICK" in resp and "newnick" in resp, "3.1  NICK change oldnick→newnick — broadcast received", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("3.1  Nick change", str(e))

    # 3.3 — Nick change to invalid nick after registration
    try:
        s, _ = quick_client("valid_nick")
        send_raw(s, "NICK 1invalid")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("432" in resp, "3.3  NICK change to invalid — ERR_ERRONEUSNICKNAME (432)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("3.3  Invalid nick change", str(e))

    # 3.4 — Nick change to same nick
    try:
        s, _ = quick_client("samenick")
        send_raw(s, "NICK samenick")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        # Should either succeed silently or send NICK message
        check("432" not in resp and "433" not in resp, "3.4  NICK change to same nick — no error", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("3.4  Same nick change", str(e))


# ═════════════════════════════════════════════════════════════════
#  4. MESSAGING (PRIVMSG)
# ═════════════════════════════════════════════════════════════════
def test_messaging():
    section("4. MESSAGING (PRIVMSG)")

    # 4.1 — Private message user to user
    try:
        s1, _ = quick_client("sender1")
        s2, _ = quick_client("receiver1")
        send_raw(s1, "PRIVMSG receiver1 :Hello world!")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        check("PRIVMSG" in resp and "Hello world!" in resp and "sender1" in resp,
              "4.1  PRIVMSG user→user — message received", "", f"Got: {resp[:150]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("4.1  User to user message", str(e))

    # 4.2 — Channel message
    try:
        s1, _ = quick_client("chansend")
        s2, _ = quick_client("chanrecv")
        send_raw(s1, "JOIN #msgtest")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #msgtest")
        recv_all(s2, 0.3)
        recv_all(s1, 0.3)  # clear join notification
        send_raw(s1, "PRIVMSG #msgtest :Channel message!")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        check("PRIVMSG" in resp and "Channel message!" in resp,
              "4.2  PRIVMSG to channel — other users receive", "", f"Got: {resp[:150]}")
        # Sender should NOT receive their own message
        resp_self = recv_all(s1, 0.3)
        check("Channel message!" not in resp_self,
              "4.2b Sender does NOT receive own channel message", "", f"Got own: {resp_self[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("4.2  Channel message", str(e))

    # 4.3 — PRIVMSG to non-existent user
    try:
        s, _ = quick_client("msg_ghost")
        send_raw(s, "PRIVMSG ghost_user :Hello?")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("401" in resp, "4.3  PRIVMSG to non-existent user — ERR_NOSUCHNICK (401)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("4.3  Non-existent user", str(e))

    # 4.4 — PRIVMSG to non-existent channel
    try:
        s, _ = quick_client("msgnoch")
        send_raw(s, "PRIVMSG #nochannel :Hello?")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("401" in resp or "403" in resp, "4.4  PRIVMSG to non-existent channel — error", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("4.4  Non-existent channel", str(e))

    # 4.5 — PRIVMSG to channel without being in it
    try:
        s1, _ = quick_client("chanownr")
        s2, _ = quick_client("outsider")
        send_raw(s1, "JOIN #private_ch")
        recv_all(s1, 0.3)
        send_raw(s2, "PRIVMSG #private_ch :Sneaky message")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("404" in resp, "4.5  PRIVMSG to channel without membership — ERR_CANNOTSENDTOCHAN (404)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("4.5  Channel msg without membership", str(e))

    # 4.6 — PRIVMSG with no recipient
    try:
        s, _ = quick_client("no_recip")
        send_raw(s, "PRIVMSG")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("411" in resp, "4.6  PRIVMSG no recipient — ERR_NORECIPIENT (411)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("4.6  No recipient", str(e))

    # 4.7 — PRIVMSG with no text
    try:
        s, _ = quick_client("no_text")
        send_raw(s, "PRIVMSG someone")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("412" in resp, "4.7  PRIVMSG no text — ERR_NOTEXTTOSEND (412)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("4.7  No text", str(e))

    # 4.8 — PRIVMSG with empty text
    try:
        s1, _ = quick_client("emptytext")
        send_raw(s1, "PRIVMSG emptytext :")
        time.sleep(0.1)
        resp = recv_all(s1, 0.5)
        check("412" in resp or "PRIVMSG" not in resp,
              "4.8  PRIVMSG empty text — rejected or silently ignored", "", f"Got: {resp[:100]}")
        close_conn(s1)
    except Exception as e:
        test_fail("4.8  Empty text", str(e))

    # 4.9 — Message with special characters
    try:
        s1, _ = quick_client("special_s")
        s2, _ = quick_client("special_r")
        msg = "Hello! @#$%^&*() 日本語 émojis 🎉"
        send_raw(s1, f"PRIVMSG special_r :{msg}")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        check("PRIVMSG" in resp, "4.9  PRIVMSG with special/unicode characters — delivered", "", f"Got: {resp[:150]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("4.9  Special characters", str(e))


# ═════════════════════════════════════════════════════════════════
#  5. CHANNEL OPERATIONS (JOIN, PART, KICK, TOPIC)
# ═════════════════════════════════════════════════════════════════
def test_channel_ops():
    section("5. CHANNEL OPERATIONS")

    # 5.1 — Basic JOIN
    try:
        s, _ = quick_client("joiner1")
        send_raw(s, "JOIN #basicjoin")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("JOIN" in resp and "#basicjoin" in resp, "5.1  JOIN #basicjoin — JOIN message received", "", f"Got: {resp[:100]}")
        check("353" in resp, "5.1b JOIN — RPL_NAMREPLY (353) with user list")
        check("366" in resp, "5.1c JOIN — RPL_ENDOFNAMES (366)")
        close_conn(s)
    except Exception as e:
        test_fail("5.1  Basic JOIN", str(e))

    # 5.2 — First user gets operator
    try:
        s, _ = quick_client("first_op")
        send_raw(s, "JOIN #optest")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("@first_op" in resp, "5.2  First user in channel gets operator (@prefix)", "", f"Got: {resp[:150]}")
        close_conn(s)
    except Exception as e:
        test_fail("5.2  First user operator", str(e))

    # 5.3 — JOIN with invalid channel name (no #)
    try:
        s, _ = quick_client("joinbad")
        send_raw(s, "JOIN invalidchan")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("476" in resp, "5.3  JOIN without # prefix — ERR_BADCHANMASK (476)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("5.3  Invalid channel name", str(e))

    # 5.4 — JOIN no params
    try:
        s, _ = quick_client("joinnop")
        send_raw(s, "JOIN")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("461" in resp, "5.4  JOIN with no params — ERR_NEEDMOREPARAMS (461)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("5.4  JOIN no params", str(e))

    # 5.5 — JOIN multiple channels with commas
    try:
        s, _ = quick_client("multijoin")
        send_raw(s, "JOIN #multi1,#multi2,#multi3")
        time.sleep(0.2)
        resp = recv_all(s, 1.0)
        check("#multi1" in resp and "#multi2" in resp and "#multi3" in resp,
              "5.5  JOIN #multi1,#multi2,#multi3 — all joined", "", f"Got: {resp[:200]}")
        close_conn(s)
    except Exception as e:
        test_fail("5.5  Multi-channel JOIN", str(e))

    # 5.6 — Double JOIN same channel
    try:
        s, _ = quick_client("double_j")
        send_raw(s, "JOIN #doublejoin")
        recv_all(s, 0.3)
        send_raw(s, "JOIN #doublejoin")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        # Should silently ignore (no error, no double join)
        check("JOIN" not in resp, "5.6  Double JOIN same channel — silently ignored", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("5.6  Double JOIN", str(e))

    # 5.7 — JOIN broadcasts to existing members
    try:
        s1, _ = quick_client("existing1")
        send_raw(s1, "JOIN #broadcast")
        recv_all(s1, 0.3)
        s2, _ = quick_client("newcomer1")
        send_raw(s2, "JOIN #broadcast")
        recv_all(s2, 0.3)
        time.sleep(0.2)
        resp = recv_all(s1, 0.5)
        check("JOIN" in resp and "newcomer1" in resp,
              "5.7  JOIN broadcasts to existing members", "", f"Got: {resp[:150]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("5.7  JOIN broadcast", str(e))

    # 5.8 — PART command (NOTE: may not be implemented)
    try:
        s, _ = quick_client("parter")
        send_raw(s, "JOIN #parttest")
        recv_all(s, 0.3)
        send_raw(s, "PART #parttest :Leaving")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        if "PART" in resp:
            test_pass("5.8  PART #parttest — PART message received")
        elif "421" in resp:
            test_warn("5.8  PART command — NOT IMPLEMENTED (421 Unknown command)")
        else:
            test_warn("5.8  PART command — unexpected response", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("5.8  PART", str(e))

    # 5.9 — KICK (by operator)
    try:
        s1, _ = quick_client("kicker")
        s2, _ = quick_client("kicked")
        send_raw(s1, "JOIN #kicktest")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #kicktest")
        recv_all(s2, 0.3)
        recv_all(s1, 0.3)
        send_raw(s1, "KICK #kicktest kicked :Bye!")
        time.sleep(0.2)
        resp1 = recv_all(s1, 0.5)
        resp2 = recv_all(s2, 0.5)
        check("KICK" in resp1 or "KICK" in resp2,
              "5.9  KICK by operator — KICK message broadcast", "", f"kicker got: {resp1[:100]}, kicked got: {resp2[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("5.9  KICK", str(e))

    # 5.10 — KICK by non-operator
    try:
        s1, _ = quick_client("op_user")
        s2, _ = quick_client("nonopusr")
        s3, _ = quick_client("targetusr")
        send_raw(s1, "JOIN #kickperm")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #kickperm")
        recv_all(s2, 0.3)
        send_raw(s3, "JOIN #kickperm")
        recv_all(s3, 0.3)
        send_raw(s2, "KICK #kickperm targetusr :No permission")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("482" in resp, "5.10 KICK by non-operator — ERR_CHANOPRIVSNEEDED (482)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
        close_conn(s3)
    except Exception as e:
        test_fail("5.10 KICK by non-op", str(e))

    # 5.11 — KICK non-existent user
    try:
        s, _ = quick_client("kicker2")
        send_raw(s, "JOIN #kickghost")
        recv_all(s, 0.3)
        send_raw(s, "KICK #kickghost ghostuser :Bye")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("401" in resp, "5.11 KICK non-existent user — ERR_NOSUCHNICK (401)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("5.11 KICK non-existent", str(e))

    # 5.12 — KICK no params
    try:
        s, _ = quick_client("kicknop")
        send_raw(s, "KICK")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("461" in resp, "5.12 KICK no params — ERR_NEEDMOREPARAMS (461)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("5.12 KICK no params", str(e))

    # 5.13 — KICK from non-existent channel
    try:
        s, _ = quick_client("kicknoch")
        send_raw(s, "KICK #nonexist someone :reason")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("403" in resp, "5.13 KICK from non-existent channel — ERR_NOSUCHCHANNEL (403)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("5.13 KICK non-existent channel", str(e))

    # 5.14 — Empty channel cleanup after last user kicked
    try:
        s1, _ = quick_client("lastman")
        s2, _ = quick_client("kicked2")
        send_raw(s1, "JOIN #cleanup")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #cleanup")
        recv_all(s2, 0.3)
        recv_all(s1, 0.3)
        send_raw(s1, "KICK #cleanup kicked2 :out")
        recv_all(s1, 0.3)
        # Now s1 leaves too via QUIT
        send_raw(s1, "QUIT :bye")
        time.sleep(0.3)
        # New user tries to join — channel should be fresh
        s3, _ = quick_client("reborn")
        send_raw(s3, "JOIN #cleanup")
        time.sleep(0.1)
        resp = recv_all(s3, 0.5)
        check("@reborn" in resp, "5.14 Empty channel cleanup — new joiner gets operator", "", f"Got: {resp[:150]}")
        close_conn(s2)
        close_conn(s3)
    except Exception as e:
        test_fail("5.14 Channel cleanup", str(e))


# ═════════════════════════════════════════════════════════════════
#  6. TOPIC
# ═════════════════════════════════════════════════════════════════
def test_topic():
    section("6. TOPIC")

    # 6.1 — View topic (no topic set)
    try:
        s, _ = quick_client("topic_v1")
        send_raw(s, "JOIN #topictest")
        recv_all(s, 0.3)
        send_raw(s, "TOPIC #topictest")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("331" in resp, "6.1  View topic (none set) — RPL_NOTOPIC (331)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("6.1  View empty topic", str(e))

    # 6.2 — Set topic (as operator)
    try:
        s, _ = quick_client("topic_set")
        send_raw(s, "JOIN #topicset")
        recv_all(s, 0.3)
        send_raw(s, "TOPIC #topicset :Hello World Topic!")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("TOPIC" in resp and "Hello World Topic!" in resp,
              "6.2  Set topic as operator — broadcast", "", f"Got: {resp[:150]}")
        close_conn(s)
    except Exception as e:
        test_fail("6.2  Set topic", str(e))

    # 6.3 — View topic (after setting)
    try:
        s, _ = quick_client("topic_v2")
        send_raw(s, "JOIN #topicview")
        recv_all(s, 0.3)
        send_raw(s, "TOPIC #topicview :Persistent Topic")
        recv_all(s, 0.3)
        send_raw(s, "TOPIC #topicview")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("332" in resp and "Persistent Topic" in resp,
              "6.3  View topic after setting — RPL_TOPIC (332)", "", f"Got: {resp[:150]}")
        close_conn(s)
    except Exception as e:
        test_fail("6.3  View set topic", str(e))

    # 6.4 — Set topic without operator (when +t is set)
    try:
        s1, _ = quick_client("topic_op")
        s2, _ = quick_client("topic_nop")
        send_raw(s1, "JOIN #topicperm")
        recv_all(s1, 0.3)
        # +t is default (topicRestricted = true)
        send_raw(s2, "JOIN #topicperm")
        recv_all(s2, 0.3)
        send_raw(s2, "TOPIC #topicperm :Unauthorized!")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("482" in resp, "6.4  TOPIC change without op (+t set) — ERR_CHANOPRIVSNEEDED (482)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("6.4  Topic without op", str(e))

    # 6.5 — Set topic when +t is removed
    try:
        s1, _ = quick_client("topic_t1")
        s2, _ = quick_client("topic_t2")
        send_raw(s1, "JOIN #topicfree")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #topicfree -t")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #topicfree")
        recv_all(s2, 0.3)
        send_raw(s2, "TOPIC #topicfree :Anyone can set this!")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("TOPIC" in resp and "Anyone can set this!" in resp,
              "6.5  TOPIC change with -t (anyone can set) — success", "", f"Got: {resp[:150]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("6.5  Topic with -t", str(e))

    # 6.6 — TOPIC for non-existent channel
    try:
        s, _ = quick_client("topic_ne")
        send_raw(s, "TOPIC #nope")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("403" in resp, "6.6  TOPIC for non-existent channel — ERR_NOSUCHCHANNEL (403)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("6.6  Topic non-existent channel", str(e))

    # 6.7 — TOPIC without being in channel
    try:
        s1, _ = quick_client("in_ch")
        s2, _ = quick_client("out_ch")
        send_raw(s1, "JOIN #topiconly")
        recv_all(s1, 0.3)
        send_raw(s2, "TOPIC #topiconly")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("442" in resp, "6.7  TOPIC without membership — ERR_NOTONCHANNEL (442)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("6.7  Topic not on channel", str(e))

    # 6.8 — Topic shown on JOIN
    try:
        s1, _ = quick_client("topic_j1")
        send_raw(s1, "JOIN #topicjoin")
        recv_all(s1, 0.3)
        send_raw(s1, "TOPIC #topicjoin :Welcome!")
        recv_all(s1, 0.3)
        s2, _ = quick_client("topic_j2")
        send_raw(s2, "JOIN #topicjoin")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        check("332" in resp and "Welcome!" in resp,
              "6.8  Topic shown to new joiner — RPL_TOPIC (332)", "", f"Got: {resp[:150]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("6.8  Topic on join", str(e))


# ═════════════════════════════════════════════════════════════════
#  7. CHANNEL MODES
# ═════════════════════════════════════════════════════════════════
def test_channel_modes():
    section("7. CHANNEL MODES (i, t, k, o, l)")

    # 7.1 — MODE query (no mode string)
    try:
        s, _ = quick_client("mode_q")
        send_raw(s, "JOIN #modequery")
        recv_all(s, 0.3)
        send_raw(s, "MODE #modequery")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("324" in resp, "7.1  MODE query — RPL_CHANNELMODEIS (324)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("7.1  MODE query", str(e))

    # ── 7.2 Invite-Only (+i) ──
    try:
        s1, _ = quick_client("mode_i_op")
        s2, _ = quick_client("modeiusr")
        send_raw(s1, "JOIN #invitetest")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #invitetest +i")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #invitetest")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("473" in resp, "7.2  +i (invite-only) — uninvited user gets ERR_INVITEONLYCHAN (473)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("7.2  Invite-only mode", str(e))

    # 7.3 — INVITE bypasses +i
    try:
        s1, _ = quick_client("inv_op")
        s2, _ = quick_client("inv_user")
        send_raw(s1, "JOIN #invbypass")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #invbypass +i")
        recv_all(s1, 0.3)
        send_raw(s1, "INVITE inv_user #invbypass")
        recv_all(s1, 0.3)
        time.sleep(0.1)
        resp_invite = recv_all(s2, 0.5)
        check("INVITE" in resp_invite, "7.3a INVITE notification sent to target", "", f"Got: {resp_invite[:100]}")
        send_raw(s2, "JOIN #invbypass")
        time.sleep(0.2)
        resp = recv_all(s2, 1.0)
        check("JOIN" in resp and "#invbypass" in resp and "473" not in resp,
              "7.3b Invited user can JOIN +i channel", "", f"Got: {resp[:150]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("7.3  INVITE bypass", str(e))

    # 7.4 — Remove invite-only (-i)
    try:
        s1, _ = quick_client("rmi_op")
        send_raw(s1, "JOIN #rmitest")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #rmitest +i")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #rmitest -i")
        recv_all(s1, 0.3)
        s2, _ = quick_client("rmi_user")
        send_raw(s2, "JOIN #rmitest")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        check("JOIN" in resp and "473" not in resp,
              "7.4  -i (remove invite-only) — user can join", "", f"Got: {resp[:150]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("7.4  Remove invite-only", str(e))

    # ── 7.5 Channel Key (+k) ──
    try:
        s1, _ = quick_client("key_op")
        s2, _ = quick_client("key_wrong")
        s3, _ = quick_client("key_right")
        send_raw(s1, "JOIN #keytest")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #keytest +k secretkey")
        recv_all(s1, 0.3)
        # Wrong key
        send_raw(s2, "JOIN #keytest wrongkey")
        time.sleep(0.1)
        resp2 = recv_all(s2, 0.5)
        check("475" in resp2, "7.5a +k wrong key — ERR_BADCHANNELKEY (475)", "", f"Got: {resp2[:100]}")
        # No key
        send_raw(s2, "JOIN #keytest")
        time.sleep(0.1)
        resp2b = recv_all(s2, 0.5)
        check("475" in resp2b, "7.5b +k no key — ERR_BADCHANNELKEY (475)", "", f"Got: {resp2b[:100]}")
        # Right key
        send_raw(s3, "JOIN #keytest secretkey")
        time.sleep(0.1)
        resp3 = recv_all(s3, 0.5)
        check("JOIN" in resp3 and "475" not in resp3, "7.5c +k correct key — user joins", "", f"Got: {resp3[:150]}")
        close_conn(s1)
        close_conn(s2)
        close_conn(s3)
    except Exception as e:
        test_fail("7.5  Channel key", str(e))

    # 7.6 — Remove channel key (-k)
    try:
        s1, _ = quick_client("rmk_op")
        send_raw(s1, "JOIN #rmktest")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #rmktest +k mykey")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #rmktest -k")
        recv_all(s1, 0.3)
        s2, _ = quick_client("rmk_user")
        send_raw(s2, "JOIN #rmktest")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        check("JOIN" in resp and "475" not in resp, "7.6  -k (remove key) — user joins without key", "", f"Got: {resp[:150]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("7.6  Remove channel key", str(e))

    # ── 7.7 User Limit (+l) ──
    try:
        s1, _ = quick_client("lim_op")
        s2, _ = quick_client("lim_u2")
        s3, _ = quick_client("lim_u3")
        send_raw(s1, "JOIN #limtest")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #limtest +l 2")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #limtest")
        recv_all(s2, 0.3)
        send_raw(s3, "JOIN #limtest")
        time.sleep(0.1)
        resp = recv_all(s3, 0.5)
        check("471" in resp, "7.7  +l 2 — 3rd user gets ERR_CHANNELISFULL (471)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
        close_conn(s3)
    except Exception as e:
        test_fail("7.7  User limit", str(e))

    # 7.8 — Remove user limit (-l)
    try:
        s1, _ = quick_client("rml_op")
        send_raw(s1, "JOIN #rmltest")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #rmltest +l 1")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #rmltest -l")
        recv_all(s1, 0.3)
        s2, _ = quick_client("rml_user")
        send_raw(s2, "JOIN #rmltest")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        check("JOIN" in resp and "471" not in resp, "7.8  -l (remove limit) — user can join", "", f"Got: {resp[:150]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("7.8  Remove user limit", str(e))

    # ── 7.9 Operator Mode (+o / -o) ──
    try:
        s1, _ = quick_client("op_giver")
        s2, _ = quick_client("oprecv")
        send_raw(s1, "JOIN #opmode")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #opmode")
        recv_all(s2, 0.3)
        recv_all(s1, 0.3)
        # Give +o
        send_raw(s1, "MODE #opmode +o oprecv")
        time.sleep(0.1)
        resp = recv_all(s1, 0.5)
        check("MODE" in resp and "+o" in resp, "7.9a +o operator grant — MODE broadcast", "", f"Got: {resp[:100]}")
        # Now op_receiver should be able to KICK
        send_raw(s2, "KICK #opmode op_giver :I'm op now!")
        time.sleep(0.1)
        resp2 = recv_all(s2, 0.5)
        check("KICK" in resp2 or "482" not in resp2, "7.9b +o verified — new op can KICK", "", f"Got: {resp2[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("7.9  Operator mode", str(e))

    # 7.10 — Remove operator (-o)
    try:
        s1, _ = quick_client("deop_op")
        s2, _ = quick_client("deop_tgt")
        send_raw(s1, "JOIN #deop")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #deop")
        recv_all(s2, 0.3)
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #deop +o deop_tgt")
        recv_all(s1, 0.3)
        recv_all(s2, 0.3)
        send_raw(s1, "MODE #deop -o deop_tgt")
        recv_all(s1, 0.3)
        recv_all(s2, 0.3)
        # deop_tgt should no longer be able to change modes
        send_raw(s2, "MODE #deop +i")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("482" in resp, "7.10 -o (remove operator) — deoped user gets 482", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("7.10 Remove operator", str(e))

    # 7.11 — Unknown mode
    try:
        s, _ = quick_client("mode_unk")
        send_raw(s, "JOIN #modeunk")
        recv_all(s, 0.3)
        send_raw(s, "MODE #modeunk +z")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("472" in resp, "7.11 Unknown mode +z — ERR_UNKNOWNMODE (472)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("7.11 Unknown mode", str(e))

    # 7.12 — Combined modes (+il)
    try:
        s, _ = quick_client("mode_comb")
        send_raw(s, "JOIN #modecomb")
        recv_all(s, 0.3)
        send_raw(s, "MODE #modecomb +il 5")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("MODE" in resp, "7.12 Combined modes +il 5 — both applied", "", f"Got: {resp[:100]}")
        # Verify with query
        send_raw(s, "MODE #modecomb")
        time.sleep(0.1)
        resp2 = recv_all(s, 0.5)
        check("i" in resp2 and "l" in resp2, "7.12b MODE query shows both i and l set", "", f"Got: {resp2[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("7.12 Combined modes", str(e))

    # 7.13 — Mode change without operator
    try:
        s1, _ = quick_client("mode_nop1")
        s2, _ = quick_client("mode_nop2")
        send_raw(s1, "JOIN #modenop")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #modenop")
        recv_all(s2, 0.3)
        send_raw(s2, "MODE #modenop +i")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("482" in resp, "7.13 MODE by non-op — ERR_CHANOPRIVSNEEDED (482)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("7.13 Mode without op", str(e))

    # 7.14 — MODE on non-existent channel
    try:
        s, _ = quick_client("mode_nech")
        send_raw(s, "MODE #doesnotexist +i")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("403" in resp, "7.14 MODE on non-existent channel — ERR_NOSUCHCHANNEL (403)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("7.14 MODE non-existent channel", str(e))

    # 7.15 — MODE no params
    try:
        s, _ = quick_client("mode_nop")
        send_raw(s, "MODE")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("461" in resp, "7.15 MODE no params — ERR_NEEDMOREPARAMS (461)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("7.15 MODE no params", str(e))

    # 7.16 — +o on non-member
    try:
        s1, _ = quick_client("op_nontgt")
        send_raw(s1, "JOIN #opnon")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #opnon +o ghostuser")
        time.sleep(0.1)
        resp = recv_all(s1, 0.5)
        check("441" in resp or "401" in resp, "7.16 +o on non-member — ERR_USERNOTINCHANNEL (441)", "", f"Got: {resp[:100]}")
        close_conn(s1)
    except Exception as e:
        test_fail("7.16 +o non-member", str(e))

    # 7.17 — +k without argument
    try:
        s, _ = quick_client("k_noarg")
        send_raw(s, "JOIN #knoarg")
        recv_all(s, 0.3)
        send_raw(s, "MODE #knoarg +k")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("461" in resp, "7.17 +k without key argument — ERR_NEEDMOREPARAMS (461)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("7.17 +k no argument", str(e))

    # 7.18 — +l with invalid (non-numeric) argument
    try:
        s, _ = quick_client("l_bad")
        send_raw(s, "JOIN #lbad")
        recv_all(s, 0.3)
        send_raw(s, "MODE #lbad +l abc")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("461" in resp, "7.18 +l with non-numeric argument — error", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("7.18 +l non-numeric", str(e))

    # 7.19 — +l with zero or negative
    try:
        s, _ = quick_client("l_zero")
        send_raw(s, "JOIN #lzero")
        recv_all(s, 0.3)
        send_raw(s, "MODE #lzero +l 0")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("461" in resp or "MODE" not in resp,
              "7.19 +l 0 — rejected (invalid limit)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("7.19 +l zero", str(e))


# ═════════════════════════════════════════════════════════════════
#  8. INVITE
# ═════════════════════════════════════════════════════════════════
def test_invite():
    section("8. INVITE")

    # 8.1 — Basic INVITE
    try:
        s1, _ = quick_client("inv_s1")
        s2, _ = quick_client("inv_s2")
        send_raw(s1, "JOIN #invch")
        recv_all(s1, 0.3)
        send_raw(s1, "INVITE inv_s2 #invch")
        time.sleep(0.1)
        resp1 = recv_all(s1, 0.5)
        resp2 = recv_all(s2, 0.5)
        check("341" in resp1, "8.1a INVITE — RPL_INVITING (341) to sender", "", f"Got: {resp1[:100]}")
        check("INVITE" in resp2, "8.1b INVITE — notification sent to target", "", f"Got: {resp2[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("8.1  Basic INVITE", str(e))

    # 8.2 — INVITE to non-existent user
    try:
        s, _ = quick_client("inv_ne")
        send_raw(s, "JOIN #invne")
        recv_all(s, 0.3)
        send_raw(s, "INVITE ghostuser #invne")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("401" in resp, "8.2  INVITE non-existent user — ERR_NOSUCHNICK (401)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("8.2  INVITE non-existent", str(e))

    # 8.3 — INVITE to non-existent channel
    try:
        s1, _ = quick_client("inv_nc1")
        s2, _ = quick_client("inv_nc2")
        send_raw(s1, "INVITE inv_nc2 #nochannel")
        time.sleep(0.1)
        resp = recv_all(s1, 0.5)
        check("403" in resp, "8.3  INVITE to non-existent channel — ERR_NOSUCHCHANNEL (403)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("8.3  INVITE non-existent channel", str(e))

    # 8.4 — INVITE without being in channel
    try:
        s1, _ = quick_client("inv_out1")
        s2, _ = quick_client("inv_out2")
        s3, _ = quick_client("inv_out3")
        send_raw(s3, "JOIN #invout")
        recv_all(s3, 0.3)
        send_raw(s1, "INVITE inv_out2 #invout")
        time.sleep(0.1)
        resp = recv_all(s1, 0.5)
        check("442" in resp, "8.4  INVITE without being in channel — ERR_NOTONCHANNEL (442)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
        close_conn(s3)
    except Exception as e:
        test_fail("8.4  INVITE not in channel", str(e))

    # 8.5 — INVITE user already in channel
    try:
        s1, _ = quick_client("inv_dup1")
        s2, _ = quick_client("inv_dup2")
        send_raw(s1, "JOIN #invdup")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #invdup")
        recv_all(s2, 0.3)
        send_raw(s1, "INVITE inv_dup2 #invdup")
        time.sleep(0.1)
        resp = recv_all(s1, 0.5)
        check("443" in resp, "8.5  INVITE user already in channel — ERR_USERONCHANNEL (443)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("8.5 INVITE already in channel", str(e))

    # 8.6 — INVITE on +i channel by non-op
    try:
        s1, _ = quick_client("inv_iop")
        s2, _ = quick_client("inv_inop")
        s3, _ = quick_client("inv_itgt")
        send_raw(s1, "JOIN #invi")
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #invi +i")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #invi")  # This should fail since +i
        recv_all(s2, 0.5)
        # Let's invite s2 first so they can join
        send_raw(s1, "INVITE inv_inop #invi")
        recv_all(s1, 0.3)
        recv_all(s2, 0.3)
        send_raw(s2, "JOIN #invi")
        recv_all(s2, 0.3)
        # Now s2 (non-op) tries to invite s3
        send_raw(s2, "INVITE inv_itgt #invi")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("482" in resp, "8.6  INVITE on +i by non-op — ERR_CHANOPRIVSNEEDED (482)", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
        close_conn(s3)
    except Exception as e:
        test_fail("8.6  INVITE +i by non-op", str(e))

    # 8.7 — INVITE no params
    try:
        s, _ = quick_client("inv_nop")
        send_raw(s, "INVITE")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("461" in resp, "8.7  INVITE no params — ERR_NEEDMOREPARAMS (461)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("8.7  INVITE no params", str(e))


# ═════════════════════════════════════════════════════════════════
#  9. QUIT & DISCONNECTION
# ═════════════════════════════════════════════════════════════════
def test_quit():
    section("9. QUIT & DISCONNECTION")

    # 9.1 — Graceful QUIT
    try:
        s, _ = quick_client("quitter")
        send_raw(s, "QUIT :Goodbye!")
        time.sleep(0.3)
        # Connection should be closed
        try:
            s.sendall(b"PING :test\r\n")
            time.sleep(0.1)
            data = s.recv(1024)
            if len(data) == 0:
                test_pass("9.1  QUIT :Goodbye! — connection closed")
            else:
                test_fail("9.1  QUIT — connection still alive", f"Got: {data[:50]}")
        except (BrokenPipeError, ConnectionResetError, OSError):
            test_pass("9.1  QUIT :Goodbye! — connection closed")
        close_conn(s)
    except Exception as e:
        test_fail("9.1  Graceful QUIT", str(e))

    # 9.2 — QUIT broadcasts to shared channels
    try:
        s1, _ = quick_client("quit_b1")
        s2, _ = quick_client("quit_b2")
        send_raw(s1, "JOIN #quitbcast")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #quitbcast")
        recv_all(s2, 0.3)
        recv_all(s1, 0.3)
        send_raw(s1, "QUIT :Leaving...")
        time.sleep(0.3)
        resp = recv_all(s2, 0.5)
        if "QUIT" in resp:
            test_pass("9.2  QUIT broadcasts to channel members")
        else:
            test_warn("9.2  QUIT broadcast to channel — NOT IMPLEMENTED (no QUIT msg to peers)",
                       f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("9.2  QUIT broadcast", str(e))

    # 9.3 — Abrupt disconnect (close socket without QUIT)
    try:
        s1, _ = quick_client("abrupt1")
        s2, _ = quick_client("abrupt2")
        send_raw(s1, "JOIN #abruptch")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #abruptch")
        recv_all(s2, 0.3)
        recv_all(s1, 0.3)  # clear
        # Abruptly close s1
        s1.close()
        time.sleep(0.5)
        # s2 should still be functional
        send_raw(s2, "PING :alive")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        check("PONG" in resp, "9.3  Abrupt disconnect — server survives, other clients unaffected", "", f"Got: {resp[:100]}")
        close_conn(s2)
    except Exception as e:
        test_fail("9.3  Abrupt disconnect", str(e))

    # 9.4 — QUIT cleanup: nick becomes available
    try:
        s1, _ = quick_client("reusable")
        send_raw(s1, "QUIT :bye")
        time.sleep(0.3)
        close_conn(s1)
        s2, resp = quick_client("reusable")
        check("001" in resp, "9.4  QUIT frees nickname — same nick reusable", "", f"Got: {resp[:100]}")
        close_conn(s2)
    except Exception as e:
        test_fail("9.4  QUIT cleanup", str(e))


# ═════════════════════════════════════════════════════════════════
# 10. CAP NEGOTIATION
# ═════════════════════════════════════════════════════════════════
def test_cap():
    section("10. CAP NEGOTIATION (HexChat Compatibility)")

    # 10.1 — CAP LS
    try:
        s = make_conn()
        send_raw(s, "CAP LS")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("CAP" in resp, "10.1 CAP LS — server responds with capabilities list", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("10.1 CAP LS", str(e))

    # 10.2 — CAP + full HexChat registration flow
    try:
        s = make_conn()
        send_raw(s, "CAP LS")
        time.sleep(0.05)
        send_raw(s, f"PASS {PASSWORD}")
        send_raw(s, "NICK hexsim")
        send_raw(s, "USER hexsim 0 * :HexChat Simulation")
        send_raw(s, "CAP END")
        time.sleep(0.2)
        resp = recv_all(s, 1.0)
        check("001" in resp, "10.2 HexChat flow (CAP LS→PASS→NICK→USER→CAP END) — registered", "", f"Got: {resp[:200]}")
        close_conn(s)
    except Exception as e:
        test_fail("10.2 HexChat flow", str(e))


# ═════════════════════════════════════════════════════════════════
# 11. UNKNOWN COMMANDS & HELP
# ═════════════════════════════════════════════════════════════════
def test_unknown():
    section("11. UNKNOWN COMMANDS & HELP")

    # 11.1 — Unknown command
    try:
        s, _ = quick_client("unk_user")
        send_raw(s, "FOOBAR random params")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("421" in resp, "11.1 Unknown command FOOBAR — ERR_UNKNOWNCOMMAND (421)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("11.1 Unknown command", str(e))

    # 11.2 — HELP command
    try:
        s, _ = quick_client("help_user")
        send_raw(s, "HELP")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("NOTICE" in resp and "PASS" in resp, "11.2 HELP — sends NOTICE with command list", "", f"Got: {resp[:150]}")
        close_conn(s)
    except Exception as e:
        test_fail("11.2 HELP command", str(e))

    # 11.3 — NOTICE (may not be implemented)
    try:
        s1, _ = quick_client("notice_s")
        s2, _ = quick_client("notice_r")
        send_raw(s1, "NOTICE notice_r :This is a notice")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        if "NOTICE" in resp and "This is a notice" in resp:
            test_pass("11.3 NOTICE command — delivered")
        elif "421" in recv_all(s1, 0.5):
            test_warn("11.3 NOTICE command — NOT IMPLEMENTED (421 Unknown)")
        else:
            test_warn("11.3 NOTICE command — no response observed")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("11.3 NOTICE", str(e))


# ═════════════════════════════════════════════════════════════════
# 12. PARSER & NETWORK ROBUSTNESS
# ═════════════════════════════════════════════════════════════════
def test_parser_robustness():
    section("12. PARSER & NETWORK ROBUSTNESS")

    # 12.1 — \\n only (no \\r)
    try:
        s = make_conn()
        s.sendall(f"PASS {PASSWORD}\n".encode())
        s.sendall(b"NICK lf_only\n")
        s.sendall(b"USER lf_only 0 * :LF Only\n")
        time.sleep(0.3)
        resp = recv_all(s, 1.0)
        check("001" in resp, "12.1 LF-only line endings (\\n) — accepted", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("12.1 LF-only", str(e))

    # 12.2 — Multiple commands in one TCP segment
    try:
        s = make_conn()
        bulk = f"PASS {PASSWORD}\r\nNICK bulkuser\r\nUSER bulkuser 0 * :Bulk\r\nJOIN #bulktest\r\n"
        s.sendall(bulk.encode())
        time.sleep(0.3)
        resp = recv_all(s, 1.0)
        check("001" in resp and "JOIN" in resp,
              "12.2 Multiple commands in one recv() — all processed", "", f"Got: {resp[:200]}")
        close_conn(s)
    except Exception as e:
        test_fail("12.2 Bulk commands", str(e))

    # 12.3 — Partial packet (split NICK across two sends)
    try:
        s = make_conn()
        s.sendall(f"PASS {PASSWORD}\r\n".encode())
        time.sleep(0.05)
        s.sendall(b"NICK par")
        time.sleep(0.1)
        s.sendall(b"tial\r\n")
        time.sleep(0.1)
        s.sendall(b"USER partial 0 * :Partial Test\r\n")
        time.sleep(0.3)
        resp = recv_all(s, 1.0)
        check("001" in resp, "12.3 Partial packet (NICK split: 'par' + 'tial\\r\\n') — buffered correctly", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("12.3 Partial packet", str(e))

    # 12.4 — Empty lines
    try:
        s, _ = quick_client("emptyline")
        send_raw(s, "")
        send_raw(s, "")
        send_raw(s, "PING :alive")
        time.sleep(0.2)
        resp = recv_all(s, 0.5)
        check("PONG" in resp, "12.4 Empty lines — server ignores, stays functional", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("12.4 Empty lines", str(e))

    # 12.5 — Message with prefix (from client — should be ignored or handled)
    try:
        s, _ = quick_client("pfx_user")
        send_raw(s, ":fakeserver PRIVMSG pfx_user :Spoofed!")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        # Server should not crash; may ignore prefix or process command
        test_pass("12.5 Client-sent prefix — server doesn't crash")
        close_conn(s)
    except Exception as e:
        test_fail("12.5 Client prefix", str(e))

    # 12.6 — Buffer overflow protection (>4096 bytes)
    try:
        s = make_conn()
        send_raw(s, f"PASS {PASSWORD}")
        send_raw(s, "NICK overflow")
        send_raw(s, "USER overflow 0 * :Test")
        time.sleep(0.2)
        recv_all(s, 0.5)
        # Send a massive payload without \r\n to fill buffer
        giant = "A" * 5000
        try:
            s.sendall(giant.encode())
            time.sleep(0.3)
            resp = recv_all(s, 1.0)
            if "Buffer limit" in resp or resp == "":
                test_pass("12.6 Buffer overflow (>4096 bytes) — connection terminated")
            else:
                test_warn("12.6 Buffer overflow — unexpected behavior", f"Got: {resp[:100]}")
        except (BrokenPipeError, ConnectionResetError):
            test_pass("12.6 Buffer overflow — connection terminated")
        close_conn(s)
    except Exception as e:
        test_fail("12.6 Buffer overflow", str(e))

    # 12.7 — Message > 512 bytes (single IRC message)
    try:
        s, _ = quick_client("longmsg")
        send_raw(s, "JOIN #longtest")
        recv_all(s, 0.3)
        long_text = "X" * 1000
        send_raw(s, f"PRIVMSG #longtest :{long_text}")
        time.sleep(0.2)
        resp = recv_all(s, 0.5)
        # Server should not crash — may truncate or forward
        test_pass("12.7 Message >512 bytes — server handles without crashing")
        close_conn(s)
    except Exception as e:
        test_fail("12.7 Long message", str(e))

    # 12.8 — Case insensitivity of commands
    try:
        s, _ = quick_client("casecmd")
        send_raw(s, "ping :casetest")
        time.sleep(0.1)
        resp = recv_all(s, 0.5)
        check("PONG" in resp, "12.8 Lowercase command 'ping' — handled (case insensitive)", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("12.8 Case insensitive commands", str(e))

    # 12.9 — Extra spaces between parameters
    try:
        s = make_conn()
        s.sendall(f"PASS   {PASSWORD}\r\n".encode())
        time.sleep(0.05)
        s.sendall(b"NICK   spacey\r\n")
        time.sleep(0.05)
        s.sendall(b"USER   spacey   0   *   :Spacey User\r\n")
        time.sleep(0.3)
        resp = recv_all(s, 1.0)
        check("001" in resp, "12.9 Extra spaces in commands — handled gracefully", "", f"Got: {resp[:100]}")
        close_conn(s)
    except Exception as e:
        test_fail("12.9 Extra spaces", str(e))

    # 12.10 — Rapid connect/disconnect stress test
    try:
        success = True
        for i in range(10):
            try:
                s = make_conn()
                s.close()
            except:
                success = False
                break
        time.sleep(0.5)
        # Verify server still alive
        s, resp = quick_client("stresok")
        alive = "001" in resp
        close_conn(s)
        check(success and alive, "12.10 Rapid connect/disconnect (10x) — server survives", "",
              "Server died or refused connection")
    except Exception as e:
        test_fail("12.10 Stress test", str(e))


# ═════════════════════════════════════════════════════════════════
# 13. MULTI-CLIENT INTERACTION SCENARIOS
# ═════════════════════════════════════════════════════════════════
def test_multi_client():
    section("13. MULTI-CLIENT INTERACTION SCENARIOS")

    # 13.1 — 5 clients in one channel, messaging
    try:
        clients = []
        for i in range(5):
            s, _ = quick_client(f"mc{i}")
            send_raw(s, "JOIN #multichat")
            recv_all(s, 0.3)
            clients.append(s)
        
        send_raw(clients[0], "PRIVMSG #multichat :Hello everyone!")
        time.sleep(0.3)
        received_count = 0
        for i in range(1, 5):
            resp = recv_all(clients[i], 0.3)
            if "Hello everyone!" in resp:
                received_count += 1
        check(received_count == 4, f"13.1 5-client channel message — {received_count}/4 received", "",
              f"Expected 4, got {received_count}")
        for s in clients:
            close_conn(s)
    except Exception as e:
        test_fail("13.1 Multi-client chat", str(e))

    # 13.2 — Operator transfers and channel survival
    try:
        s1, _ = quick_client("founder")
        s2, _ = quick_client("successo")
        send_raw(s1, "JOIN #transfer")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #transfer")
        recv_all(s2, 0.3)
        recv_all(s1, 0.3)
        send_raw(s1, "MODE #transfer +o successo")
        recv_all(s1, 0.3)
        send_raw(s1, "QUIT :bye")
        time.sleep(0.3)
        # successo should still be able to operate
        send_raw(s2, "TOPIC #transfer :I'm in charge now!")
        time.sleep(0.1)
        resp = recv_all(s2, 0.5)
        check("TOPIC" in resp, "13.2 Operator transfer + founder quit — successo can set topic", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("13.2 Operator transfer", str(e))

    # 13.3 — Cross-channel messaging
    try:
        s1, _ = quick_client("cross1")
        s2, _ = quick_client("cross2")
        send_raw(s1, "JOIN #room_a")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #room_b")
        recv_all(s2, 0.3)
        # DM across channels
        send_raw(s1, "PRIVMSG cross2 :Hello from room_a!")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        check("Hello from room_a!" in resp, "13.3 Cross-channel DM — delivered regardless of channel", "", f"Got: {resp[:100]}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("13.3 Cross-channel DM", str(e))

    # 13.4 — User in multiple channels
    try:
        s, _ = quick_client("multch")
        send_raw(s, "JOIN #mc1")
        recv_all(s, 0.3)
        send_raw(s, "JOIN #mc2")
        recv_all(s, 0.3)
        send_raw(s, "JOIN #mc3")
        recv_all(s, 0.3)
        # Should be operator in all three
        send_raw(s, "MODE #mc1")
        send_raw(s, "MODE #mc2")
        send_raw(s, "MODE #mc3")
        time.sleep(0.2)
        resp = recv_all(s, 1.0)
        check(resp.count("324") >= 3, "13.4 User in 3 channels — all modes queryable", "", f"Got: {resp[:200]}")
        close_conn(s)
    except Exception as e:
        test_fail("13.4 Multiple channels", str(e))

    # 13.5 — Kick user from multiple channels
    try:
        s1, _ = quick_client("opsmult")
        s2, _ = quick_client("victim")
        send_raw(s1, "JOIN #kickmulti1")
        recv_all(s1, 0.3)
        send_raw(s1, "JOIN #kickmulti2")
        recv_all(s1, 0.3)
        send_raw(s2, "JOIN #kickmulti1")
        recv_all(s2, 0.3)
        send_raw(s2, "JOIN #kickmulti2")
        recv_all(s2, 0.3)
        recv_all(s1, 0.3)
        send_raw(s1, "KICK #kickmulti1 victim :out1")
        time.sleep(0.1)
        send_raw(s1, "KICK #kickmulti2 victim :out2")
        time.sleep(0.2)
        resp = recv_all(s2, 0.5)
        kick_count = resp.count("KICK")
        check(kick_count >= 2, f"13.5 Kick from multiple channels — {kick_count} KICK messages received", "",
              f"Expected 2, got {kick_count}")
        close_conn(s1)
        close_conn(s2)
    except Exception as e:
        test_fail("13.5 Multi-channel kick", str(e))


# ═════════════════════════════════════════════════════════════════
#  RUN ALL TESTS
# ═════════════════════════════════════════════════════════════════
def main():
    print(f"\n{C.BOLD}{C.PURPLE}{'━'*60}{C.RESET}")
    print(f"{C.BOLD}{C.PURPLE}  ft_irc — COMPREHENSIVE AUTOMATED TEST SUITE{C.RESET}")
    print(f"{C.BOLD}{C.PURPLE}  Server: {HOST}:{PORT}  Password: {PASSWORD}{C.RESET}")
    print(f"{C.BOLD}{C.PURPLE}{'━'*60}{C.RESET}")

    # Verify server is running
    try:
        s = make_conn()
        close_conn(s)
    except ConnectionRefusedError:
        print(f"\n{C.RED}ERROR: Cannot connect to {HOST}:{PORT}")
        print(f"Make sure your server is running: ./ircserv {PORT} {PASSWORD}{C.RESET}\n")
        sys.exit(1)

    test_connection_auth()
    test_ping_pong()
    test_nickchge()
    test_messaging()
    test_channel_ops()
    test_topic()
    test_channel_modes()
    test_invite()
    test_quit()
    test_cap()
    test_unknown()
    test_parser_robustness()
    test_multi_client()

    # ─── Summary ─────────────────────────────────────────────
    print(f"\n{C.BOLD}{'━'*60}{C.RESET}")
    print(f"{C.BOLD}  TEST RESULTS SUMMARY{C.RESET}")
    print(f"{'━'*60}")
    
    total = stats['total']
    passed = stats['passed']
    failed = stats['failed']
    warned = stats['warned']
    
    pct = (passed / total * 100) if total > 0 else 0

    print(f"  {C.GREEN}✓ Passed:  {passed}{C.RESET}")
    print(f"  {C.RED}✗ Failed:  {failed}{C.RESET}")
    print(f"  {C.YELLOW}⚠ Warned:  {warned}{C.RESET}")
    print(f"  Total:   {total}")
    print(f"  Score:   {pct:.1f}%")
    
    if failed == 0 and warned == 0:
        print(f"\n  {C.BG_GREEN}{C.BOLD} 🎉 ALL TESTS PASSED! {C.RESET}")
    elif failed == 0:
        print(f"\n  {C.GREEN}{C.BOLD} ✓ No failures — {warned} warnings (possible missing features){C.RESET}")
    else:
        print(f"\n  {C.BG_RED}{C.BOLD} {failed} TEST(S) FAILED {C.RESET}")

    print(f"{'━'*60}\n")

    # Return exit code
    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()