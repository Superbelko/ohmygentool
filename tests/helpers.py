
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


def setup_args(args, config='Release'):
    if is_windows():
        args.append('msvcrt.lib')
        args.append('-m32mscoff')
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
