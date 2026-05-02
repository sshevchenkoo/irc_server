This project has been created as part
of the 42 curriculum by yashevch, rkobelie.

# IRC Server

# Description
This project is a C++98 implementation of an IRC (Internet Relay Chat) server developed as part of the 42 curriculum.
The goal of the project is to build a fully functional IRC server that handles multiple clients using a single-threaded, event-driven architecture based on poll() and non-blocking sockets (O_NONBLOCK). The server supports core IRC features such as client registration, messaging, channel management, and operator privileges.
A C++98 IRC server using `poll()`-based I/O multiplexing. Compatible with the [Halloy](https://halloy.chat) IRC client.
## Project Structure


irc_server/
├── main.cpp                  # Entry point: argument parsing, server startup
├── Makefile                  # Build configuration
|│
├── Server/
│   ├── Server.hpp / .cpp     # Event loop (poll), client and channel management
│
├── Client/
│   ├── Client.hpp / .cpp     # Client state: buffers, nick, registration, ping/pong
│
├── irc/
│   ├── Irc.hpp / .cpp        # Command dispatcher, message parser
│   ├── Channel.hpp / .cpp    # Channel logic: members, operators, modes
│   └── helpers.cpp           # IRC command handler implementations
│
├── Utils/
│   └── utils.hpp / .cpp      # parsePort, signal_handler, split, etc.
│
├── obj/                      # Compiled object files (auto-generated)
└── deps/                     # Dependency files for incremental builds


## Architecture

Single-threaded event loop powered by `poll()` with non-blocking sockets (`O_NONBLOCK`).

`Server` — holds the listen socket, `poll` fd list, and maps of `fd → Client` and `name → Channel`.

`Client` — one TCP connection: I/O buffers, registration state (`PASS`/`NICK`/`USER`), ping/pong tracking.

`Channel` — sets of members, operators and invited clients; bitmask of active modes; key, topic, limit.

`IRC` — static dispatcher: parses raw lines into `command { prefix, cmd, params, trailing }` and routes to handlers via `map<string, handler>`.

### Connection lifecycle


accept()
  └─► Client created with fd + IP
        └─► PASS + NICK + USER
              └─► tryRegister() → send 001 welcome
                    └─► JOIN / PRIVMSG / MODE / ...
                          └─► QUIT or ping/pong timeout → removeClient()


### Ping/Pong timeout

Every `poll()` cycle calls `tick()`. If a client is inactive for 240 s the server sends `PING :tick`. If no `PONG` is received within 250 s the client is disconnected.

# Instructions

## Build

`Requirements:` C++98-compatible compiler, POSIX OS (Linux / macOS).

      bash
make          # build
make re       # clean rebuild
make clean    # remove object files
make fclean   # remove object files + binary


Output binary: `ircserv`



## Running

   bash
./ircserv <port> <password>


Example:

   bash
./ircserv 13195 pass


The server listens on all interfaces (`0.0.0.0`). Stop with `Ctrl+C`.


## Supported Commands

### Registration

| Command | Syntax | Notes |

| `PASS` | `PASS <password>` | Must be sent first |
| `NICK` | `NICK <nickname>` | Max 9 characters |
| `USER` | `USER <username> 0 * :<realname>` | Completes registration |
| `CAP` | `CAP LS` | Server responds with empty capability list |

Registration is complete once `PASS`, `NICK`, and `USER` are all accepted — server sends `001`.

### Messaging

| Command | Syntax | Notes |

| `PRIVMSG` | `PRIVMSG <target> :<text>` | Target is a nick or `#channel` |

### Channels

| Command | Syntax | Notes |

| `JOIN` | `JOIN <#channel> [key]` | Creates channel if it doesn't exist |
| `PART` | `PART <#channel> [:<reason>]` | Leave a channel |
| `TOPIC` | `TOPIC <#channel> [:<text>]` | Get or set channel topic |
| `MODE` | `MODE <#channel> <+/-modes> [params]` | Manage channel modes |
| `INVITE` | `INVITE <nick> <#channel>` | Operators only |
| `KICK` | `KICK <#channel> <nick> [:<reason>]` | Operators only |
| `PING` | `PING <token>` | Keep-alive |
| `PONG` | `PONG <token>` | Reply to PING |
| `QUIT` | `QUIT [:<reason>]` | Disconnect |


## Channel Modes

| Flag | Name | Param | Description |

| `i` | Invite Only | — | Only invited users can join |
| `t` | Topic Lock | — | Only operators can change the topic |
| `l` | User Limit | `<n>` | Maximum number of members |
| `k` | Key | `<key>` | Channel password |
| `o` | Operator | `<nick>` | Grant or revoke operator status |


MODE #general +i
MODE #general +k secretkey
MODE #general +l 10
MODE #general +o alice
MODE #general -i


## Connecting with Halloy

[Halloy](https://halloy.chat) is an open-source IRC client written in Rust, configured via a TOML file.

### Opening the config file

In Halloy, go to **Settings → Open Config File**. This opens `config.toml` in your default text editor. Paste the configuration below, save the file, then restart Halloy.

### Configuration

   toml
ping_time = 10

[servers.rostek]
use_tls = false
server = "localhost"
port = 13195
username = "rkobelie"
realname = "Rostyslav"
password = "pass"
channels = ["#general"]

[servers.yarek]
use_tls = false
server = "localhost"
port = 13195
nickname = "yarek"
username = "yashevch"
realname = "Michal"
password = "pass"
channels = ["#general"]


### Steps

1. Start the server: `./ircserv 13195 pass`
2. In Halloy open **Settings → Open Config File**
3. Paste the config above and save
4. Restart Halloy — it will connect and auto-join `#general`

### Useful in-client commands


/join #general
/topic #general Welcome!
/mode #general +i
/invite bob #general
/kick #general bob reason
/quit Goodbye

## Numeric Replies

| Code | Name | Description |

| `001` | `RPL_WELCOME` | Successful registration |
| `324` | `RPL_CHANNELMODEIS` | Current channel modes |
| `331` | `RPL_NOTOPIC` | No topic set |
| `332` | `RPL_TOPIC` | Channel topic |
| `341` | `RPL_INVITING` | Invite sent |
| `353` | `RPL_NAMREPLY` | Channel member list |
| `366` | `RPL_ENDOFNAMES` | End of member list |
| `401` | `ERR_NOSUCHNICK` | Unknown nick |
| `403` | `ERR_NOSUCHCHANNEL` | Unknown channel |
| `431` | `ERR_NONICKNAMEGIVEN` | No nick provided |
| `432` | `ERR_ERRONEUSNICKNAME` | Invalid nick |
| `433` | `ERR_NICKNAMEINUSE` | Nick already taken |
| `442` | `ERR_NOTONCHANNEL` | Not in channel |
| `461` | `ERR_NEEDMOREPARAMS` | Missing parameters |
| `464` | `ERR_PASSWDMISMATCH` | Wrong password |
| `471` | `ERR_CHANNELISFULL` | Channel is full |
| `473` | `ERR_INVITEONLYCHAN` | Invite-only channel |
| `475` | `ERR_BADCHANNELKEY` | Wrong channel key |
| `482` | `ERR_CHANOPRIVNEEDED` | Operator privileges required |


## Technical Details



Standard  C++98
Compiler flags  `-Wall -Wextra -Werror`
I/O model  `poll()` + `O_NONBLOCK`
Connection queue 100
Poll timeout 200 ms
Read buffer 1024 bytes
Max message length 510 chars
Server name `SuperServ` (override via `-DSERVERNAME`)

# Resources
## Documentation & References

- RFC 1459 — Internet Relay Chat Protocol
- RFC 2812 — IRC Client Protocol
- poll() and socket programming (POSIX documentation)
- Beej’s Guide to Network Programming
- man pages
- Non-blocking I/O и event loops