CC = gcc -O3 -march=native -flto -lm
#CC = gcc -fsanitize=undefined,address -g


CFLAGS = -Wall -Wextra -g
CFLAGS += -Iinclude

TARGET = bot
OBJ_DIR = obj

LOCAL_SRC  := $(wildcard src/*.c)

SOURCES := $(LOCAL_SRC) $(CORE_SRC)

OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SOURCES)))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ 

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
