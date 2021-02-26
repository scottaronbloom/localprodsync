# The MIT License (MIT)
#
# Copyright (c) 2020-2021 Scott Aron Bloom
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.1)
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)
find_package(Threads REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(Qt5SrcMoc  REQUIRED)
AddQtIncludes()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

SET(CMAKE_AUTOMOC OFF)
SET(CMAKE_AUTORCC OFF)
SET(CMAKE_AUTOUIC OFF)

include( include.cmake )
UNSET( qtproject_UIS_H )
UNSET( qtproject_MOC_SRCS )
UNSET( qtproject_CPPMOC_H )
UNSET( qtproject_QRC_SRCS )
QT5_WRAP_UI(qtproject_UIS_H ${qtproject_UIS})
if( DEFINED SAB_MOC_OPTIONS )
	QT5_WRAP_CPP(qtproject_MOC_SRCS ${qtproject_H} OPTIONS ${SAB_MOC_OPTIONS})
	SAB_WRAP_SRCMOC(qtproject_CPPMOC_H ${qtproject_CPPMOC_SRCS} OPTIONS ${SAB_MOC_OPTIONS})
else()
	QT5_WRAP_CPP(qtproject_MOC_SRCS ${qtproject_H})
	SAB_WRAP_SRCMOC(qtproject_CPPMOC_H ${qtproject_CPPMOC_SRCS})
endif()
QT5_ADD_RESOURCES( qtproject_QRC_SRCS ${qtproject_QRC} )

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

source_group("Generated Files" FILES ${qtproject_UIS_H} ${qtproject_MOC_SRCS} ${qtproject_QRC_SRCS} ${qtproject_CPPMOC_H})
source_group("Resource Files" FILES ${qtproject_QRC} ${qtproject_QRC_SOURCES} )
source_group("Designer Files" FILES ${qtproject_UIS} )
source_group("Header Files" FILES ${qtproject_H} ${project_H} )
source_group("Source Files" FILES ${qtproject_CPPMOC_SRCS} )

SET( _CMAKE_FILES "CMakeLists.txt;include.cmake" )
source_group("CMake Files" FILES ${_CMAKE_FILES} )
FILE(GLOB _CMAKE_MODULE_FILES "${CMAKE_SOURCE_DIR}/Modules/*")
source_group("CMake Files\\Modules" FILES ${_CMAKE_MODULE_FILES} )

include( ${CMAKE_SOURCE_DIR}/CompilerSettings.cmake )


