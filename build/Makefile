CXX = g++
CFLAGS = -std=c++11 -Wall -g

TARGET = server
OBJS = ../code/log/*.cc ../code/pool/*.cc ../code/timer/*.cc \
       ../code/http/*.cc ../code/server/*.cc \
       ../code/buffer/*.cc ../code/main.cc

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -lpthread -lmysqlclient

clean:
	rm -rf ../bin/$(OBJS) $(TARGET)

# CXX = g++
# CFLAGS = -std=c++11 -Wall -g

# TARGET = webserver
# SRCDIR = ../code
# BINDIR = ../bin
# SRCS = $(wildcard $(SRCDIR)/log/*.cc $(SRCDIR)/pool/*.cc $(SRCDIR)/timer/*.cc \
#                  $(SRCDIR)/http/*.cc $(SRCDIR)/server/*.cc \
#                  $(SRCDIR)/buffer/*.cc $(SRCDIR)/main.cc)
# OBJS = $(patsubst $(SRCDIR)/%.cc,$(BINDIR)/%.o,$(SRCS))

# $(BINDIR)/$(TARGET): $(OBJS)
# 	$(CXX) $(CFLAGS) $^ -o $@ -lpthread -lmysqlclient

# $(BINDIR)/%.o: $(SRCDIR)/%.cc
# 	mkdir -p $(dir $@)
# 	$(CXX) $(CFLAGS) -c $< -o $@

# clean:
# 	rm -rf $(BINDIR)/*.o $(BINDIR)/$(TARGET)
