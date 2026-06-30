NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g

SRC = 	src/main.cpp \
		src/webserv.cpp \
		config/Config.cpp \
		config/LocationConfig.cpp \
		config/ParssingConf.cpp \
		config/serverConfig.cpp \
		server/utils.cpp \
		server/server.cpp \
		server/client.cpp \
		http/request.cpp \
		http/response.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all