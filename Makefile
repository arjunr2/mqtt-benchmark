.PHONY: comp dir clean

build:
	gcc mqtt.c -lpthread -lpaho-mqtt3cs -lm -o benchmark

debug:
	gcc -g mqtt.c -lpthread -lpaho-mqtt3cs -lm -o benchmark

start: build
	mkdir -p results
	mkdir -p logs
	screen -S $(bench) -dm bash -c "./update_bench.sh; \
		python3 run_benchmark.py --config hc-mqtt.cfg --batch batch.yml $(bench)"

stop:
	screen -S $(bench) -p 0 -X stuff "^C"

clean:
	rm benchmark

clean-stats:
	rm -r results
	rm -r logs
