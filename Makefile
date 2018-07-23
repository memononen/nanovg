# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: tmilon <tmilon@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2017/11/07 18:38:31 by tmilon            #+#    #+#              #
#    Updated: 2018/05/30 10:27:32 by cpieri           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME	=	lnvg.a

CC		=	gcc

CFLAGS	=	-Wall -Wextra -Werror

INCS	=	fontstash.h			\
			nanovg.h			\
			nanovg_gl.h			\
			nanovg_gl_utils.h	\
			stb_image.h			\
			stb_truetype.h

HEADERS	=	./libft.h

SRC_PATH=	src

OBJ_PATH=	obj

CPPFLAG	=	-Iinclude

SRCS = 		nanovg.c

OBJS	=	$(SRCS:.c=.o)

SRC		=	$(addprefix $(SRC_PATH)/,$(SRCS) )

OBJ		=	$(addprefix $(OBJ_PATH)/,$(OBJS) )

.PHONY: all clean fclean re

all:		$(NAME)

$(NAME): 	$(OBJ)
				@echo "compiling libnvg.a"
				@ar rc $(NAME) $(OBJ)
				@ranlib $(NAME)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
				@mkdir $(OBJ_PATH) 2> /dev/null || true
				@$(CC) $(CFLAGS) $(CPPFLAG) -o $@ -c $<

clean:
				@echo "cleaning nvg"
				@/bin/rm -f $(OBJ)
				@rmdir $(OBJ_PATH) 2> /dev/null || true

fclean:		clean
				@/bin/rm -f $(NAME)

re:			fclean all
