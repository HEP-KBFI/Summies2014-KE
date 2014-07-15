CXX = g++

LDPATH    =  /usr/local/lib/

BINEXT    =  .out
OBJEXT    =  .o
STLIBEXT  =  .a

INCLUDE   = 
LDFLAGS   =  `root-config --libs` $(LDPATH)libboost_program_options$(STLIBEXT)
CXXFLAGS  =  `root-config --cflags`
CXXFLAGS  += -Wall -Wextra -Werror

SRCS    = main.cxx
OBJS    = $(SRCS:%.cxx=%$(OBJEXT))
TARGET  = main

all: $(TARGET)$(BINEXT)

$(TARGET)$(BINEXT): %$(BINEXT): %$(OBJEXT) $(OBJS)
	@echo -n "Linking $@ ... "
	@$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@
	@echo "[OK]"

$(patsubst %,%$(OBJEXT),$(TARGET)): %$(OBJEXT): %.cxx
	@echo -n "Compiling $@ ... "
	@$(CXX) -c $(INCLUDE) $(CXXFLAGS) $< -o $@
	@echo "[OK]"

.PHONY: clean
clean:
	@echo -n "Cleaning ... "
	@rm -rf *.o *.out
	@echo "[OK]"