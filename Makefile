#OBJS specifies which files to compile as part of the project
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

#CC specifies which compiler we're using
CC = gcc

#COMPILER_FLAGS specifies the additional compilation options we're using
# -w suppresses all warnings
COMPILER_FLAGS := -O2 -Wall --std=gnu11 $(shell sdl2-config --cflags) -g

#LINKER_FLAGS specifies the libraries we're linking against
LINKER_FLAGS := $(shell sdl2-config --libs) -lSDL2_mixer -lSDL2_ttf

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = chip8emu

#This is the target that compiles our executable
all : $(OBJ_NAME)

clean:
	rm -f *.o *.map $(OBJ) $(OBJ_NAME)

$(OBJ_NAME): $(OBJ)
	$(CC) $(SRC) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME) 
