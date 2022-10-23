.PHONY: all

all:
	gcc mqtt.c -lpthread -lpaho-mqtt3cs -lm -o benchmark

start-benchmark:
	screen -S mqtt-benchmark -dm bash -c "./run_benchmark_set.sh"

stop-benchmark:
	screen -S mqtt-benchmark -p 0 -X stuff "^C"

clean:
	rm benchmark
