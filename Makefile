NAME     = ircserv
CXX      := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98 -I./includes

#-I./includes : tells the compiler to look in the includes/ folder when resolving headers

OBJDIR  = build
COREDIR = src/core
CMDSDIR = src/cmds
INCDIR  = includes

HEADERS = $(INCDIR)/Channel.hpp $(INCDIR)/Client.hpp $(INCDIR)/Server.hpp

CORE_SRC = main.cpp Server.cpp Client.cpp Channel.cpp Bot.cpp

CMDS_SRC = handleCmds.cpp handleMode.cpp handlePart.cpp handleJoin.cpp \
           handlePing.cpp handlePrivmsg.cpp handleQuit.cpp handleInvite.cpp \
           handleKick.cpp handleTopic.cpp utils.cpp


SRC = $(addprefix $(COREDIR)/, $(CORE_SRC)) $(addprefix $(CMDSDIR)/, $(CMDS_SRC))

OBJ = $(addprefix $(OBJDIR)/, $(SRC:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@echo "\nCreated executable '$(NAME)'"

# @mkdir -p $(dir $@) It needs to create build/src/core/ and build/src/cmds/ subdirectories before compiling into them.

$(OBJDIR)/%.o: %.cpp Makefile $(HEADERS) | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	@mkdir -p $(OBJDIR)

clean:
	@rm -rf $(OBJDIR)

fclean: clean
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re run

run:
	./$(NAME) 8080 1337

# pass 1337
# nick ajelloul
# user ajelloul * 0 : Aboubakr Jelloulat
