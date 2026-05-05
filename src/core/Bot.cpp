#include "Bot.hpp"
#include <ctime>
#include <sstream>


// ← constructor REMOVED

std::string Bot::handleCommand(const std::string& cmd, Client* sender, Server* server) 
{
    (void)sender;
    if (cmd == "!who")      return cmdWho(server);
    if (cmd == "!time")     return cmdTime();
    if (cmd == "!users")    return cmdUsers(server);
    if (cmd == "!channels") return cmdChannels(server);
    if (cmd == "!help")     return cmdHelp();
    return "Unknown command. Type !help for available commands.";
}

std::string Bot::cmdWho(Server* server)
{
    std::ostringstream oss;
    std::vector<Channel>& channels = server->getChannels(); // ← vector<Channel> not Channel*

    if (channels.empty())
        return "No channels found.";

    for (size_t i = 0; i < channels.size(); i++)
    {
        Channel& ch = channels[i];
        oss << "Channel: " << ch.getName();
        std::string topic = ch.getTopic();
        if (!topic.empty())
            oss << " | Topic: " << topic;
        else
            oss << " | Topic: (none)";
        oss << " | Members: ";
        std::vector<Client*>& members = ch.getMembers(); // ← getMembers()
        for (size_t j = 0; j < members.size(); j++)
        {
            oss << members[j]->get_nickname();
            if (j + 1 < members.size()) oss << ", ";
        }
        oss << "\n";
    }
    return oss.str();
}

std::string Bot::cmdUsers(Server* server)
{
    std::ostringstream oss;
    std::vector<Client>& clients = server->getClients(); // ← vector<Client> not Client*
    if (clients.empty())
        return "No users connected.";
    oss << "Connected users (" << clients.size() << "): ";
    for (size_t i = 0; i < clients.size(); i++)
    {
        oss << clients[i].get_nickname(); // ← dot not arrow
        if (i + 1 < clients.size()) oss << ", ";
    }
    return oss.str();
}

std::string Bot::cmdChannels(Server* server)
{
    std::ostringstream oss;
    std::vector<Channel>& channels = server->getChannels(); // ← vector<Channel>
    if (channels.empty())
        return "No channels exist yet.";
    oss << "Channels (" << channels.size() << "): ";
    for (size_t i = 0; i < channels.size(); i++)
        oss << channels[i].getName() << " "; // ← dot not arrow
    return oss.str();
}

std::string Bot::cmdTime() 
{
    std::time_t now = std::time(NULL);
    std::string t = std::ctime(&now);
    t.erase(t.find('\n'));
    return "Current server time: " + t;
}


std::string Bot::cmdHelp() 
{
    return  "=== IRC Bot Commands ===\n"
            "!who      - List all channels, members, and topics\n"
            "!time     - Show current server time\n"
            "!users    - List all connected users\n"
            "!channels - List all channels\n"
            "!help     - Show this help message\n"
            "=======================\n"
            "How to connect:\n"
            "  ./ircserv 8080 8080 1337\n"
            "  PASS 1337\n"
            "  NICK yournick\n"
            "  USER yournick * 0 :Your Name";
}
