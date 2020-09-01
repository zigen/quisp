include ./quisp/Makefile

SRCS=./quisp/modules/*.cc ./quisp/rules/*.cc
tidy:
	clang-tidy $(SRCS) -- $(COPTS:-I.=-I./quisp)

format:
	clang-format $(SRCS) -i
