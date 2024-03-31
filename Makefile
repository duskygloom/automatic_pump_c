CC		= gcc
ARGS	= -Wall -g
BUILD	= build
BINARY	= automatic_pump
OBJECTS = $(BUILD)/request_handler.o $(BUILD)/fake_sensors.o $(BUILD)/server.o $(BUILD)/logger.o $(BUILD)/main.o


all: $(BINARY)

$(BINARY): $(OBJECTS)
	$(CC) $^ -o $@ $(ARGS)

$(BUILD)/%.o: %.c
	$(CC) $^ -o $@ $(ARGS) -c

init:
	mkdir -p $(BUILD)

run: $(BINARY)
	./$^

clean:
	rm -f $(BUILD)/*.o
