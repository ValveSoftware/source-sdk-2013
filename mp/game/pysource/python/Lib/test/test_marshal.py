#!/usr/bin/env python3

from test import support
import array
import marshal
import sys
import unittest
import os
import types

class HelperMixin:
    def helper(self, sample, *extra):
        new = marshal.loads(marshal.dumps(sample, *extra))
        self.assertEqual(sample, new)
        try:
            with open(support.TESTFN, "wb") as f:
                marshal.dump(sample, f, *extra)
            with open(support.TESTFN, "rb") as f:
                new = marshal.load(f)
            self.assertEqual(sample, new)
        finally:
            support.unlink(support.TESTFN)

class IntTestCase(unittest.TestCase, HelperMixin):
    def test_ints(self):
        # Test the full range of Python ints.
        n = sys.maxsize
        while n:
            for expected in (-n, n):
                self.helper(expected)
            n = n >> 1

    def test_int64(self):
        # Simulate int marshaling on a 64-bit box.  This is most interesting if
        # we're running the test on a 32-bit box, of course.

        def to_little_endian_string(value, nbytes):
            b = bytearray()
            for i in range(nbytes):
                b.append(value & 0xff)
                value >>= 8
            return b

        maxint64 = (1 << 63) - 1
        minint64 = -maxint64-1

        for base in maxint64, minint64, -maxint64, -(minint64 >> 1):
            while base:
                s = b'I' + to_little_endian_string(base, 8)
                got = marshal.loads(s)
                self.assertEqual(base, got)
                if base == -1:  # a fixed-point for shifting right 1
                    base = 0
                else:
                    base >>= 1

    def test_bool(self):
        for b in (True, False):
            self.helper(b)

class FloatTestCase(unittest.TestCase, HelperMixin):
    def test_floats(self):
        # Test a few floats
        small = 1e-25
        n = sys.maxsize * 3.7e250
        while n > small:
            for expected in (-n, n):
                self.helper(float(expected))
            n /= 123.4567

        f = 0.0
        s = marshal.dumps(f, 2)
        got = marshal.loads(s)
        self.assertEqual(f, got)
        # and with version <= 1 (floats marshalled differently then)
        s = marshal.dumps(f, 1)
        got = marshal.loads(s)
        self.assertEqual(f, got)

        n = sys.maxsize * 3.7e-250
        while n < small:
            for expected in (-n, n):
                f = float(expected)
                self.helper(f)
                self.helper(f, 1)
            n *= 123.4567

class StringTestCase(unittest.TestCase, HelperMixin):
    def test_unicode(self):
        for s in ["", "Andr\xe8 Previn", "abc", " "*10000]:
            self.helper(marshal.loads(marshal.dumps(s)))

    def test_string(self):
        for s in ["", "Andr\xe8 Previn", "abc", " "*10000]:
            self.helper(s)

    def test_bytes(self):
        for s in [b"", b"Andr\xe8 Previn", b"abc", b" "*10000]:
            self.helper(s)

class ExceptionTestCase(unittest.TestCase):
    def test_exceptions(self):
        new = marshal.loads(marshal.dumps(StopIteration))
        self.assertEqual(StopIteration, new)

class CodeTestCase(unittest.TestCase):
    def test_code(self):
        co = ExceptionTestCase.test_exceptions.__code__
        new = marshal.loads(marshal.dumps(co))
        self.assertEqual(co, new)

    def test_many_codeobjects(self):
        # Issue2957: bad recursion count on code objects
        count = 5000    # more than MAX_MARSHAL_STACK_DEPTH
        codes = (ExceptionTestCase.test_exceptions.__code__,) * count
        marshal.loads(marshal.dumps(codes))

    def test_different_filenames(self):
        co1 = compile("x", "f1", "exec")
        co2 = compile("y", "f2", "exec")
        co1, co2 = marshal.loads(marshal.dumps((co1, co2)))
        self.assertEqual(co1.co_filename, "f1")
        self.assertEqual(co2.co_filename, "f2")

    @support.cpython_only
    def test_same_filename_used(self):
        s = """def f(): pass\ndef g(): pass"""
        co = compile(s, "myfile", "exec")
        co = marshal.loads(marshal.dumps(co))
        for obj in co.co_consts:
            if isinstance(obj, types.CodeType):
                self.assertIs(co.co_filename, obj.co_filename)

