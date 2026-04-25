#include "Client.hpp"
#include <iostream>

Client::Client(int _client_socket) : client_socket(_client_socket), pass(false), nick(false), user(false){}

int Client::get_client_fd() {
	return (client_socket);
}

void Client::set_buffer(std::string _string) {
	this->buffer.append(_string);
}

std::string &Client::get_buffer() {
	return (buffer);
}

bool Client::pass_status() const { return this->pass; }
bool Client::nick_status() const { return this->nick; }
bool Client::user_status() const { return this->user; }
void Client::set_pass(bool status) { this->pass = status; }
void Client::set_nick(bool status) { this->nick = status; }
void Client::set_user(bool status) { this->user = status; }
void Client::set_username(const std::string &u) { this->username = u; }
void Client::set_realname(const std::string &r) { this->realname = r; }
std::string Client::get_username() const { return this->username; }
std::string Client::get_realname() const { return this->realname; }
std::string Client::get_nickname() { return this->nickname; }
void Client::set_nickname(std::string _string) { this->nickname = _string; }
bool Client::is_registered() const
{
	std::cout << "pass: " << this->pass << "nick: " << this->nick << "user: " << this->user << std::endl;
	return (this->pass && this->nick && this->user);
}