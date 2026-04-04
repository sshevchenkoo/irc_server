NAME := ircserv

CXX := c++
CXXFLAGS := -std=c++98 -Wall -Wextra -Werror

OBJ_DIR := obj
DEP_DIR := deps

SRCS := \
	main.cpp \
	Client/Client.cpp \
	Server/Server.cpp \
	Utils/utils.cpp \
	irc/Irc.cpp \
	irc/Channel.cpp \
	irc/helpers.cpp

OBJS := $(addprefix $(OBJ_DIR)/,$(notdir $(SRCS:.cpp=.o)))
DEPS := $(addprefix $(DEP_DIR)/,$(notdir $(SRCS:.cpp=.d)))

all: $(NAME)

$(NAME): $(OBJ_DIR) $(DEP_DIR) $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: Client/%.cpp
	$(CXX) $(CXXFLAGS) -MMD -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: Server/%.cpp
	$(CXX) $(CXXFLAGS) -MMD -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: Utils/%.cpp
	$(CXX) $(CXXFLAGS) -MMD -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: irc/%.cpp
	$(CXX) $(CXXFLAGS) -MMD -MF $(DEP_DIR)/$*.d -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(DEP_DIR):
	mkdir -p $(DEP_DIR)

clean:
	rm -rf $(OBJ_DIR) $(DEP_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

-include $(DEPS)
