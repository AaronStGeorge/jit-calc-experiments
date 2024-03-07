import argparse
import pprint

from decode import parse_module


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Dump decoded wasm file")
    parser.add_argument("file", type=str, help="Wasm file to load")

    args = parser.parse_args()

    with open(args.file, "rb") as file:
        stream = iter(bytes(file.read()))
        module = parse_module(stream)

        pprint.pprint(module)
