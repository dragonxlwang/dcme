SHELL := /bin/bash
CC = gcc
DEBUG ?= 0
NOEXEC ?= 0


#Using -Ofast instead of -O3 might result in faster code,
#but is supported only by newer GCC versions
ifeq ($(DEBUG), 0)
	override CFLAGS += -std=gnu99 -lm -pthread -O3 -march=native -Wall \
										 -funroll-loops  -Wno-unused-variable
	DIR = bin
else
	override CFLAGS += -lm -pthread -O0 -g3 -DDEBUG
	DIR = debug
	BIN = gdb
endif

%: %.c
	@echo -en "\033[1;31m"; printf '==== (RE)MAKE '; printf '=%.0s' {1..66};
	@echo -e "\033[0m"

	-rm -f $(DIR)/$@
	mkdir -p $(dir $(DIR)/$@)
	$(CC) $< -o $(DIR)/$@ $(CFLAGS)

ifeq ($(NOEXEC), 0)
	$(eval STR_LENGTH :=  $(shell echo $(DIR)/$@ | wc -m))
	$(eval STR_LENGTH :=  $(shell echo 70 - $(STR_LENGTH) | bc))
	@echo -en "\033[1;36m"; printf '==== RUN: '; echo -n "$(DIR)/$@ "
	@printf '=%.0s' {1..$(STR_LENGTH)}; echo -e "\033[0m"
	@$(BIN) ./$(DIR)/$@
	@echo -en "\033[1;36m"; printf '=%.0s' {1..80}; echo -e "\033[0m"
else
	@echo -en "\033[1;31m"; printf '=%.0s' {1..80}; echo -e "\033[0m"
endif

clean:
	rm -rf $(DIR)/*

# $@ The name of the target file (the one before the colon)
# $< The name of the first (or only) prerequisite file
# $^ The names of all the prerequisite files (space separated)
# $* The stem (the bit which matches the % wildcard in the rule definition.
