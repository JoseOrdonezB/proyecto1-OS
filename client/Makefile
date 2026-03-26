CC     = gcc
FLAGS  = -Wall -Wextra -pthread
COMMON = ../common/protocol.h

# ── Archivos fuente del cliente ──
CLIENT_SRC = client.c receiver.c ui.c
CLIENT_OUT = client

# ── Directorios de includes ──
INCLUDES = -I../common

# ────────────────────────────────────────────────────────────────────────────

all: $(CLIENT_OUT)

$(CLIENT_OUT): $(CLIENT_SRC) $(COMMON)
	$(CC) $(FLAGS) $(INCLUDES) -o $(CLIENT_OUT) $(CLIENT_SRC)

# ── Limpia el ejecutable ──
clean:
	rm -f $(CLIENT_OUT)

# ── Recompila desde cero ──
re: clean all

.PHONY: all clean re
