CC      = gcc
FLAGS   = -Wall -Wextra -pthread
COMMON_DIR = ../common
CLIENT_DIR = ../client
INCLUDES   = -I$(COMMON_DIR) -I$(CLIENT_DIR)

# --- Archivos fuente ---
SERVER_SRC = server.c client_handler.c user_list.c
# Agregamos los archivos del cliente uno por uno para rastrear el error
C1 = $(CLIENT_DIR)/client.c
C2 = $(CLIENT_DIR)/receiver.c
C3 = $(CLIENT_DIR)/ui.c

all: check_files server_exe client_exe

# Esta regla nos dirá si los archivos existen antes de intentar compilar
check_files:
	@ls $(C1) > /dev/null || echo " ERROR: No se encuentra $(C1)"
	@ls $(C2) > /dev/null || echo " ERROR: No se encuentra $(C2)"
	@ls $(C3) > /dev/null || echo " ERROR: No se encuentra $(C3)"

server_exe: $(SERVER_SRC)
	$(CC) $(FLAGS) $(INCLUDES) -o server_exe $(SERVER_SRC)

client_exe: $(C1) $(C2) $(C3)
	$(CC) $(FLAGS) $(INCLUDES) -o client_exe $(C1) $(C2) $(C3)

clean:
	rm -f server_exe client_exe

re: clean all

.PHONY: all clean re check_files