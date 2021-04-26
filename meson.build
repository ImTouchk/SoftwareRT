project(
  'Raytracing', 
  'cpp', 
  version : '0.1', 
  default_options : ['warning_level=3', 'cpp_std=c++2a']
)

deps = [
  dependency('threads')
]

header_files = include_directories('include')
source_files = files([
  'source/main.cpp'
])

exe = executable(
  'Raytracer',
  source_files,
  include_directories : header_files,
  dependencies: deps
)
