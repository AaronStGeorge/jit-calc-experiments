import argparse
import pprint

import wasm


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Dump decoded wasm file")
    parser.add_argument("file", type=str, help="Wasm file to load")

    args = parser.parse_args()

    with open(args.file, "rb") as file:
        stream = iter(bytes(file.read()))
        module = wasm.decode(stream)

        pprint.pprint(module)
