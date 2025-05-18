# Компилятор
CC=gcc

# Флаги компиляции:
# -Wall - включение всех предупреждений
# -Wextra - включение дополнительных предупреждений
# -I - добавление путей к заголовочным файлам
CFLAGS=-Wall -Wextra -I./mongoose -I./input -I./constants

# Объектные файлы, участвующие в сборке
OBJ=input/input.o mongoose/mongoose.o main.o

# Имя результирующего исполняемого файла
TARGET=server

# Цель по умолчанию — сборка исполняемого файла
all: $(TARGET)

# Компиляция main.c в main.o
main.o: main.c
	$(CC) $(CFLAGS) -c main.c

# Компиляция input/input.c в input/input.o
input/input.o: input/input.c
	$(CC) $(CFLAGS) -c input/input.c -o input/input.o

# Компиляция mongoose/mongoose.c в mongoose/mongoose.o
mongoose/mongoose.o: mongoose/mongoose.c
	$(CC) $(CFLAGS) -c mongoose/mongoose.c -o mongoose/mongoose.o

# Сборка исполняемого файла из объектных файлов
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

# Очистка: удаление объектных файлов и исполняемого файла
clean:
	rm -f *.o input/*.o mongoose/*.o $(TARGET)

# Запуск сервера через скрипт
run:
	./run.sh

# Остановка сервера, если существует файл server.pid
stop:
	@if [ -f server.pid ]; then \
		kill $$(cat server.pid) && rm -f server.pid; \
		echo "Сервер остановлен"; \
	else \
		echo "Файл server.pid не найден"; \
	fi
