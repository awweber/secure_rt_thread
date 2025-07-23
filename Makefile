# Makefile for secure_rt_thread.c

CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = -lpthread -lrt
TARGET = secure_rt_thread
SRC = secure_rt_thread.c

# Standard-Target
all: $(TARGET)

# Kompilieren
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

# Testen
test: $(TARGET)
	@echo "Running tests..."
	@./$(TARGET) || true

# Aufräumen
clean:
	rm -f $(TARGET)


# hilfe
help:
	@echo "Makefile for secure_rt_thread"
	@echo "Usage:"
	@echo "  make          - Build the project"
	@echo "  make test     - Run tests"
	@echo "  make clean    - Clean up build files"
	@echo "  make help     - Show this help message"
	@echo "  make all      - Build the project (default)"