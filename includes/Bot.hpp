#pragma once
#include <string>
#include <vector>
#include <map>
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

class Bot 
{
public:
    static std::string handleCommand(const std::string& cmd, Client* sender, Server* server);

private:
    static std::string cmdWho(Server* server);
    static std::string cmdTime();
    static std::string cmdUsers(Server* server);
    static std::string cmdChannels(Server* server);
    static std::string cmdHelp();
};