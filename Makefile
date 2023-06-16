.PHONY: comp dir clean

GPTP_DIR_MAIN=gptp
SYSROOT_DIR=/home/arjun/Documents/research/webassembly/wali/wali-musl/sysroot

.ONESHELL: time-sync
prerun=:

build:
	clang -v --target=wasm32-wasi-threads -O0 --sysroot=$(SYSROOT_DIR) -L$(SYSROOT_DIR)/lib \
		-L./lib -L/home/arjun/Documents/mqtt-benchmark/lib -matomics -mbulk-memory -mmutable-globals -msign-ext \
		-Wl,--shared-memory -Wl,--export-memory -Wl,--max-memory=67108864 \
		-lm -lpaho-mqtt3c -o benchmark mqtt.c

native:
	gcc mqtt.c -lpthread -lpaho-mqtt3cs -lm -o benchmark-native
	
debug:
	gcc -g mqtt.c -lpthread -lpaho-mqtt3cs -lm -o benchmark-native

start: build
	mkdir -p results
	mkdir -p logs
	mkdir -p debug
	screen -S $(bench) -L -Logfile "debug/$(bench).debug" -dm bash -c "$(prerun); \
		python3 run_benchmark.py --config hc-mqtt.cfg --batch batch.yml $(bench) $(args)"
	screen -S $(bench) -X colon "logfile flush 0^M"

stop:
	screen -S $(bench) -p 0 -X stuff "^C"

time-sync:
	@cd $(GPTP_DIR_MAIN);
	./kill_gptp.sh
	@sleep 2
	./run_gptp.sh
	@echo 'Syncing nodes; may take up to a minute to stabilize'


clean:
	rm benchmark benchmark-native

clean-stats:
	rm -r results
	rm -r logs

clean-debug:
	rm -r debug
