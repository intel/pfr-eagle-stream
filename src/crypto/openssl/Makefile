# Enable second expansion
.SECONDEXPANSION:

# Clear all built in suffixes
.SUFFIXES:

##############################################################################
# Set default goal before any targets. The default goal here is "test"
##############################################################################
DEFAULT_TARGET := test

.DEFAULT_GOAL := default
.PHONY: default
default: $(DEFAULT_TARGET)

##############################################################################
# Project Definitions
##############################################################################
	

##############################################################################
# Makefile starts here
##############################################################################

.PHONY: clean
clean:
	-rm -f openssl384 openssl384_key

% : %.cpp utils.h
	g++ -g $^ -lcrypto -o $@

openssl256_key: openssl256.cpp
	g++ $^ -DSPECIFY_KEY -lcrypto -o $@

openssl384_key: openssl384.cpp
	g++ $^ -DSPECIFY_KEY -lcrypto -o $@

.PHONY: compile
compile: openssl256 openssl256_key openssl384 openssl384_key generate_384_testdata generate_256_testdata

.PHONY: test
test: openssl256 openssl256_key openssl384 openssl384_key generate_384_testdata generate_256_testdata canned_256_testdata
	./openssl256
	./openssl256_key
	./openssl384
	./openssl384_key
	./generate_384_testdata
	./generate_256_testdata
	./canned_256_testdata


