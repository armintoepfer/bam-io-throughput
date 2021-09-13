project(
  'biot',
  'cpp',
  version : '0.0.1',
  default_options : [
    'buildtype=release',
    'warning_level=3',
    'cpp_std=c++17',
    'b_ndebug=if-release'],
  license : 'BSD-3',
  meson_version : '>= 0.52.0')

# CXXFLAGS
biot_flags = []
cpp = meson.get_compiler('cpp')
foreach cflag: [
  '-Werror=strict-aliasing',
  '-Wno-unused-parameter',
  '-Wno-unused-local-typedef']
    if cpp.has_argument(cflag)
      biot_flags += cflag
    endif
endforeach

# dependencies #
## threads
biot_thread_dep = dependency('threads', required : true)
## boost
biot_boost_dep = dependency('boost', required : true)
## zlib
biot_zlib_dep = dependency('zlib', required : true, fallback : ['zlib', 'zlib_dep'])
## pbbam
biot_pbbam_dep = dependency('pbbam', fallback : ['pbbam', 'pbbam_dep'])
## pbcopper
biot_pbcopper_dep = dependency('pbcopper', fallback : ['pbcopper', 'pbcopper_dep'])
# htslib
biot_htslib_dep = dependency('htslib', required : true, version : '>=1.4', fallback : ['htslib', 'htslib_dep'])

biot_lib_deps = [
  biot_thread_dep,
  biot_boost_dep,
  biot_zlib_dep,
  biot_pbbam_dep,
  biot_pbcopper_dep,
  biot_htslib_dep,
]

biot_src_include_directories = include_directories('src')

# sources + executable
biot_main = executable(
  'biot',
  files([
    'src/main.cpp',
  ]),
  install : true,
  dependencies : biot_lib_deps,
  include_directories : biot_src_include_directories,
  cpp_args : biot_flags)
