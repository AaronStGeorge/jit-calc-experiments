from decode import parse_module
import pprint

# So far this is a straight forward clone of https://github.com/dabeaz/wadze
if __name__ == "__main__":
    with open("add.wasm", "rb") as file:
        stream = iter(bytes(file.read()))
        module = parse_module(stream)

        pprint.pprint(module)