class ContainerTestCase(unittest.TestCase, HelperMixin):
    d = {'astring': 'foo@bar.baz.spam',
         'afloat': 7283.43,
         'anint': 2**20,
         'ashortlong': 2,
         'alist': ['.zyx.41'],
         'atuple': ('.zyx.41',)*10,
         'aboolean': False,
         'aunicode': "Andr\xe8 Previn"
         }

    def test_dict(self):
        self.helper(self.d)

    def test_list(self):
        self.helper(list(self.d.items()))

    def test_tuple(self):
        self.helper(tuple(self.d.keys()))

    def test_sets(self):
        for constructor in (set, frozenset):
            self.helper(constructor(self.d.keys()))


class BufferTestCase(unittest.TestCase, HelperMixin):

    def test_bytearray(self):
        b = bytearray(b"abc")
        self.helper(b)
        new = marshal.loads(marshal.dumps(b))
        self.assertEqual(type(new), bytes)

    def test_memoryview(self):
        b = memoryview(b"abc")
        self.helper(b)
        new = marshal.loads(marshal.dumps(b))
        self.assertEqual(type(new), bytes)

    def test_array(self):
        a = array.array('B', b"abc")
        new = marshal.loads(marshal.dumps(a))
        self.assertEqual(new, b"abc")


class BugsTestCase(unittest.TestCase):
    def test_bug_5888452(self):
        # Simple-minded check for SF 588452: Debug build crashes
        marshal.dumps([128] * 1000)

    def test_patch_873224(self):
        self.assertRaises(Exception, marshal.loads, '0')
        self.assertRaises(Exception, marshal.loads, 'f')
        self.assertRaises(Exception, marshal.loads, marshal.dumps(2**65)[:-1])

    def test_version_argument(self):
        # Python 2.4.0 crashes for any call to marshal.dumps(x, y)
        self.assertEqual(marshal.loads(marshal.dumps(5, 0)), 5)
        self.assertEqual(marshal.loads(marshal.dumps(5, 1)), 5)

    def test_fuzz(self):
        # simple test that it's at least not *totally* trivial to
        # crash from bad marshal data
        for c in [chr(i) for i in range(256)]:
            try:
                marshal.loads(c)
            except Exception:
                pass

    def test_loads_recursion(self):
        s = b'c' + (b'X' * 4*4) + b'{' * 2**20
        self.assertRaises(ValueError, marshal.loads, s)

    def test_recursion_limit(self):
        # Create a deeply nested structure.
        head = last = []
        # The max stack depth should match the value in Python/marshal.c.
        if os.name == 'nt' and hasattr(sys, 'gettotalrefcount'):
            MAX_MARSHAL_STACK_DEPTH = 1500
        else:
            MAX_MARSHAL_STACK_DEPTH = 2000
        for i in range(MAX_MARSHAL_STACK_DEPTH - 2):
            last.append([0])
            last = last[-1]

        # Verify we don't blow out the stack with dumps/load.
        data = marshal.dumps(head)
        new_head = marshal.loads(data)
        # Don't use == to compare objects, it can exceed the recursion limit.
        self.assertEqual(len(new_head), len(head))
        self.assertEqual(len(new_head[0]), len(head[0]))
        self.assertEqual(len(new_head[-1]), len(head[-1]))

        last.append([0])
        self.assertRaises(ValueError, marshal.dumps, head)

    def test_exact_type_match(self):
        # Former bug:
        #   >>> class Int(int): pass
        #   >>> type(loads(dumps(Int())))
        #   <type 'int'>
        for typ in (int, float, complex, tuple, list, dict, set, frozenset):
            # Note: str subclasses are not tested because they get handled
            # by marshal's routines for objects supporting the buffer API.
            subtyp = type('subtyp', (typ,), {})
            self.assertRaises(ValueError, marshal.dumps, subtyp())

    # Issue #1792 introduced a change in how marshal increases the size of its
    # internal buffer; this test ensures that the new code is exercised.
    def test_large_marshal(self):
        size = int(1e6)
        testString = 'abc' * size
        marshal.dumps(testString)

    def test_invalid_longs(self):
        # Issue #7019: marshal.loads shouldn't produce unnormalized PyLongs
        invalid_string = b'l\x02\x00\x00\x00\x00\x00\x00\x00'
        self.assertRaises(ValueError, marshal.loads, invalid_string)

    def test_multiple_dumps_and_loads(self):
        # Issue 12291: marshal.load() should be callable multiple times
        # with interleaved data written by non-marshal code
        # Adapted from a patch by Engelbert Gruber.
        data = (1, 'abc', b'def', 1.0, (2, 'a', ['b', b'c']))
        for interleaved in (b'', b'0123'):
            ilen = len(interleaved)
            positions = []
            try:
                with open(support.TESTFN, 'wb') as f:
                    for d in data:
                        marshal.dump(d, f)
                        if ilen:
                            f.write(interleaved)
                        positions.append(f.tell())
                with open(support.TESTFN, 'rb') as f:
                    for i, d in enumerate(data):
                        self.assertEqual(d, marshal.load(f))
                        if ilen:
                            f.read(ilen)
                        self.assertEqual(positions[i], f.tell())
            finally:
                support.unlink(support.TESTFN)

    def test_loads_reject_unicode_strings(self):
        # Issue #14177: marshal.loads() should not accept unicode strings
        unicode_string = 'T'
        self.assertRaises(TypeError, marshal.loads, unicode_string)

