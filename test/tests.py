#! /usr/bin/env python

import unittest
import sys

if __name__ == "__main__":
    testsuite = unittest.TestLoader().discover('unit_tests')
    ret = unittest.TextTestRunner(verbosity=2).run(testsuite).wasSuccessful()
    sys.exit(1 if not ret else 0)
