from ctypes import *
import os
import sys
from contextlib import contextmanager
from pyqir import BasicQisBuilder, SimpleModule

cdll.LoadLibrary("include/lib-quallcom.so")
libc = CDLL("include/lib-quallcom.so")


@contextmanager
def capture_c_stdout(filename="c_output.txt"):
    # Flush Python streams first
    sys.stdout.flush()

    # Save original stdout file descriptor
    stdout_fd = sys.stdout.fileno()
    saved_stdout = os.dup(stdout_fd)

    # Open a temporary file and redirect stdout to it
    tfile = open(filename, "w+")
    os.dup2(tfile.fileno(), stdout_fd)

    try:
        yield
    finally:
        # Flush C streams, restore original stdout, and clean up
        libc.fflush(None)
        os.dup2(saved_stdout, stdout_fd)
        os.close(saved_stdout)
        tfile.close()

python_args = ["-h", "examples/test.ql"]

c_argv_data = [create_string_buffer(arg.encode("utf-8")) for arg in python_args]
c_argv = (c_char_p * len(c_argv_data))(*[cast(s, c_char_p) for s in c_argv_data])
c_argc = c_int(len(python_args))

output_file = "captured_printf.txt"

print("--- Starting C Execution ---")
with capture_c_stdout(output_file):
    libc.main(c_argc, c_argv)
    #libc.print_man_page(c_void_p())
print("--- C Execution Finished ---")

with open(output_file, "r") as f:
    c_output = f.read()

print("\nCaptured Output from C:")
print(c_output)

#print(libc.print_token_list(libc.get_token(c_wchar_p("qbit abc = 0;"))))



# Create the module with two qubits and two results.
bell = SimpleModule("bell", num_qubits=2, num_results=2)
qis = BasicQisBuilder(bell.builder)

# Add instructions to the module to create a Bell pair and measure both qubits.
qis.h(bell.qubits[0])
#qis.cx(bell.qubits[0], bell.qubits[1])
qis.mz(bell.qubits[0], bell.results[0])
#qis.mz(bell.qubits[1], bell.results[1])

if __name__ == "__main__":
    print(bell.ir())