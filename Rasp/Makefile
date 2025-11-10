# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dde-giov <dde-giov@student.42roma.it>      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2024/05/20 17:37:53 by dde-giov          #+#    #+#              #
#    Updated: 2025/11/10 04:46:58 by dde-giov         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME	= Rasp
TEST_NAME = steering_test

CC := c++
FLAGS := -Wall -Wextra -Werror -lwiringPi
RM := rm -f
LINKFLAGS := -lwiringPi
#OPENCV_FLAGS := `pkg-config --cflags --libs opencv4`
#RASPICAM_FLAGS := -I/usr/local/include
#RASPICAM_LIBS := -L/usr/local/lib -lraspicam -lraspicam_cv -lmmal -lmmal_core -lmmal_util
# LIBCAMERA_FLAGS := -I/usr/include/libcamera
# LIBCAMERA_LIBS := -L/usr/lib/arm-linux-gnueabihf -lstdc++ -lboost_system -lboost_filesystem -lboost_program_options

OBJSDIR = objects

# COLORS
CLR_RMV := \033[0m
RED := \033[1;31m
GREEN := \033[1;32m
YELLOW := \033[1;33m
BLUE := \033[1;34m
CYAN := \033[1;36m

SRC :=	srcs/Cam.cpp \
		srcs/CarControll.cpp \
		srcs/Main.cpp \

OBJS := $(addprefix $(OBJSDIR)/, $(SRC:.cpp=.o))
TEST_OBJS := $(OBJSDIR)/tests/SteeringSweep.o

all: $(NAME)

$(OBJSDIR)/%.o: %.cpp
	mkdir -p $(@D)
	$(CC) $(FLAGS) -c $< -o $@

$(NAME): $(OBJS)
	@echo "$(GREEN)Compilation $(CLR_RMV)of $(YELLOW)$(NAME) $(CLR_RMV)..."
	@$(CC) $(FLAGS) $(OBJS) $(LINKFLAGS) -o $(NAME)
	@echo "$(GREEN)$(NAME) created [0m ✔️"

test: $(TEST_OBJS)
	@echo "$(GREEN)Compilation $(CLR_RMV)of $(YELLOW)$(TEST_NAME) $(CLR_RMV)..."
	@$(CC) $(FLAGS) $(TEST_OBJS) $(LINKFLAGS) -o $(TEST_NAME)
	@echo "$(GREEN)$(TEST_NAME) created [0m ✔️"

clean:
	@$(RM) $(OBJS)
	@echo "$(RED)Deleting $(CYAN)$(NAME) $(CLR_RMV)objs ✔️"

fclean: clean
	@$(RM) $(NAME) $(TEST_NAME) -rf $(OBJSDIR)
	@echo "$(RED)Deleting $(CYAN)$(NAME) $(CLR_RMV)binary ✔️"

re: fclean all

.PHONY: all clean fclean re test
