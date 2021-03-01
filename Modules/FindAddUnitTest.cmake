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
FIND_PACKAGE( Qt5 COMPONENTS Core Widgets Test REQUIRED)
FUNCTION(CreateUserProj)
    IF(WIN32)
        SET(USER_VCXPROJ ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user )
        IF ( NOT EXISTS ${USER_VCXPROJ} )
            SET(SAB_DEBUG_TEST "--gtest_catch_exceptions=0  --gtest_also_run_disabled_tests --gtest_filter=*" CACHE STRING "Added command line arguments for the Unit Tests" )
    
            SET(SAB_VCX_ARCH x64)
            IF(CMAKE_SIZEOF_VOID_P EQUAL 4) 
                SET(SAB_VCX_ARCH Win32)
            ELSE()
                SET(SAB_VCX_ARCH x64)
            ENDIF()

            SET(ProjName ${PROJECT_NAME})
            if ( ARGV0 )
                SET(ProjName ${ARGV0})
            endif()

            set( templateFound false )
            foreach( currDir ${CMAKE_MODULE_PATH} )
                #MESSAGE( STATUS "${currDir}" )
                if ( EXISTS "${currDir}/vcxproj.unittest.user.in" )
                    SET( FILE_TEMPLATE ${currDir}/vcxproj.unittest.user.in )
                    Message( STATUS "=== Generating User VCXProj User File ===" )
                    MESSAGE( STATUS "   Project:${PROJECT_NAME}" )
                    MESSAGE( STATUS "   File:${USER_VCXPROJ}" )
                    MESSAGE( STATUS "   SOURCE=${FILE_TEMPLATE}" )
                    MESSAGE( STATUS "   USER=$ENV{USERDOMAIN}.$ENV{USERNAME}" )
                    MESSAGE( STATUS "   DebugDir:${CMAKE_INSTALL_PREFIX}" )
                    configure_file(
                        "${FILE_TEMPLATE}"
                        "${USER_VCXPROJ}"
                    )
                    Message( STATUS "=== Finished Generating User VCXProj User File ===" )
                    set( templateFound true )
                ENDIF()
            endforeach()
            if ( NOT templateFound )
                message( FATAL_ERROR "Could not find vcxproj.unittest.user.in" )
            ENDIF()
        ENDIF()
    ENDIF()
ENDFUNCTION(CreateUserProj)


FUNCTION(SAB_UNIT_TEST_RESOURCE name)
    STRING(REPLACE "${CMAKE_SOURCE_DIR}" "" LCL_DIR ${CMAKE_CURRENT_LIST_DIR})
    STRING(REPLACE "/UnitTests" "" LCL_DIR ${LCL_DIR})
    STRING(REGEX REPLACE "/(.*)/(.*)" "SABTests/UnitTests/\\1/\\2" FOLDER_NAME ${FOLDER_NAME})
    SET(FOLDER_NAME "UnitTests/${LCL_DIR}")

    #MESSAGE( "FOLDER_NAME=${FOLDER_NAME}" ) 
    SET(RESOURCE_LIB_NAME ${LCL_DIR}_${name} ) 

    STRING(SUBSTRING ${RESOURCE_LIB_NAME} 1 -1 RESOURCE_LIB_NAME)
    STRING(REGEX REPLACE "(.*)/(.*)" "\\2" RESOURCE_LIB_NAME ${RESOURCE_LIB_NAME})
    #MESSAGE( "RESOURCE_LIB_NAME=${RESOURCE_LIB_NAME}" )

    project( ${RESOURCE_LIB_NAME} )

    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif()

    QT5_ADD_RESOURCES( qt_project_QRC_SRCS ${ARGN} )
    add_library(${RESOURCE_LIB_NAME} STATIC ${qt_project_QRC_SRCS})
    set_target_properties( ${RESOURCE_LIB_NAME} PROPERTIES FOLDER ${FOLDER_NAME})

    set( ${RESOURCE_LIB_NAME} ${RESOURCE_LIB_NAME} PARENT_SCOPE )
