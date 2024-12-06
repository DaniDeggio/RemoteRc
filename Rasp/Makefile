# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dde-giov <dde-giov@student.42roma.it>      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/05/20 17:37:53 by dde-giov          #+#    #+#              #
#    Updated: 2024/12/06 03:11:15 by dde-giov         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME	= Rasp

CC := c++
FLAGS := -Wall -Wextra -Werror -lwiringPi
RM := rm -f
LINKFLAGS := -lwiringPi

# OpenCV specific flags
OPENCV_INCLUDE := -I/usr/include/opencv4
OPENCV_LIBS := $(shell pkg-config --libs opencv4)

OBJSDIR = objects

# COLORS
CLR_RMV := \033[0m
RED := \033[1;31m
GREEN := \033[1;32m
YELLOW := \033[1;33m
BLUE := \033[1;34m
CYAN := \033[1;36m

SRC :=	srcs/Rasp.cpp \

OBJS := $(addprefix $(OBJSDIR)/, $(SRC:.cpp=.o))

all: $(NAME)

$(OBJSDIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CC) $(FLAGS) $(OPENCV_INCLUDE) -c $< -o $@

$(NAME): $(OBJS)
	@echo "$(GREEN)Compilation $(CLR_RMV)of $(YELLOW)$(NAME) $(CLR_RMV)..."
	@$(CC) $(FLAGS) $(OBJS) $(LINKFLAGS) $(OPENCV_LIBS) -o $(NAME)
	@echo "$(GREEN)$(NAME) created [0m ✔️"

clean:
	@$(RM) $(OBJS)
	@echo "$(RED)Deleting $(CYAN)$(NAME) $(CLR_RMV)objs ✔️"

fclean: clean
	@$(RM) $(NAME) -rf $(OBJSDIR)
	@echo "$(RED)Deleting $(CYAN)$(NAME) $(CLR_RMV)binary ✔️"

re: fclean all

.PHONY: all clean fclean re
