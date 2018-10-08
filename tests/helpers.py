
import platform
import subprocess

def is_64bit():
    return print(platform.machine().endswith('64'))


def is_windows():
    return platform.system() == "Windows"


def static_lib_extensions():
    if platform.system() == "Windows":
        return '.lib'
    return '.a'


def cmake_run(path='../cpp', build=True, config='Release', **kwargs):
    '''Runs configure and optional build'''

    subprocess.run(f'cmake {path}', check=True, **kwargs)
    if build:
        subprocess.run(f'cmake --build . --config {config}', check=True, **kwargs)
