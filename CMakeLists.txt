# build_lib_example(
#         NAME cca-perf
#         SOURCE_FILES cca-perf.cc
#         LIBRARIES_TO_LINK ${libai} ${libnr} ${libcore}
# )
# pybind11_add_module(cca_perf_py cca-perf-ai-py.cc)
# set_target_properties(cca_perf_py PROPERTIES
#         LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# # Build Python binding library along with C++ library
# add_dependencies(cca-perf cca_perf_py)

check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

find_package(Boost REQUIRED COMPONENTS program_options serialization)
include_directories(${Boost_INCLUDE_DIRS})
find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)
include_directories(${pybind11_INCLUDE_DIRS})

find_package(PythonLibs REQUIRED)
find_package(Protobuf CONFIG)
if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()
include_directories(${Protobuf_INCLUDE_DIRS})

string(REPLACE "/scratch/cca-perf" "/build/scratch" scratch_dir ${CMAKE_CURRENT_SOURCE_DIR})

build_exec(
      EXECNAME ccaperf-exec
      EXECNAME_PREFIX scratch_
      SOURCE_FILES 
            cca-perf.cc
            cca-perf-headerapp.cc
            cca-perf-clientapp.cc
            cca-perf-serverapp.cc
            physical-scenarios.cc
      LIBRARIES_TO_LINK 
            ${libai}
            ${libnr}
            "${ns3-libs}"
      EXECUTABLE_DIRECTORY_PATH ${scratch_dir}
  )

pybind11_add_module(cca_perf_py cca-perf-ai-py.cc)
set_target_properties(cca_perf_py PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cca_perf_py PRIVATE Boost::serialization pybind11::module "${ns3-libs}" ${PYTHON_LIBRARIES} ${libai})
