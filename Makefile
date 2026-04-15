SERVER_NAME = myteams_server
CLIENT_NAME = myteams_cli

LIB_DIR     = ./libs/myteams

CXX         = g++
CXXFLAGS    = -Wall -Wextra -std=c++20 -I. -I./common -I./common/utils -I./server -I./server/core -I./client -I$(LIB_DIR)
LDFLAGS     = -L$(LIB_DIR) -lmyteams -luuid

SERVER_SRC  = $(shell find ./server -name "*.cpp") \
              $(shell find ./common -name "*.cpp")

CLIENT_SRC  = $(shell find ./client -name "*.cpp") \
              $(shell find ./common -name "*.cpp")

SERVER_OBJ  = $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ  = $(CLIENT_SRC:.cpp=.o)

all: $(SERVER_NAME) $(CLIENT_NAME)

$(SERVER_NAME): $(SERVER_OBJ)
	$(CXX) $(SERVER_OBJ) -o $(SERVER_NAME) $(LDFLAGS)

$(CLIENT_NAME): $(CLIENT_OBJ)
	$(CXX) $(CLIENT_OBJ) -o $(CLIENT_NAME) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ)
	find . -type f -name "*.gcda" -delete
	find . -type f -name "*.gcno" -delete
	find . -type f -name "*.gcov" -delete

fclean: clean
	rm -f $(SERVER_NAME) $(CLIENT_NAME)

re: fclean all
