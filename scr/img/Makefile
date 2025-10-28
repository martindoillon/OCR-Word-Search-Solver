
NAME = preprocess


CC = gcc


CFLAGS = -Wall -Wextra  -I/opt/homebrew/include

LDFLAGS = -L/opt/homebrew/lib -lSDL2 -lSDL2_image -lm

SRC = preprocess.c

OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(OBJ) -o $(NAME) $(LDFLAGS)

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

run: $(NAME)
	./$(NAME)

.PHONY: all clean fclean re run

