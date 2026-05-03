NAME = ircserv

CC = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98 -g

SRC = src/main.cpp src/Server.cpp src/Client.cpp src/Channel.cpp\
		src/cmds/handleMode.cpp src/cmds/handleCmds.cpp src/cmds/utils.cpp src/cmds/handlePart.cpp\
		src/cmds/handleJoin.cpp src/cmds/handlePing.cpp src/cmds/handlePrivmsg.cpp src/cmds/handleQuit.cpp\
		src/cmds/handleInvite.cpp src/cmds/handleKick.cpp src/cmds/handleTopic.cpp
		
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