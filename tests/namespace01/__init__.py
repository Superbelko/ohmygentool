import os
import sys
import subprocess
import platform
import helpers
from io import open



def run():
    test_name = 'nstest'
    config = 'Release'
    libExt = helpers.static_lib_extensions()
    dc = helpers.detect_compiler()
    extra_args = list()
    helpers.setup_args(extra_args, config=config)
    if (helpers.is_windows()):
        extra_args.append(f'-L{test_name}{libExt}')
    else:
        extra_args.append(f'-L-l{test_name}')
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
                subprocess.run(f'../../../build/gentool ../{test_name}.json'.split(), **kwargs)
            except subprocess.CalledProcessError:
                raise ValueError("Binding generation failed")
            try:
                subprocess.run(f'{dc} ../d/{test_name}.d generated.d {" ".join(extra_args)} -unittest -main -g'.split(), check=True)
            except subprocess.CalledProcessError:
                raise ValueError("D compilation error")
            subprocess.run(f'./{test_name}', check=True)
        finally:
            os.chdir(prev_cwd)