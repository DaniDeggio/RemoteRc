# Variabili
CXX = x86_64-w64-mingw32-g++
CXXFLAGS = -Wall -Wextra -I libs/include -L libs/lib
OBJ_DIR = objects
SRC = srcs/Client.cpp
OBJ = $(OBJ_DIR)/Client.o
TARGET = Client.exe

# Regole
all: $(TARGET)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ): $(OBJ_DIR) $(SRC)
	$(CXX) $(CXXFLAGS) -c $(SRC) -o $(OBJ)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -lmingw32 -lSDL2main -lSDL2 -lws2_32 -o $(TARGET)

clean:
	if exist $(OBJ_DIR) rmdir /s /q $(OBJ_DIR)
	if exist $(TARGET) del $(TARGET)

re : clean all

.PHONY: all clean
