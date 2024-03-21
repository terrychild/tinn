# config
TARGET := tinn
RUN_ARGS := ../moohar/www

COMP_ARGS := -Wall -Wextra -Werror -std=c17 -pedantic

# dirs
BUILD := ./build
SRC := ./src

# build list of source files
SRCS := $(shell find $(SRC) -name "*.c")
# turn source file names into object file names
OBJS := $(SRCS:$(SRC)/%=$(BUILD)/tmp/%)
OBJS := $(OBJS:.c=.o)
# and dependacy file names
DEPS := $(OBJS:.o=.d)

# build list of sub directories in src
INC := $(shell find $(SRC) -type d)
INC_ARGS := $(addprefix -I,$(INC))

# short cuts
.PHONY: build run trace clean
build: $(BUILD)/$(TARGET)
run: build
	@$(BUILD)/$(TARGET) $(RUN_ARGS)
trace: build
	@$(BUILD)/$(TARGET) -v $(RUN_ARGS)
clean:
	@rm -r $(BUILD)

# link .o objects into an executible
$(BUILD)/$(TARGET): $(OBJS)
	@$(CC) $(COMP_ARGS) $(OBJS) -o $@

# complile .c source into .o object files
$(BUILD)/tmp/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(COMP_ARGS) -MMD -MP $(INC_ARGS) -c $< -o $@

# includes the .d makefiles we made when compiling .c source files
-include $(DEPS)