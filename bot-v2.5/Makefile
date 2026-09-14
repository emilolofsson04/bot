CC = gcc -O3 -march=native -flto
#CC = gcc -fsanitize=undefined,address -g


CFLAGS = -Wall -Wextra -g
CFLAGS += -Icommon/include
CFLAGS += -Iinclude

TARGET = bot
OBJ_DIR = obj

LOCAL_SRC  := $(wildcard src/*.c)
CORE_SRC   := $(wildcard common/src/*.c)

SOURCES := $(LOCAL_SRC) $(CORE_SRC)

OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(notdir $(SOURCES)))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ 

$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: common/src/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(OBJ_DIR) $(TARGET)
