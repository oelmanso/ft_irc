NAME = ircbot

CCX = g++
CFLAGSX = -Wall -Wextra -Werror -std=c++98 -g3

SRCS = main.cpp Bot.cpp

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

