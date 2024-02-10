from decode import parse_module
import pprint
import sys

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <file.wasm> ", file=sys.stderr)
        exit(1)

    with open(sys.argv[1], "rb") as file:
        stream = iter(bytes(file.read()))
        module = parse_module(stream)

        pprint.pprint(module)
