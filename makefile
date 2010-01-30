SOURCE_DIR = ./src/
OBJECT_DIR = ./obj/

SOURCE_FILES = $(wildcard $(SOURCE_DIR)*.c)

OBJECTS = $(subst .c,.o,$(subst $(SOURCE_DIR),$(OBJECT_DIR),$(SOURCE_FILES)))
SOURCES = $(OBJECT_FILE:xx=$(SOURCE_DIR))

CC		= cc
CCOUT	= -o
CCOPT	= -c

vtags:	$(OBJECTS)
	@$(CC) $(CCOUT) vtags $(OBJECTS) -g

$(OBJECTS): $(SOURCE_FILES) $(SOURCE_DIR)vtags.h
	@$(CC) $(CCOPT) $(CCOUT)$(@) $(SOURCE_DIR)$(*F).c -g

clean:
	-@rm $(OBJECTS)
	-@rm vtags