ENDFUNCTION()

FUNCTION(SAB_UNIT_TEST name file libs)
    #MESSAGE( STATUS "name=${name}" )
    #MESSAGE( STATUS "file=${file}" )
    #MESSAGE( STATUS "libs=${libs}" )
    if(POLICY CMP0020)
        cmake_policy(SET CMP0020 NEW)
    endif()

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED true)
    #MESSAGE( "CMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}" )
    #MESSAGE( "CMAKE_CURRENT_LIST_DIR=${CMAKE_CURRENT_LIST_DIR}" )

    STRING(REPLACE "\\" "/" LCL_DIR ${CMAKE_CURRENT_LIST_DIR})
    STRING(REPLACE "/" ";" LCL_DIR ${LCL_DIR})
    LIST( LENGTH LCL_DIR LCL_DIR_LEN )

    LIST( GET LCL_DIR -1 _lastDir )
    while ( ${_lastDir} STREQUAL "UnitTests" )
        LIST( GET LCL_DIR -2 _lastDir )
    endwhile()

    #MESSAGE( "_lastDir=${_lastDir}" )

    SET( TEST_NAME ${_lastDir}_${name} ) 
    project( ${TEST_NAME} )

    # message( "Adding unit test ${TEST_NAME}" )
    #MESSAGE( "ARGN=${ARGN}" )

    add_executable(${TEST_NAME} ${file} ${ARGN})
    IF( WIN32 )
        #target_link_options( ${TEST_NAME} PRIVATE /STACK:18388608 )
    ENDIF()
    target_link_libraries(${TEST_NAME} ${libs})
    add_test(${TEST_NAME} ${TEST_NAME})

    #MESSAGE( "TEST_NAME=${TEST_NAME}" )
    SET( FOLDER_NAME "UnitTests/${FOLDER_NAME}" )
    #MESSAGE( "FOLDER_NAME=${FOLDER_NAME}" )
    set_target_properties( ${TEST_NAME} PROPERTIES FOLDER ${FOLDER_NAME})
    SET (NEWPATH "${CMAKE_BINARY_DIR}/tcl/src/Debug;${CMAKE_BINARY_DIR}/tcl/src/RelWithDebInfo;${QTDIR}/bin;${OPENSSL_ROOT_DIR};$ENV{PATH}" )
    STRING(REPLACE ";" "\\;" NEWPATH "${NEWPATH}")
    SET_TESTS_PROPERTIES( ${TEST_NAME} PROPERTIES ENVIRONMENT "PATH=${NEWPATH}" )

    STRING(FIND "${libs}" "Qt5::" pos1)
    STRING(FIND "${libs}" "Qt::" pos2)

    if( NOT ( ( ${pos1} EQUAL -1 ) AND ( ${pos2} EQUAL -1 ) ) )
        MESSAGE( STATUS "Adding Qt for Test: '${TEST_NAME}'" )
        DeployQt( ${TEST_NAME} . )
    ENDIF()
    
    #MESSAGE( "FOLDER_NAME=${FOLDER_NAME}" )
    #MESSAGE( "TEST_NAME=${TEST_NAME}" )
    #MESSAGE( "          =SABTests_${TEST_NAME}" )
    #MESSAGE( "          =${TEST_NAME}" )
    SET(TARGET_NAME SABTests_${_lastDir} )
    if( NOT TARGET ${TARGET_NAME} )
        MESSAGE( STATUS "Adding custom target ${TARGET_NAME}" )
        add_custom_target( ${TARGET_NAME} )
    endif()

    ADD_DEPENDENCIES(${TARGET_NAME} ${TEST_NAME} )
    set_target_properties( ${TARGET_NAME} PROPERTIES FOLDER CMakePredefinedTargets/UnitTests )

    CreateUserProj( Test_${PROJECT_NAME} )
ENDFUNCTION()

