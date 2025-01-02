cmake_minimum_required (VERSION 3.5.1)
project(mqtt_webrtc)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED Core Sql Network Gui Widgets Charts)

if(MSVC)
# windows -------------------------------------------------------------
    add_definitions(-DWEBRTC_WIN -DNOMINMAX -DWIN32_LEAN_AND_MEAN -DASIO_STANDALONE -DPAHO_MQTTPP_IMPORTS -DQT_NO_KEYWORDS)
    set(third_DIR ${PROJECT_SOURCE_DIR}/third)
    set(asio_DIR ${third_DIR}/asio)
    set(webrtc_DIR ${third_DIR}/webrtc)
    set(spdlog_DIR ${third_DIR}/spdlog)
    set(mqttcpp_DIR ${third_DIR}/mqttcpp_debug)
    set(vcprtc_DIR ${PROJECT_SOURCE_DIR}/vcprtc)
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd /ZI /Od")
# ----------------------------------------------------------------------
else()
    
    set(CMAKE_C_FLAGS " -O3 ")
    set(CMAKE_CXX_FLAGS " -O3 ")
endif(MSVC)

include_directories(${webrtc_DIR}/include 
                    ${webrtc_DIR}/include/third_party/abseil-cpp
                    ${webrtc_DIR}/include/third_party/libyuv/include
                    ${asio_DIR}
					${spdlog_DIR}/include 
					${mqttcpp_DIR}/include
                    ${vcprtc_DIR}
                    ${vcprtc_DIR}/jsoncpp
                    apps/app
					)
file(GLOB_RECURSE vcprtc_SOURCES ${vcprtc_DIR}/*.cpp)
file(GLOB_RECURSE vcprtc_HEADERS ${vcprtc_DIR}/*.h)
file(GLOB_RECURSE logger_SOURCES ${vcprtc_DIR}/logger/*.cpp)
file(GLOB_RECURSE logger_HEADERS ${vcprtc_DIR}/logger/*.h)
file(GLOB_RECURSE signalclient_SOURCES ${vcprtc_DIR}/SignalClient/*.cpp)
file(GLOB_RECURSE signalclient_HEADERS ${vcprtc_DIR}/SignalClient/*.h)
file(GLOB_RECURSE app_SOURCES apps/app/*.cpp)
file(GLOB_RECURSE app_HEADERS apps/app/*.h)
file(GLOB_RECURSE ui_FORMS apps/app/*.ui) 
QT5_WRAP_UI(ui_FORMS_HEADERS ${ui_FORMS})
set(mqtt_webrtc_QRC apps/app/mqtt_webrtc.qrc)

set(ALL_Sources 
	${vcprtc_SOURCES} ${vcprtc_HEADERS}
    ${logger_SOURCES} ${logger_HEADERS}
    ${signalclient_SOURCES} ${signalclient_HEADERS}
    ${app_SOURCES} ${app_HEADERS}
	${ui_FORMS} ${ui_FORMS_HEADERS} # apps/test.cpp
    )
   
qt5_add_resources(mqtt_webrtc_QRC_SOURCES ${mqtt_webrtc_QRC})


macro(source_group_by_dir source_files)
    if(MSVC)
        set(sgbd_cur_dir ${CMAKE_CURRENT_SOURCE_DIR})
        foreach(sgbd_file ${${source_files}})
            string(REGEX REPLACE ${sgbd_cur_dir}/\(.*\) \\1 sgbd_fpath ${sgbd_file})
            string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            if(sgbd_nogroup)
                set(sgbd_group_name "\\")
            endif(sgbd_nogroup)
            source_group(${sgbd_group_name} FILES ${sgbd_file})
        endforeach(sgbd_file)
    endif(MSVC)
endmacro(source_group_by_dir)

if(MSVC)
source_group_by_dir(ALL_Sources)
source_group(forms FILES ${ui_FORMS} ${ui_FORMS_HEADERS})
add_executable(mqtt_webrtc # WIN32
    ${ALL_Sources} ${mqtt_webrtc_QRC_SOURCES})

target_link_libraries(mqtt_webrtc PRIVATE 
    ${mqttcpp_DIR}/lib/paho-mqtt3a.lib
    ${mqttcpp_DIR}/lib/paho-mqttpp3.lib
    ${webrtc_DIR}/lib/webrtc.lib
    winmm.lib
    iphlpapi.lib
    crypt32.lib
    RpcRT4.lib
    ws2_32.lib
    kernel32.lib
    user32.lib
    gdi32.lib
    Qt5::Core
	Qt5::Widgets
    Qt5::Gui
	Qt5::Network
	Qt5::Sql
	Qt5::Charts
    )
else()
# add_executable(mqtt_webrtc
#     ${ALL_Sources} ${mqtt_webrtc_QRC_SOURCES})
# target_compile_definitions(mqtt_webrtc PRIVATE CHECK_LICENSE_TYPE=2)
# target_link_libraries(mqtt_webrtc PRIVATE 
#     ${OpenCV_DIR}/lib/libopencv_world.so
# 	${fftw3_DIR}/lib/libfftw3.so
# 	${fftw3_DIR}/lib/libfftw3f.so
# 	${HikVison_DIR}/lib/aarch64/libMvCameraControl.so
# 	${libusb_lib}
#     ${breakpad_DIR}/lib/libbreakpad.a
#     ${breakpad_DIR}/lib/libbreakpad_client.a
# 	pthread
# 	gomp
#     Qt5::Core
# 	Qt5::Widgets
#     Qt5::Gui
# 	Qt5::Network
# 	Qt5::Sql
# 	Qt5::Charts)

endif(MSVC)



#install(FILES ${extra_install} src/startup.bmp DESTINATION ${CMAKE_INSTALL_PREFIX})
#install(TARGETS mqtt_webrtc)
