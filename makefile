CC = gcc
LDFLAGS = -lcrypto
CFLAGS = -g -std=c99 -Wall -Wvla -fsanitize=address,undefined

SRC = detect_dups.c
TARGET = detect_dups

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Clean rule
clean:
	rm -f $(TARGET)