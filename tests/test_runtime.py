import _decimal
import ctypes
import lzma
import sqlite3
import ssl
import unittest
import uuid

import numpy as np


class RuntimeTest(unittest.TestCase):
    def test_standard_library_extensions(self):
        self.assertTrue(ssl.OPENSSL_VERSION)
        self.assertTrue(ctypes.sizeof(ctypes.c_void_p))
        self.assertEqual(str(_decimal.Decimal("0.1") + _decimal.Decimal("0.2")), "0.3")
        self.assertEqual(lzma.decompress(lzma.compress(b"hello")), b"hello")
        with sqlite3.connect(":memory:") as db:
            self.assertEqual(db.execute("select 42").fetchone()[0], 42)
        self.assertEqual(uuid.UUID(str(uuid.uuid4())).version, 4)

    def test_numpy_native_operations(self):
        values = np.array([1.0, 2.0, 3.0])
        self.assertEqual(np.dot(values, values), 14.0)
        np.testing.assert_allclose(np.linalg.solve([[2.0, 0.0], [0.0, 4.0]], [4.0, 8.0]), [2.0, 2.0])


if __name__ == "__main__":
    unittest.main()
