def chunks(list, size):
    for i in range(0, len(list), size):
        yield list[i : i + size]


s = []
with open("hankaku.txt", "r", encoding="cp932") as r:
    for line in r:
        line = line.rstrip("\n")
        if line.startswith("OSASK") or line.startswith("char 0x") or line == "":
            continue
        line = line.replace(".", "0")
        line = line.replace("*", "1")
        s.append(int(line.replace(".", "0").replace("*", "1"), 2))

with open("../../haribote/hankaku.c", "w") as w:
    w.write("char hankaku[4096] = {\n")
    for ch in chunks(s, 16):
        s = ", ".join([hex(c) for c in ch])
        w.write(f"        {s},\n")
    w.write("};\n")
