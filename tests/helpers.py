
import platform
import subprocess
import os

def is_64bit():
    return print(platform.machine().endswith('64'))


def is_windows():
    return platform.system() == "Windows"


def static_lib_extensions():
    if is_windows():
        return '.lib'
    return '.a'

def detect_compiler():
    executable = os.environ.get('DC')
    if not executable:
        try:
            proc = subprocess.run(f'ldc2 --version', check=True, stdout=subprocess.PIPE)
            if proc is not None:
                executable = 'ldc2'
            else:
                proc = subprocess.run(f'dmd --version', check=True, stdout=subprocess.PIPE)
                if proc is not None:
                    executable = 'dmd'
        except FileNotFoundError as e:
            raise Exception(('No D compiler detected, make sure you have compiler '
             'in PATH or specify it with DC environment variable'))
    else:
        try:
            proc = subprocess.run(f'{executable} --version', check=True, stdout=subprocess.PIPE)
        except FileNotFoundError as e:
            raise Exception('Unable to get DC, invalid path')
    return executable


def setup_args(args, config='Release'):
    if is_windows():
        args.append('msvcrt.lib')
        args.append('-m64')
        args.append('-L/NODEFAULTLIB:libcmt')
        args.append(f'-L/LIBPATH:{config}')
    else:
        args.append('-L-L.')
        args.append('-L-lstdc++')


def cmake_run(path='../cpp', build=True, config='Release', **kwargs):
    '''Runs configure and optional build'''

    subprocess.run(f'cmake {path}'.split(), check=True, **kwargs)
    if build:
        subprocess.run(f'cmake --build . --config {config}'.split(), check=True, **kwargs)
