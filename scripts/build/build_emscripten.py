import os
import subprocess

emsdk_path = os.environ.get('EMSDK')
if emsdk_path is None or not os.path.exists(emsdk_path):
    raise Exception('EMSDK environment variable is not set')

cmake_toolchain_file = emsdk_path+'/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake'
if not os.path.exists(cmake_toolchain_file):
    raise Exception('Emscripten cmake toolchain file not found at '+cmake_toolchain_file)

# verify that we're in the root folder by checking for the CMakeLists.txt file and the resources folder
if not os.path.exists('CMakeLists.txt') or not os.path.exists('resources'):
    raise Exception('This script should be run from the root folder of the project')

os.makedirs('build/game-web', exist_ok=True)
os.chdir('build/game-web')

# for now just add all resources to the web build, we can make this more granular in the future
# create a symlink to the resources folder if it doesn't exist
if not os.path.exists('resources'):
    os.system('ln -s ../../resources resources')

result = subprocess.run([
    'cmake',
    '-DCMAKE_TOOLCHAIN_FILE='+cmake_toolchain_file,
    '-DCMAKE_BUILD_TYPE=Release',
    '-G', 'Unix Makefiles',
    '-DWEB_BUILD=ON',
    '-DFAKE_NETWORK=ON',
    '-DIMGUI_ENABLED=OFF',
    '-DBUILD_UNIT_TESTS=OFF',
    '-DBUILD_AUTO_TESTS=OFF',
    '../..'
])
if result.returncode != 0:
    raise Exception('CMake generation failed')

result = subprocess.run([
    'cmake',
    '--build',
    '.',
    '--config',
    'Release'
])
if result.returncode != 0:
    raise Exception('CMake build failed')

os.system('cp -r ../../resources/web-assets/* ../../bin/')
