CC = gcc
LDFLAGS = -lcrypto
CFLAGS = -g -std=c99 -Wall -Wvla -fsanitize=address,undefined
SRC = detect_dups.c
TEST = file.c
TESTOUTPUT = classWalk
TARGET = detect_dups

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	
$(TESTOUTPUT): $(TEST)
	$(CC) $(CFLAGS) -o $(TESTOUTPUT) $(TEST) $(LDFLAGS)

# Clean rule
clean:
	rm -f $(TARGET)
	rm -f $(TESTOUTPUT)
