# meant for UNIX systems only
# todo: dependecy files aren't working for some peculiar reason
# (e.g. editing common.h doesn't change the timestamp)

CXX       =  g++

LDPATH    =  /usr/local/lib/
#LDPATH    = /usr/lib64/

# file extensions
BINEXT    =  out
OBJEXT    =  o
DEPEXT    =  d
SRCEXT    =  cpp
STLIBEXT  =  a
DYNLIBEXT =  so

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

DEP_MSG   = echo -n "$(BOLD)Creating dependency for $(patsubst $(SRCDIR)/%.$(SRCEXT),$(OBJDIR)/%.$(OBJEXT),$(1)) ... $(RESET)"
COMP_MSG  = echo -n "$(BOLD)Compiling $(1) ... $(RESET)"
LD_MSG    = echo -n "$(BOLD)Linking $(1) ... $(RESET)"

FAIL_MSG  = echo "$(1)" | GREP_COLOR='01;31' egrep --color=always -E '^|error' | GREP_COLOR='00;38;5;226'  egrep --color=always -E '^|warning'

# create folder if it doesn't exist
DIR       =  \
             if [ ! -d "$(1)" ]; then \
                     echo -n "$(BOLD)Creating directory $(1)/ ... ";\
                     mkdir $(1);\
                     echo "$(OK)";\
             fi

# compilation flags
INCLUDE    =  
LDFLAGS    =  `root-config --libs --ldflags`
BOOSTFLAGS =  $(LDPATH)libboost_program_options.$(DYNLIBEXT)
LDFLAGS    += $(BOOSTFLAGS)
CXXFLAGS   =  `root-config --cflags`
CXXFLAGS   += -g -O3 -Wall -Wextra -Werror

# project files
SRCS      =  
OBJS      =  $(SRCS:%=$(OBJDIR)/%.$(OBJEXT))
TARGET    =  process histoplot efficiency copytree sample

# makefile rules
all: $(TARGET:%=$(BINDIR)/%.$(BINEXT))

# target binary
$(TARGET:%=$(BINDIR)/%.$(BINEXT)): $(BINDIR)/%.$(BINEXT): $(OBJDIR)/%.$(OBJEXT) $(OBJS)
	@$(call DIR,$(BINDIR))
	@$(call LD_MSG,$<)
	@_ERROR=$$($(CXX) $^ $(CXXFLAGS) $(LDFLAGS) -o $@ 2>&1); \
	if [ $$? -eq 0 ]; then echo "$(OK)"; \
	else $(call FAIL_MSG,$(FAIL)\n$$_ERROR); exit 1; fi

# target object files
$(patsubst %,$(OBJDIR)/%.$(OBJEXT),$(TARGET)): $(OBJDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT) $(DEPDIR)/%.$(DEPEXT)
	@$(call DIR,$(OBJDIR))
	@$(call COMP_MSG,$<)
	@_ERROR=$$($(CXX) -c $(INCLUDE) $(CXXFLAGS) $< -o $@ 2>&1); \
	if [ $$? -eq 0 ]; then echo "$(OK)"; \
	else $(call FAIL_MSG,$(FAIL)\n$$_ERROR); exit 1; fi

# other object files
$(OBJS): $(OBJDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT) $(DEPDIR)/%.$(DEPEXT)
	@$(call DIR,$(OBJDIR))
	@$(call COMP_MSG,$<)
	@_ERROR=$$($(CXX) -c $(INCLUDE) $(CXXFLAGS) $< -o $@ 2>&1); \
	if [ $$? -eq 0 ]; then echo "$(OK)"; \
	else $(call FAIL_MSG,$(FAIL)\n$$_ERROR); exit 1; fi

# dependency files
$(DEPDIR)/%.$(DEPEXT): $(SRCDIR)/%.$(SRCEXT)
	@$(call DIR,$(DEPDIR))
	@$(call DEP_MSG,$<)
	@_ERROR=$$($(CXX) $(CXXFLAGS) -MM -MT '$(patsubst $(SRCDIR)/%.$(SRCEXT),$(OBJDIR)/%.$(OBJEXT),$<)' $< -MF $@ 2>&1); \
	if [ $$? -eq 0 ]; then echo "$(OK)"; \
	else $(call FAIL_MSG,$(FAIL)\n$$_ERROR); exit 1; fi

# dependency files into separate dir
DEPS    = $(SRCS:%=$(DEPDIR)/%.$(DEPEXT))
CLEAN   = clean
DOCS    = docs
NODEPS  = $(CLEAN) $(DOCS)

ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
	-include $(DEPS)
endif

.PHONY: clean
$(CLEAN):
	@$(foreach var,$(DELDIR), echo -n "$(BOLD)Removing $(var)/ ... "\
				&& rm -rf $(var) && echo "$(OK)";)

#$(DOCS):
#	doxygen Doxyfile
