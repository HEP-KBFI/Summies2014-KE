# meant for UNIX systems only

CXX = g++

LDPATH    =  /usr/local/lib/

# file extensions
BINEXT    =  out
OBJEXT    =  o
DEPEXT    =  d
SRCEXT    =  cpp
STLIBEXT  =  a

# project dirs
SRCDIR    = src
OBJDIR    = obj
BINDIR    = bin
RESDIR    = res
DEPDIR    = dep

DELDIR    = $(OBJDIR) $(BINDIR) $(DEPDIR)

# colored output
BOLD      = $(shell tput bold)
RED       = $(shell tput setaf 1)
GREEN     = $(shell tput setaf 2)
RESET     = $(shell tput sgr0)

OK        = $(BOLD)$(GREEN)[OK]$(RESET)
FAIL      = $(BOLD)$(RED)[FAIL]$(RESET)

DEP_MSG   = echo "$(BOLD)Creating dependency for $(patsubst $(SRCDIR)/%.$(SRCEXT),$(OBJDIR)/%.$(OBJEXT),$(1)) ... $(RESET)$(2)"
COMP_MSG  = echo "$(BOLD)Compiling $(1) ... $(RESET)$(2)"
LD_MSG    = echo "$(BOLD)Linking $(1) ... $(RESET)$(2)"

# create folder if it doesn't exist
DIR       =  \
             if [ ! -d "$(1)" ]; then \
                     echo -n "$(BOLD)Creating directory $(1)/ ... ";\
                     mkdir $(1);\
                     echo "$(OK)";\
             fi

# compilation flags
INCLUDE   =  
LDFLAGS   =  `root-config --libs` $(LDPATH)libboost_program_options.$(STLIBEXT)
CXXFLAGS  =  `root-config --cflags`
CXXFLAGS  += -Wall -Wextra -Werror -g

# project files
SRCS    = InputData FilePointer
OBJS    = $(SRCS:%=$(OBJDIR)/%.$(OBJEXT))
TARGET  = main

# makefile rules
all: $(BINDIR)/$(TARGET).$(BINEXT)

# target binary
$(BINDIR)/$(TARGET).$(BINEXT): $(BINDIR)/%.$(BINEXT): $(OBJDIR)/%.$(OBJEXT) $(OBJS)
	@$(call DIR,$(BINDIR))
	@_ERROR=$$($(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@ 2>&1); \
	if [ $$? -eq 0 ]; then $(call LD_MSG,$<,$(OK)); \
	else $(call LD_MSG,$<,$(FAIL)\n$$_ERROR); exit 1; fi

# target object files
$(patsubst %,$(OBJDIR)/%.$(OBJEXT),$(TARGET)): $(OBJDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT) $(DEPDIR)/%.$(DEPEXT)
	@$(call DIR,$(OBJDIR))
	@_ERROR=$$($(CXX) -c $(INCLUDE) $(CXXFLAGS) $< -o $@ 2>&1); \
	if [ $$? -eq 0 ]; then $(call COMP_MSG,$<,$(OK)); \
	else $(call COMP_MSG,$<,$(FAIL)\n$$_ERROR); exit 1; fi

# other object files
$(OBJS): $(OBJDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT) $(DEPDIR)/%.$(DEPEXT)
	@$(call DIR,$(OBJDIR))
	@_ERROR=$$($(CXX) -c $(INCLUDE) $(CXXFLAGS) $< -o $@ 2>&1); \
	if [ $$? -eq 0 ]; then $(call COMP_MSG,$<,$(OK)); \
	else $(call COMP_MSG,$<,$(FAIL)\n$$_ERROR); exit 1; fi

# dependency files
$(DEPDIR)/%.$(DEPEXT): $(SRCDIR)/%.$(SRCEXT)
	@$(call DIR,$(DEPDIR))
	@_ERROR=$$($(CXX) $(CXXFLAGS) -MM -MT '$(patsubst $(SRCDIR)/%.$(SRCEXT),$(OBJDIR)/%.$(OBJEXT),$<)' $< -MF $@ 2>&1); \
	if [ $$? -eq 0 ]; then $(call DEP_MSG,$<,$(OK)); \
	else $(call DEP_MSG,$<,$(FAIL)\n$$_ERROR); exit 1; fi

# dependency files into separate dir
DEPS    = $(SRCS:%=$(DEPDIR)/%.$(DEPEXT))
NODEPS  = clean docs

ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
	-include $(DEPS)
endif

.PHONY: clean
clean:
	@$(foreach var,$(DELDIR), echo -n "$(BOLD)Removing $(var)/ ... "\
				&& rm -rf $(var) && echo "$(OK)";)

docs:
	doxygen Doxyfile