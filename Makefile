CC = gcc
TARGET = anking.exe
SRC = main.c

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

clean:
	del $(TARGET)