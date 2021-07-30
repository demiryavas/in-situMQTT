# ------------------------------------------------
# Generic Makefile
#
# Author: yanick.rochon@gmail.com
# Date  : 2011-08-10
#
# Changelog :
#   2010-11-05 - first version
#   2011-08-10 - added structure : sources, objects, binaries
#                thanks to http://stackoverflow.com/users/128940/beta
# ------------------------------------------------

# project name and version (generate library with this name with and without version)
MAJOR := 1
MINOR := 0
MICRO := 0
NAME := mqtt_insp
VERSION := $(MAJOR).$(MINOR).$(MICRO)
TARGET   = lib$(NAME).so.$(VERSION)
TARGET2  = lib$(NAME).so

CC       = gcc
# compiling flags here
# get from environment settings
CFLAGS   = $(CPPFLAGS) -fPIC -g -c  -Wall -I.
$(info CFLAGS is $(CFLAGS))

LINKER   = gcc
# linking flags here
LFLAGS   = -shared -rdynamic -Wall 
LIB_DIRS = -L../lib 
LIBS = -lpthread -lm 

# change these to set the proper directories where each files shoould be
SRCDIR   = .
OBJDIR   = obj
LIBDIR   = ../lib

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -f
MKDIR_P = mkdir -p

# create obj directory if does not exist
#${OBJDIR}:
#	${MKDIR_P} ${OBJDIR}

$(info $(SOURCES))

# to activate versioning activate the following line 
# and and the last line then deactivate the next line
# for TARGET2
$(LIBDIR)/$(TARGET2): $(OBJECTS)
	$(LINKER) $(OBJECTS) $(LFLAGS) -o $@ $(LIB_DIRS) $(LIBS)
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONEY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!"

.PHONEY: remove
remove: clean
	@$(rm) $(LIBDIR)/$(TARGET2)
	@echo "library removed!"

