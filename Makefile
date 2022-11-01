.PHONY: comp dir clean
#cmds := $(shell python3-config --includes)

comp:
	gcc mqtt.c $(cmds) -lpthread -lpaho-mqtt3cs -lm -o benchmark

start-benchmark: dir comp
	mkdir -p results
	mkdir -p logs
	screen -S mqtt-benchmark-$(type) -dm bash -c "./update_bench.sh; ./run_benchmark_set.sh $(type)"

stop-benchmark:
	screen -S mqtt-benchmark-$(type) -p 0 -X stuff "^C"

clean:
	rm benchmark

clean-stats:
	rm -r results
	rm -r logs
