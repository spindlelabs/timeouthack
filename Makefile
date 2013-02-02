TARGET_PORT_LOW := 80
TARGET_PORT_HIGH := 90
TARGET_IP_ADDRESS := 127.0.0.0
TARGET_IP_ADDRESS_MASK := 255.255.0.0
TIMEOUT_MILLISECONDS := 1800000
# set to a non-empty string to enable debugging
DEBUG := 1

timeouthack.so: timeouthack.c
	gcc "-DTARGET_PORT_LOW=$(TARGET_PORT_LOW)" "-DTARGET_PORT_HIGH=$(TARGET_PORT_HIGH)" "-DTARGET_IP_ADDRESS=\"$(TARGET_IP_ADDRESS)\"" "-DTARGET_IP_ADDRESS_MASK=\"$(TARGET_IP_ADDRESS_MASK)\"" "-DTIMEOUT_MILLISECONDS=$(TIMEOUT_MILLISECONDS)" $(if $(DEBUG),-DDEBUG) -Werror -Wall -O2 -fpic -shared -o "$@" "$^" -ldl

.PHONY: test
# tests require a debug build
test: timeouthack.so
	LD_PRELOAD=./timeouthack.so nc -z -v 127.0.0.1 80 2>&1 | grep -l "intercept" > /dev/null
	LD_PRELOAD=./timeouthack.so nc -z -v 127.0.25.1 90 2>&1 | grep -l "intercept" > /dev/null
	LD_PRELOAD=./timeouthack.so nc -z -v -u 127.0.32.2 85 2>&1 | grep -l "intercept" > /dev/null
	[ `LD_PRELOAD=./timeouthack.so nc -z -v 127.254.2.2 80 2>&1 | grep -L "intercept" | wc -l` -eq 1 ] > /dev/null
	[ `LD_PRELOAD=./timeouthack.so nc -z -v 127.0.0.2 91 2>&1 | grep -L "intercept" | wc -l` -eq 1 ] > /dev/null
	[ `LD_PRELOAD=./timeouthack.so nc -z -v 127.84.2.2 94 2>&1 | grep -L "intercept" | wc -l` -eq 1 ] > /dev/null

.PHONY: clean
clean:
	-rm timeouthack.so
