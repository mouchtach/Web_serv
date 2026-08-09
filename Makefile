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
	http/response.cpp \
	http/httpexception.cpp 

OBJ = $(SRC:.cpp=.o)

# ── colors ──────────────────────────────────────────────
RESET   = \033[0m
BOLD    = \033[1m
GREEN   = \033[32m
YELLOW  = \033[33m
BLUE    = \033[34m
RED     = \033[31m
CYAN    = \033[36m
MAGENTA = \033[35m

TOTAL := $(words $(SRC))

all: banner $(NAME)
	@mkdir -p uploads
	@echo "$(GREEN)$(BOLD)[OK]$(RESET) $(GREEN)webserv is built and ready — ./$(NAME)$(RESET)\n"

banner:
	@echo "$(CYAN)$(BOLD)┌──────────────────────────────────────────┐$(RESET)"
	@echo "$(CYAN)$(BOLD)│   building webserv   ($(TOTAL) source files)$(RESET)"
	@echo "$(CYAN)$(BOLD)└──────────────────────────────────────────┘$(RESET)"

$(NAME): $(OBJ)
	@echo "$(MAGENTA)[LINK]$(RESET) linking objects -> $(BOLD)$(NAME)$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)
	@echo "$(GREEN)[DONE]$(RESET) binary created"

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@
	@echo "$(BLUE)[CC]$(RESET)   $<"

clean:
	@rm -f $(OBJ)
	@echo "$(YELLOW)[CLEAN]$(RESET) object files removed"

fclean: clean
	@rm -f $(NAME)
	@rm -rf uploads
	@echo "$(RED)[FCLEAN]$(RESET) binary and uploads/ removed"

clean_users:
	@rm -rf cgi/users.json
	@echo "$(RED)[CLEAN]$(RESET) cgi/users.json removed"

re: fclean all

.PHONY: all clean fclean re clean_users banner