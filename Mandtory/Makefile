# Makefile

NAME = ircserv

CCX = g++
CFLAGSX = -Wall -Wextra -Werror -std=c++98 #-g3

SRCS = main.cpp server.cpp client.cpp A_Channel.cpp cmd_Channel.cpp parser.cpp signal.cpp helper.cpp join.cpp mode.cpp registration.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CCX) $(CFLAGSX) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CCX) $(CFLAGSX) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

.SECONDARY: