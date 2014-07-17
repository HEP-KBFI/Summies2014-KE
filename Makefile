CXX = g++

LDPATH    =  /usr/local/lib/

BINEXT    =  out
OBJEXT    =  o
DEPEXT    =  d
SRCEXT    =  cpp
STLIBEXT  =  a

SRCDIR    = src
OBJDIR    = obj
BINDIR    = bin
RESDIR    = res

INCLUDE   =  
LDFLAGS   =  `root-config --libs` $(LDPATH)libboost_program_options.$(STLIBEXT)
CXXFLAGS  =  `root-config --cflags`
CXXFLAGS  += -Wall -Wextra -Werror -g -MMD

SRCS    = InputData FilePointer
OBJS    = $(SRCS:%=$(OBJDIR)/%.$(OBJEXT))
DEPS    = $(SRCS:%=$(OBJDIR)/%.$(DEPEXT))
TARGET  = main

all: $(TARGET)

# target binary
$(TARGET): %: $(OBJDIR)/%.$(OBJEXT) $(OBJS)
	@test -d $(BINDIR) || (echo -n "Creating directory $(BINDIR)/ ... "\
	                       && mkdir -p $(BINDIR) && echo "[OK]")
	@echo -n "Linking $@ ... "
	@$(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $(BINDIR)/$@.$(BINEXT)
	@echo "[OK]"

# target object files
$(patsubst %,$(OBJDIR)/%.$(OBJEXT),$(TARGET)): $(OBJDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@test -d $(OBJDIR) || (echo -n "Creating directory $(OBJDIR)/ ... "\
	                       && mkdir -p $(OBJDIR) && echo "[OK]")
	@echo -n "Compiling $@ ... "
	@$(CXX) -c $(INCLUDE) $(CXXFLAGS) $< -o $@
	@echo "[OK]"

# other object files
$(OBJS): $(OBJDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@test -d $(OBJDIR) || (echo -n "Creating directory $(OBJDIR)/ ... "\
	                       && mkdir -p $(OBJDIR) && echo "[OK]")
	@echo -n "Compiling $@ ... "
	@$(CXX) -c $(INCLUDE) $(CXXFLAGS) $< -o $@
	@echo "[OK]"

.PHONY: clean
clean:
	@echo -n "Removing $(OBJDIR)/ ... "
	@rm -rf $(OBJDIR)
	@echo "[OK]"
	@echo -n "Removing $(BINDIR)/ ... "
	@rm -rf $(BINDIR)
	@echo "[OK]"

docs:
	doxygen Doxyfile