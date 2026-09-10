CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = dns

all: $(TARGET)

$(TARGET): src/main.c src/parse_dns.c src/argument_parse.c src/read_filter.c src/response_dns.c src/utils_dns.c 
	$(CC) $(CFLAGS) -o $(TARGET) src/main.c src/parse_dns.c src/argument_parse.c src/read_filter.c src/response_dns.c src/utils_dns.c 

clean:
	rm -f $(TARGET)