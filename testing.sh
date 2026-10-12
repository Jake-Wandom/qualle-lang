#!/bin/bash
set -e
make clean && make DEBUG=1
./build/bin/quallcom -al examples/bell.ql
./build/bin/quallcom -al examples/coinflip.ql
./build/bin/quallcom -al examples/statement_examples/binop.ql
./build/bin/quallcom -al examples/statement_examples/call.ql
./build/bin/quallcom -al examples/statement_examples/define.ql
./build/bin/quallcom -al examples/statement_examples/func.ql
./build/bin/quallcom -al examples/statement_examples/if.ql
./build/bin/quallcom -al examples/statement_examples/loop.ql
echo "Finished testing features"
