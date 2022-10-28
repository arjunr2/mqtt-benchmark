.PHONY: comp dir clean

comp:
	gcc mqtt.c -lpthread -lpaho-mqtt3cs -lm -o benchmark

start-benchmark: dir comp
	mkdir -p results
	screen -S mqtt-benchmark-$(type) -dm bash -c "./update_bench.sh; ./run_benchmark_set.sh $(type)"

stop-benchmark:
	screen -S mqtt-benchmark-$(type) -p 0 -X stuff "^C"

clean:
	rm benchmark

clean-results:
	rm -r results
