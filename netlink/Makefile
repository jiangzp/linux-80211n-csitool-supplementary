all: print_packets  parse_log log_to_file

KERNEL = $(strip $(shell uname -r))
KERNEL_INCLUDE = -I /usr/src/linux-headers-$(KERNEL)/include
CFLAGS = -Wall -Werror $(KERNEL_INCLUDE) -lm 
CC = gcc

log_to_file: log_to_file.c
print_packets: print_packets.c
 parse_log:  parse_log.c

clean:
	rm -f log_to_file print_packets parse_log
