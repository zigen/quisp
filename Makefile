include ./quisp/Makefile

SRCS=./quisp/modules/*.cc ./quisp/rules/*.cc
tidy:
	clang-tidy $(SRCS) -- $(COPTS)

format:
	clang-format $(SRCS) -i
