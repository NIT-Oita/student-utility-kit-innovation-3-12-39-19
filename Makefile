CC = gcc
TARGET = anking.exe
SRC = main.c

$(TARGET): $(SRC)
	$(CC) -Wall -o $(TARGET) $(SRC)

clean:
	del $(TARGET)