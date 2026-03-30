##
## EPITECH PROJECT, 2025
## nanotekspice
## Makefile
##

NAME        = myteams_cli

CXX         = clang++

INCLUDES = \
    -I../libs/myteams \
    -I../common \
    -I../common/utils \

LIBS = \
	-L../libs/myteams \
	## This mean the link will only work if the binary is at the root of the repo. \
	## (since it's a .so, can't do smth else)
	-Wl,-rpath,'$$ORIGIN/libs/myteams' -lmyteams

BASE_FLAGS  = -Wall -Wextra -std=c++20
CXXFLAGS    = $(BASE_FLAGS) $(INCLUDES)

SRC             = $(shell find . -name '*.cpp')

OBJ             = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS) $(LIBS)
	mv $(NAME) ../

clean:
	rm -rf $(OBJ)

fclean: clean
	rm -rf ../$(NAME)

re: fclean all

