import os
import sys
import subprocess
import platform
import helpers
from io import open


def run():
    config = 'Release'
    libExt = helpers.static_lib_extensions()
    extra_args = list()
    if (helpers.is_windows()):
        extra_args.append('msvcrt.lib')
        extra_args.append('-m32mscoff')
        extra_args.append('-L/NODEFAULTLIB:libcmt')
    prev_cwd = os.getcwd()
    rootPath = os.path.dirname(sys.argv[0])
    path = (rootPath + '/' + __package__)
    os.chdir(path)
    try:
        os.mkdir('temp')
    except FileExistsError:
        pass
    os.chdir('temp')
    with open('out.txt', 'w') as out, open('err.txt', 'w') as err:
        kwargs = dict(stdout=out, stderr=err)
        try:
            try:
                helpers.cmake_run('../cpp', config=config, **kwargs)
            except subprocess.CalledProcessError:
                raise ValueError("C++ compilation error")
            try:
                subprocess.run('../../../build/gentool ../optest.json', **kwargs)
                pass
            except subprocess.CalledProcessError:
                raise ValueError("Binding generation failed")
            try:
                subprocess.run(f'dmd ../d/optest.d generated.d {config}/optest{libExt} {" ".join(extra_args)} -unittest -main -g', check=True)
            except subprocess.CalledProcessError:
                raise ValueError("D compilation error")
            subprocess.run('./optest', check=True)
        finally:
            os.chdir(prev_cwd)