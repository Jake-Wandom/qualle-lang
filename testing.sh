#!/bin/bash
set -e
make clean && make DEBUG=1
./build/bin/quallcom -l examples/bell.ql
./build/bin/quallcom -l examples/coinflip.ql
./build/bin/quallcom -al examples/statement_examples/binop.ql
./build/bin/quallcom -al examples/statement_examples/call.ql
./build/bin/quallcom -al examples/statement_examples/define.ql
./build/bin/quallcom -al examples/statement_examples/func.ql
./build/bin/quallcom -al examples/statement_examples/if.ql
./build/bin/quallcom -al examples/statement_examples/loop.ql
./build/bin/quallcom -al examples/quantum_algorithms/bernstein-vazirani.ql
echo "Finished testing features"
