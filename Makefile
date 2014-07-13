CXX = g++

INCLUDE   = 
LDFLAGS   = `root-config --libs` -lboost_program_options
CXXFLAGS  = `root-config --cflags`

BINEXT = .out
OBJEXT = .o

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