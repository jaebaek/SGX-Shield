#!/bin/bash

cd linux-sgx

echo "=== patching enclave data pages ==="
patch -p1 < ../data_page.patch
