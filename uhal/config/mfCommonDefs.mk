# Sanitize BUILD_HOME
BUILD_HOME := $(shell cd $(BUILD_HOME); pwd)

$(info Using BUILD_HOME=${BUILD_HOME})

# Compilers
CXX = g++
LD = ${CXX}
AR = ar

# Compiler flags
CXXFLAGS = -g -Wall -pedantic -O3 -MMD -MP -fPIC
ARFLAGS = rc

ifdef BUILD_STATIC
  LDFLAGS = -static -Wl,-Bstatic -shared-libgcc -Wall -g -O3 -fPIC
else
  LDFLAGS = -Wall -g -O3 -fPIC
endif

# Tools
MakeDir = mkdir -p


PYTHON ?= python
PYTHON_VERSION ?= $(shell ${PYTHON} -c "import distutils.sysconfig; print(distutils.sysconfig.get_python_version())")
PYTHON_INCLUDE_PREFIX ?= $(shell ${PYTHON} -c "import distutils.sysconfig; print(distutils.sysconfig.get_python_inc())")
PYTHON_LIB_PREFIX ?= $(shell ${PYTHON} -c "from distutils.sysconfig import get_python_lib; import os.path; print(os.path.split(get_python_lib(standard_lib=True))[0])")

# Construct C++ compiler suffix for RPM release field (tested with clang & gcc)
CXX_VERSION_TAG = $(word 1, $(shell ${CXX} --version))
CXX_VERSION_TAG := $(subst g++,gcc,${CXX_VERSION_TAG})
CXX_VERSION_TAG := ${CXX_VERSION_TAG}$(shell ${CXX} -dumpfullversion -dumpversion)
CXX_VERSION_TAG := $(subst .,_,${CXX_VERSION_TAG})

# Minimum C++ standard: C++11 (default standard is C++14 from GCC 6.1)
ifneq (,$(findstring gcc4,${CXX_VERSION_TAG})$(findstring gcc5,${CXX_VERSION_TAG})$(findstring gcc6_0,${CXX_VERSION_TAG}))
  CXXFLAGS += -std=c++11
endif


ifneq (,$(wildcard ${EXTERN_BOOST_LIB_PREFIX}/libboost_thread-mt.so ${EXTERN_BOOST_LIB_PREFIX}/libboost_thread-mt.a ${EXTERN_BOOST_LIB_PREFIX}/libboost_thread-mt.dylib))
  BOOST_THREAD_LIB = boost_thread-mt
else ifneq (,$(wildcard ${EXTERN_BOOST_LIB_PREFIX}/libboost_thread.so ${EXTERN_BOOST_LIB_PREFIX}/libboost_thread.a ${EXTERN_BOOST_LIB_PREFIX}/libboost_thread.dylib))
  BOOST_THREAD_LIB = boost_thread
else ifeq (,$(shell LANG=C ${CXX} -lboost_thread-mt 2>&1 | grep -E 'ld: (cannot find|library not found)'))
  BOOST_THREAD_LIB = boost_thread-mt
else
  BOOST_THREAD_LIB = boost_thread
endif
