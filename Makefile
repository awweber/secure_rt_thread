# Makefile for secure_rt_thread.c

CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lpthread -lrt
TARGET = secure_rt_thread
SERVER_TARGET = secure_rt_server
CLIENT_TARGET = test_client
SRC = secure_rt_thread.c
SERVER_SRC = secure_rt_server.c
CLIENT_SRC = test_client.c

# Standard-Target
all: $(TARGET) $(SERVER_TARGET) $(CLIENT_TARGET)

# Kompilieren - Original
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Kompilieren - Server
$(SERVER_TARGET): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_SRC) $(LDFLAGS)

# Kompilieren - Client
$(CLIENT_TARGET): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_SRC)

# Testen - Original
test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET) || true

# Server starten
server: $(SERVER_TARGET)
	@echo "Starting secure RT server..."
	@./$(SERVER_TARGET)

# Client-Test
client: $(CLIENT_TARGET)
	@echo "Starting test client..."
	@./$(CLIENT_TARGET)

# Aufräumen
clean:
	rm -f $(TARGET) $(SERVER_TARGET) $(CLIENT_TARGET)


# hilfe
help:
	@echo "Makefile for secure_rt_thread"
	@echo "Usage:"
	@echo "  make          - Build all projects (original + server + client)"
	@echo "  make test     - Run original authentication test"
	@echo "  make server   - Start the secure RT server"
	@echo "  make client   - Start test client"
	@echo "  make clean    - Clean up build files"
	@echo "  make help     - Show this help message"
	@echo ""
	@echo "Client-Server Testing:"
	@echo "  Terminal 1: make server"
	@echo "  Terminal 2: make client"
	@echo "  Username: admin"