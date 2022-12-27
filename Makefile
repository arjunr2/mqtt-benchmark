.PHONY: comp dir clean

GPTP_DIR=gptp

.ONESHELL: time-sync
prerun=:

build:
	gcc mqtt.c -lpthread -lpaho-mqtt3cs -lm -o benchmark

debug:
	gcc -g mqtt.c -lpthread -lpaho-mqtt3cs -lm -o benchmark

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
	@cd $(GPTP_DIR);
	./kill_gptp.sh
	@sleep 2
	./run_gptp.sh
	@echo 'Syncing nodes; may take up to a minute to stabilize'


clean:
	rm benchmark

clean-stats:
	rm -r results
	rm -r logs

clean-debug:
	rm -r debug
