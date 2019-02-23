# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dde-jesu <marvin@42.fr>                    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2018/11/06 16:01:17 by dde-jesu          #+#    #+#              #
#    Updated: 2019/02/23 10:55:29 by dde-jesu         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME=ft_select
CFLAGS=-Wall -Wextra -Iinclude -Ilibft/include -g -fsanitize=address
CC=gcc

include src.mk

OBJS=$(SRCS:.c=.o)

all: $(NAME)

$(OBJS): Makefile src.mk

$(NAME): $(OBJS)
	$(MAKE) -C libft libft.a
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS) libft/libft.a -lncurses

clean:
	$(MAKE) -C libft clean
	rm -f $(OBJS)

fclean: clean
	$(MAKE) -C libft fclean
	rm -rf $(NAME)

re: fclean all


.PHONY: clean fclean re proto
