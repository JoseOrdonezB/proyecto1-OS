CC     = gcc
FLAGS  = -Wall -Wextra -pthread
COMMON = ../common/protocol.h

# ── Archivos fuente del servidor ──
SERVER_SRC = server.c client_handler.c user_list.c
SERVER_OUT = server

# ── Directorios de includes ──
INCLUDES = -I../common

# ────────────────────────────────────────────────────────────────────────────

all: $(SERVER_OUT)

$(SERVER_OUT): $(SERVER_SRC) $(COMMON)
	$(CC) $(FLAGS) $(INCLUDES) -o $(SERVER_OUT) $(SERVER_SRC)

# ── Limpia el ejecutable ──
clean:
	rm -f $(SERVER_OUT)

# ── Recompila desde cero ──
re: clean all

.PHONY: all clean re