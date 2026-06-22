CC = gcc
TARGET = anking.exe
SRC = main.c

$(TARGET): $(SRC)
	$(CC) -Wall -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	del $(TARGET)