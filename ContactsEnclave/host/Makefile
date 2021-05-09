# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

include ../../config.mk

CFLAGS=$(shell pkg-config oehost-$(C_COMPILER) --cflags)
CXXFLAGS=$(shell pkg-config oehost-$(CXX_COMPILER) --cflags)
LDFLAGS=$(shell pkg-config oehost-$(CXX_COMPILER) --libs)
INCDIR=$(shell pkg-config oehost-$(C_COMPILER) --variable=includedir)

all: build

build:
	@ echo "Compilers used: $(CC), $(CXX)"
	oeedger8r ../contactsenclave.edl --untrusted \
		--search-path $(INCDIR) \
		--search-path $(INCDIR)/openenclave/edl/sgx
	$(CXX) -g -c $(CXXFLAGS) -I"../host/restbed/distribution/include" host.cpp 
	$(CC) -g -c $(CFLAGS) contactsenclave_u.c
	$(CXX) -o file-encryptorhost host.o contactsenclave_u.o $(LDFLAGS) -L"../host/restbed/distribution/library" -lrestbed

clean:
	rm -f file-encryptorhost contactsenclave_u.* contactsenclave_args.h *.o ../out.decrypted ../out.encrypted

