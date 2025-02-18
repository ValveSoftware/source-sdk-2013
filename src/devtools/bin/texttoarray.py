import sys

def main(filename, objname):
    with open(filename, 'rb') as file:
        data = file.read()

    output = f"static unsigned char {objname}[] = {{\n    "

    for i in range(len(data)):
        output += f"0x{data[i]:02x},"
        if i % 20 == 19:
            output += "\n    "

    output += "0x00\n};\n"
    print(output)

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python texttoarray.py <filename> <name>")
        sys.exit(1)

    main(sys.argv[1], sys.argv[2])
