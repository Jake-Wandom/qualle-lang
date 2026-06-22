TARGET := quallcom

# Directories
SRC_DIR := src
INC_DIR := include
OBJ_DIR := build/obj
BIN_DIR := build/bin
DEP_DIR := build/dep


# Compiler and Flags
CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wstrict-prototypes -I$(INC_DIR) # removed -Wmissing-prototypes
LDFLAGS := 
LIBS :=

# Debug (make DEBUG=1)
ifdef DEBUG
	CFLAGS += -g3 -O0 -DDEBUG -fsanitize=address,undefined
	LDFLAGS += -fsanitize=address,undefined
else
	CFLAGS += -O2 -DNDEBUG
endif

# allow args
ifeq (run,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "run"
  RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  # ...and turn them into do-nothing targets
  $(eval $(RUN_ARGS):;@:)
endif


# Sources
SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
DEPS := $(patsubst $(SRC_DIR)/%.c, $(DEP_DIR)/%.d, $(SRCS))

# Binary
BINARY := $(BIN_DIR)/$(TARGET)


# Default Target
.PHONY: all
all: $(BINARY)
$(BINARY): $(OBJS) | $(BIN_DIR)
	$(CC) $(shell llvm-config --ldflags --libs core) $(LDFLAGS) $^ -o $@ $(LIBS)
	@echo "Linked -> $@"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEP_DIR)
	@mkdir -p $(dir $@) $(dir $(DEP_DIR)/$*.d)
	$(CC) $(shell llvm-config --cflags) $(CFLAGS) -MMD -MF $(DEP_DIR)/$*.d -c $< -o $@
	@echo "Compiled $<"

-include $(DEPS)

# Utility Targets
.PHONY: run
run: all
	./$(BINARY) $(RUN_ARGS)

.PHONY: clean
clean:
	rm -rf build/

.PHONY: check
check: CFLAGS += -DTEST
check: all
	@echo "Running tests..."
	./$(BINARY) --test

.PHONY: install
install: all
	install -Dm755 $(BINARY) /usr/local/bin/$(TARGET)

.PHONY: format
format:
	clang-format -i $(shell find $(SRC_DIR) $(INC_DIR) -name '*.[ch]')

.PHONY: tidy
tidy:
	clang-tidy $(SRCS) -- $(CFLAGS)

.PHONY: info
info:
	@echo "Target: $(TARGET)"
	@echo "Binary: $(BINARY)"
	@echo "Sources: $(SRCS)"
	@echo "CFLAGS: $(CFLAGS)"

# Directory Creation
$(BIN_DIR) $(OBJ_DIR) $(DEP_DIR):
	mkdir -p $@