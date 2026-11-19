import importlib.util
import pathlib
import struct
import tempfile
import unittest


spec = importlib.util.spec_from_file_location(
    "check_elf_alignment", pathlib.Path(__file__).resolve().parents[1] / "tools" / "check_elf_alignment.py")
alignment = importlib.util.module_from_spec(spec)
spec.loader.exec_module(alignment)


class ElfAlignmentTest(unittest.TestCase):
    def test_load_segments_must_match_target_page_size(self):
        ident = b"\x7fELF\x02\x01" + bytes(10)
        header = struct.pack("<16sHHIQQQIHHHHHH", ident, 3, 183, 1, 0, 64, 0, 0, 64, 56, 1, 0, 0, 0)
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "library.so"
            for offset, page_size, valid in ((0x20000, 65536, True),
                                             (0x21000, 65536, False),
                                             (0x21000, 4096, True),
                                             (0x20360, 4096, False)):
                segment = struct.pack("<IIQQQQQQ", 1, 4, offset, 0x10000, 0, 0, 0, 65536)
                path.write_bytes(header + segment)
                if valid:
                    self.assertTrue(alignment.check_file(path, page_size))
                else:
                    with self.assertRaisesRegex(ValueError, "not aligned"):
                        alignment.check_file(path, page_size)


if __name__ == "__main__":
    unittest.main()