LARGE_SIZE = 2**31
pointer_size = 8 if sys.maxsize > 0xFFFFFFFF else 4

class NullWriter:
    def write(self, s):
        pass

@unittest.skipIf(LARGE_SIZE > sys.maxsize, "test cannot run on 32-bit systems")
class LargeValuesTestCase(unittest.TestCase):
    def check_unmarshallable(self, data):
        self.assertRaises(ValueError, marshal.dump, data, NullWriter())

    @support.bigmemtest(size=LARGE_SIZE, memuse=1, dry_run=False)
    def test_bytes(self, size):
        self.check_unmarshallable(b'x' * size)

    @support.bigmemtest(size=LARGE_SIZE, memuse=1, dry_run=False)
    def test_str(self, size):
        self.check_unmarshallable('x' * size)

    @support.bigmemtest(size=LARGE_SIZE, memuse=pointer_size, dry_run=False)
    def test_tuple(self, size):
        self.check_unmarshallable((None,) * size)

    @support.bigmemtest(size=LARGE_SIZE, memuse=pointer_size, dry_run=False)
    def test_list(self, size):
        self.check_unmarshallable([None] * size)

    @support.bigmemtest(size=LARGE_SIZE,
            memuse=pointer_size*12 + sys.getsizeof(LARGE_SIZE-1),
            dry_run=False)
    def test_set(self, size):
        self.check_unmarshallable(set(range(size)))

    @support.bigmemtest(size=LARGE_SIZE,
            memuse=pointer_size*12 + sys.getsizeof(LARGE_SIZE-1),
            dry_run=False)
    def test_frozenset(self, size):
        self.check_unmarshallable(frozenset(range(size)))

    @support.bigmemtest(size=LARGE_SIZE, memuse=1, dry_run=False)
    def test_bytearray(self, size):
        self.check_unmarshallable(bytearray(size))


def test_main():
    support.run_unittest(IntTestCase,
                         FloatTestCase,
                         StringTestCase,
                         CodeTestCase,
                         ContainerTestCase,
                         ExceptionTestCase,
                         BufferTestCase,
                         BugsTestCase,
                         LargeValuesTestCase,
                        )

if __name__ == "__main__":
    test_main()
