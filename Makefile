NAME = ircserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -g

SRC = main.cpp Server.cpp Client.cpp Channel.cpp\
		cmds/handleJoin.cpp cmds/handlePart.cpp cmds/handlePing.cpp\
		cmds/handlePrivmsg.cpp cmds/handleQuit.cpp\
		commands/handleCommands.cpp commands/handleInvite.cpp\
		commands/handleKick.cpp commands/handleMode.cpp\
		commands/handleTopic.cpp commands/utils.cpp
		
OBJ = $(SRC:%.cpp=obj/%.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

obj/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re