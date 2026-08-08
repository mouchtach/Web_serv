NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g


SRC = main.cpp \
	parssing/configparssing.cpp \
	parssing/location.cpp \
	parssing/config.cpp \
	src/redirectException.cpp \
	src/webserv.cpp \
	src/client.cpp \
	src/server.cpp \
	src/static_utils.cpp \
	http/request.cpp \
	http/response.cpp 

OBJ = $(SRC:.cpp=.o)

all: $(NAME)
	@mkdir -p uploads

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)
	rm -rf uploads
clean_users:
	rm -rf cgi/users.json

re: fclean all