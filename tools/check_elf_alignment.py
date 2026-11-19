import pathlib
import struct
import sys


def check_file(path, page_size):
    with path.open("rb") as binary:
        header = binary.read(64)
        if header[:4] != b"\x7fELF":
            return False
        if len(header) != 64 or header[4:6] != b"\x02\x01":
            raise ValueError("{}: expected a little-endian 64-bit ELF".format(path))
        fields = struct.unpack("<16sHHIQQQIHHHHHH", header)
        phoff, phentsize, phnum = fields[5], fields[9], fields[10]
        if phentsize < 56:
            raise ValueError("{}: invalid program header size".format(path))
        binary.seek(phoff)
        for index in range(phnum):
            binary.seek(phoff + index * phentsize)
            program_header = binary.read(56)
            if len(program_header) != 56:
                raise ValueError("{}: truncated program header".format(path))
            segment = struct.unpack("<IIQQQQQQ", program_header)
            if segment[0] == 1 and (segment[2] - segment[3]) % page_size:
                raise ValueError("{}: LOAD segment {} is not aligned for {}-byte pages".format(
                    path, index, page_size))
    return True


def check_directory(directory, page_size):
    count = 0
    for path in pathlib.Path(directory).rglob("*"):
        if path.is_file() and check_file(path, page_size):
            count += 1
    if not count:
        raise ValueError("{}: no ELF files found".format(directory))
    return count


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("Usage: check_elf_alignment.py DIRECTORY PAGE_SIZE")
    try:
        size = int(sys.argv[2])
        if size <= 0 or size & (size - 1):
            raise ValueError("PAGE_SIZE must be a positive power of two")
        count = check_directory(sys.argv[1], size)
    except (OSError, ValueError, struct.error) as error:
        sys.exit(str(error))
    print("Checked {} ELF files for {}-byte page alignment".format(count, size))
