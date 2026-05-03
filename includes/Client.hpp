# pragma once

#include <string>

class Client {
	private:
		int			client_socket;
		bool		pass;
		bool		nick;
		bool		user;
		std::string	buffer;
		std::string	nickname;
		std::string	username;
		std::string	realname;
	public:
		Client(int _client_socket);
		int get_client_fd();
		void set_buffer(std::string _string);
		std::string &get_buffer();
		bool pass_status() const;
		bool nick_status() const;
		bool user_status() const;
		void set_pass(bool status);
		void set_nick(bool status);
		void set_user(bool status);
		void set_username(const std::string &u);
		void set_realname(const std::string &r);
		std::string get_username() const;
		std::string get_realname() const;
		std::string get_nickname();
		void set_nickname(std::string _string);
		bool is_registered() const;
};