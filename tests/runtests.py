'''
Entry point for running system tests
'''
import os
import sys
import importlib
import argparse

# Make helper modules visible in tests
sys.path.append("./")


def find_tests(base_path=None):
    '''Finds all test packages in specified directory'''
    return find_selected(it for it in os.listdir(base_path) if it != '__pycache__')


def find_selected(names):
    for p in names:
        modpath = os.path.join(os.getcwd(), p)
        if not os.path.isdir(modpath):
            continue
        try:
            test = importlib.import_module(p)
            yield test
        except Exception as e:
            print(e)
            pass


def do_optional(f):
    '''Tries silently call f() ignoring non existing attributes'''
    try:
        f()
    except AttributeError:
        pass


def run_tests(tests):
    '''Run tests with optional setup and teardown phases'''

    tests_ok = True
    for n,test in enumerate(tests, 1):
        if not run_single(test, n):
            tests_ok = False
    if not tests_ok:
        print('Some tests not passed')
        exit(1)
    else:
        print('All tests are OK')


def run_single(test, num=None):
    '''Run single test'''
    print(f'TEST {num or ""}  [{test.__name__}]')
    do_optional(lambda: test.setup())
    try:
        test.run()
    except Exception as e:
        print(e)
        return False
    finally: 
        do_optional(lambda: test.teardown())
    return True


if __name__=='__main__':
    if len(sys.argv) == 1:
        run_tests(find_tests())
    else:
        print('Running selected tests:')
        print('\t', " ".join(sys.argv[1:]))
        run_tests(find_selected(sys.argv[1:]))
