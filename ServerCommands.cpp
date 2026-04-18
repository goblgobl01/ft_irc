#include "Server.hpp"
#include "Channel.hpp"

/*
check a client is fully registered (completed PASS + NICK + USER)
Client (a connected user)

client.get_client_fd(); => the socket fd, used to send msg to THIS client
client.get_nickname(); => used in IRC messages and error replies
client.get_username(); => used in the prefix :nick!user@localhost
client.is_registered(); => true only when PASS + NICK + USER all done

always check if the client is regitered first
always check if the channel exists
always check if the client is in the channel before doing anything
and for operator commands, check if it is an operator

*/

