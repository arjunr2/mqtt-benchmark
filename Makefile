.PHONY: all

all:
	gcc mqtt.c -lpthread -lpaho-mqtt3cs -o benchmark

clean:
	rm benchmark
