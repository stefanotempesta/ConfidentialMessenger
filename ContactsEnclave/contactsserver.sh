#!/bin/sh
cd /home/azureuser/ContactsEnclave
. /opt/openenclave/share/openenclave/openenclaverc
make build
export LD_LIBRARY_PATH="/home/azureuser/ContactsEnclave/host/restbed/distribution/library"
make run