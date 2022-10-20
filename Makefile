.PHONY: all

all:
	gcc mqtt.c -lpthread -lpaho-mqtt3cs -lm -o benchmark

clean:
	rm benchmark
