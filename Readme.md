# Chat en C — Sistemas Operativos
Universidad del Valle de Guatemala  
Curso: Sistemas Operativos — 2026

---

## Descripción

Aplicación de chat desarrollada en C que permite la comunicación en tiempo real entre múltiples usuarios a través de sockets TCP. El sistema está compuesto por un servidor y un cliente, ambos con soporte de multithreading para manejo concurrente de conexiones y mensajes.

El servidor mantiene la lista de usuarios conectados y se encarga de enrutar los mensajes entre clientes. El cliente provee una interfaz interactiva para enviar mensajes, consultar usuarios y cambiar de estado.

### Funcionalidades

- Registro y desconexión de usuarios
- Mensajes broadcast (a todos los usuarios)
- Mensajes directos (privados entre dos usuarios)
- Listado de usuarios conectados
- Información de un usuario específico
- Cambio de estado (ACTIVE, BUSY, INACTIVE)
- Detección automática de inactividad

---

## Estructura de archivos

```
chat/
├── common/
│   └── protocol.h          # Definición del protocolo compartido (ChatPacket)
├── server/
│   ├── Makefile
│   ├── server.c             # Punto de entrada del servidor
│   ├── client_handler.c/h  # Lógica de manejo de cada cliente (por thread)
│   └── user_list.c/h       # Lista enlazada de usuarios conectados (thread-safe)
└── client/
    ├── Makefile
    ├── client.c             # Punto de entrada del cliente
    ├── receiver.c/h         # Thread receptor de mensajes del servidor
    └── ui.c/h               # Interfaz de usuario en terminal
```

---

## Compilar y correr

### Requisitos
- GCC
- Make
- Linux (probado en Ubuntu 24.04)

### Servidor

```bash
cd server
make
./server <puerto>
```

Ejemplo:
```bash
./server 8080
```

### Cliente

```bash
cd client
make
./client <usuario> <IP_servidor> <puerto>
```

Ejemplo local:
```bash
./client juan 127.0.0.1 8080
```


## Uso del cliente

Al conectarse se muestra el siguiente menú:

```
╔══════════════════════════════════════╗
║         MENÚ DE OPCIONES             ║
╠══════════════════════════════════════╣
║  1. Enviar mensaje a todos           ║
║  2. Enviar mensaje privado           ║
║  3. Cambiar estado                   ║
║  4. Ver usuarios conectados          ║
║  5. Ver info de un usuario           ║
║  6. Ayuda                            ║
║  7. Salir                            ║
╚══════════════════════════════════════╝
```