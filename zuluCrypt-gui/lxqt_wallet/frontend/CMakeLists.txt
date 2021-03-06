
cmake_minimum_required(VERSION 2.6)
cmake_policy(SET CMP0017 OLD)

INCLUDE(FindPkgConfig)

#
# set below variable to "false" if you want to build a static library
#
set(BUILD_SHARED "false")

SET(QT_USE_QTCORE TRUE)
SET(QT_USE_QTGUI TRUE)

set(LIBRARY_VERSION "2.0.0")

add_definitions(-I${PROJECT_BINARY_DIR}/frontend)
add_definitions(-I${PROJECT_BINARY_DIR}/backend)
add_definitions(-I${PROJECT_BINARY_DIR})

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

if(QT5)
	set(NOKDESUPPORT "true")

	find_package(Qt5Widgets REQUIRED)
	find_package(Qt5Core REQUIRED)

	set(CMAKE_INCLUDE_CURRENT_DIR ON)
	include_directories(${Qt5Widgets_INCLUDE_DIRS})
	
	add_definitions(${Qt5Widgets_DEFINITIONS})
else()
	find_package(Qt4 REQUIRED)

	add_definitions(-I${Qt4_INCLUDE_DIR})
	add_definitions(-I${QT_HEADERS_DIR})

	INCLUDE(${QT_USE_FILE})
	
	INCLUDE(${QT_USE_FILE})
endif()

if(NOSECRETSUPPORT)
	SET(SECRET_SUPPORT "false")
	file(WRITE  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_SECRET_SUPPORT 0\n")
else()
	pkg_check_modules(LIBSECRET libsecret-1)

	if(LIBSECRET_FOUND)
		add_subdirectory(secret_service)
		SET(SECRET_SUPPORT "true")
		file(WRITE  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_SECRET_SUPPORT 1\n")
	else()
		SET(SECRET_SUPPORT "false")
		file(WRITE  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_SECRET_SUPPORT 0\n")
	endif()
endif()

if(NOKDESUPPORT)
	add_definitions(-std=c++0x)
	SET(KDE_SUPPORT "false")
	SET(KWALLET_SUPPORT "false")
	file(APPEND  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_KWALLET_SUPPORT 0\n")
else()
	find_package(KDE4 QUIET)
	if(KDE4_FOUND)
		include(KDE4Defaults)
		include_directories(${KDE4_INCLUDES})
		add_definitions(-I${KDE4_INCLUDE_DIR})

		add_definitions(-I${Qt4_INCLUDE_DIR})
		add_definitions(-I${QT_HEADERS_DIR})

		SET(KDE_SUPPORT "true")

		if(NOKWALLETSUPPORT)
			SET(KWALLET_SUPPORT "false")
			file(APPEND  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_KWALLET_SUPPORT 0\n")
		else()
			find_library(LIBKWALLETBACKEND libkwalletbackend.so)
			if(NOT LIBKWALLETBACKEND)
				# some distributions do not have libkwalletbackend.so for reasons i currently do not know
				# look for libkwalletbackend.sp.4 when the above happen
				find_library(LIBKWALLETBACKEND libkwalletbackend.so.4)
				file(APPEND  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_KWALLET_SUPPORT 0\n")
				SET(KWALLET_SUPPORT "false")
			else()
				SET(KWALLET_SUPPORT "true")
				file(APPEND  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_KWALLET_SUPPORT 1\n")
			endif()

			if(NOT LIBKWALLETBACKEND)
				SET(KWALLET_SUPPORT "false")
				file(APPEND  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_KWALLET_SUPPORT 0\n")
				#message(FATAL_ERROR "could not find libkwalletbackend.so or libkwalletbackend.so.4,please make sure you have kwallet development libraries installed and try again")
			else()
				SET(KWALLET_SUPPORT "true")
				file(APPEND  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_KWALLET_SUPPORT 1\n")
			endif()
		endif()
	
		find_library(LIBKDEUI libkdeui.so)
		if(NOT LIBKDEUI)
			# adding this one just in case the same case as above happen
			find_library(LIBKDEUI libkdeui.so.5)
		endif()

		if(NOT LIBKDEUI)
			SET(KDE_SUPPORT "false")
			message(STATUS "could not find libkdeui.so or libkdeui.so.5,please make sure you have kde development libraries installed and try again")
		endif()

		find_library(LIBKDECORE libkdecore.so)
		if(NOT LIBKDECORE)
			# adding this one just in case the same case as above happen
			find_library(LIBKDECORE libkdecore.so.5)
		endif()

		if(NOT LIBKDECORE)
			SET(KDE_SUPPORT "false")
			message(STATUS "could not find libkdecore.so or libkdecore.so.5,please make sure you have kde development libraries installed and try again")
		endif()
	else()
		add_definitions(-std=c++0x)
		file(APPEND  ${PROJECT_BINARY_DIR}/storage_manager.h "\n#define HAS_KWALLET_SUPPORT 0\n")
		SET(KDE_SUPPORT "false")
		SET(KWALLET_SUPPORT "false")
	endif()
endif()

if(KWALLET_SUPPORT)
	message(STATUS "\n--------------------------------------------------------------------------")
	message(STATUS "kwallet support found,will build kwallet functionality")
	message(STATUS "-----------------------------------------------------------------------")
else()
	message(STATUS "\n--------------------------------------------------------------------------")
	message(STATUS "kwallet support NOT found,will not build kwallet functionality")
	message(STATUS "-----------------------------------------------------------------------")
endif()

if(SECRET_SUPPORT)
	message(STATUS "\n--------------------------------------------------------------------------")
	message(STATUS "libsecret support found,will build libsecret functionality")
	message(STATUS "-----------------------------------------------------------------------")
else()
	message(STATUS "\n--------------------------------------------------------------------------")
	message(STATUS "libsecret support NOT found,will not build libsecret functionality")
	message(STATUS "-----------------------------------------------------------------------")
endif()

set(UI_FILES 	changepassworddialog.ui password_dialog.ui)

set(MOC_FILES 	changepassworddialog.h lxqt_internal_wallet.h task.h password_dialog.h)
set(MOC_FILES_1 changepassworddialog.h lxqt_internal_wallet.h task.h password_dialog.h lxqt_kwallet.h)
set(MOC_FILES_2 changepassworddialog.h lxqt_internal_wallet.h task.h password_dialog.h lxqt_kwallet.h lxqt_secret_service.h)
set(MOC_FILES_3 changepassworddialog.h lxqt_internal_wallet.h task.h password_dialog.h lxqt_secret_service.h)

set(SRC 	changepassworddialog.cpp password_dialog.cpp lxqt_internal_wallet.cpp lxqt_wallet.cpp task.cpp)
set(SRC_1 	changepassworddialog.cpp password_dialog.cpp lxqt_internal_wallet.cpp lxqt_wallet.cpp task.cpp lxqt_kwallet.cpp)
set(SRC_2 	changepassworddialog.cpp password_dialog.cpp lxqt_internal_wallet.cpp lxqt_wallet.cpp task.cpp lxqt_kwallet.cpp lxqt_secret_service.cpp)
set(SRC_3 	changepassworddialog.cpp password_dialog.cpp lxqt_internal_wallet.cpp lxqt_wallet.cpp task.cpp lxqt_secret_service.cpp)

if(BUILD_SHARED)
	set( LIBRARY_TYPE "SHARED")
else()
	set( LIBRARY_TYPE "STATIC")
endif()

if(QT5)

Qt5_WRAP_UI(UI ${UI_FILES})

if(KDE_SUPPORT)
	if(KWALLET_SUPPORT)
		if(SECRET_SUPPORT)
			Qt5_WRAP_CPP(MOC ${MOC_FILES_2})
			add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC_2} ${MOC})
			target_link_libraries(lxqtwallet secretService)
			target_link_libraries(lxqtwallet "${Qt5Widgets_LIBRARIES}")
			target_link_libraries(lxqtwallet "${QtCore_LIBRARIES}")
			target_link_libraries(lxqtwallet "${LIBKDEUI}")
			target_link_libraries(lxqtwallet "${LIBKDECORE}")
			target_link_libraries(lxqtwallet "${LIBKWALLETBACKEND}")
		else()
			Qt5_WRAP_CPP(MOC ${MOC_FILES_1})
			target_link_libraries(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC_1} ${MOC})
			target_link_libraries(lxqtwallet "${Qt5Widgets_LIBRARIES}")
			target_link_libraries(lxqtwallet "${QtCore_LIBRARIES}")
			target_link_libraries(lxqtwallet "${LIBKDEUI}")
			target_link_libraries(lxqtwallet "${LIBKDECORE}")
			target_link_libraries(lxqtwallet "${LIBKWALLETBACKEND}")
			target_link_libraries(lxqtwallet "${Qt5Widgets_LIBRARIES}")
			target_link_libraries(lxqtwallet "${QtCore_LIBRARIES}")
		endif()
	else()
		if(SECRET_SUPPORT)
			Qt5_WRAP_CPP(MOC ${MOC_FILES_3})
			add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC_3} ${MOC})
			target_link_libraries(lxqtwallet secretService)
			target_link_libraries(lxqtwallet "${Qt5Widgets_LIBRARIES}")
			target_link_libraries(lxqtwallet "${QtCore_LIBRARIES}")
		else()
			Qt5_WRAP_CPP(MOC ${MOC_FILES})
			add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC} ${MOC})
			target_link_libraries(lxqtwallet "${Qt5Widgets_LIBRARIES}")
			target_link_libraries(lxqtwallet "${QtCore_LIBRARIES}")
			target_link_libraries(lxqtwallet "${LIBKDEUI}")
			target_link_libraries(lxqtwallet "${LIBKDECORE}")
		endif()
	endif()
else()
	if(SECRET_SUPPORT)
		Qt5_WRAP_CPP(MOC ${MOC_FILES_3})
		add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC_3} ${MOC})
		target_link_libraries(lxqtwallet secretService)
		target_link_libraries(lxqtwallet "${Qt5Widgets_LIBRARIES}")
		target_link_libraries(lxqtwallet "${QtCore_LIBRARIES}")
	else()
		Qt5_WRAP_CPP(MOC ${MOC_FILES})
		add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC} ${MOC})
		target_link_libraries(lxqtwallet "${Qt5Widgets_LIBRARIES}")
		target_link_libraries(lxqtwallet "${QtCore_LIBRARIES}")
	endif()
endif()

else()
	
Qt4_WRAP_UI(UI ${UI_FILES})

if(KDE_SUPPORT)
	if(KWALLET_SUPPORT)
		if(SECRET_SUPPORT)
			Qt4_WRAP_CPP(MOC ${MOC_FILES_2})
			add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC_2} ${MOC})
			target_link_libraries(lxqtwallet secretService)
			set(link_libraries "-L${KDE4_LIB_DIR} -L${QT_LIBRARY_DIR} ${LIBKDEUI} ${LIBKDECORE} ${LIBKWALLETBACKEND}")
		else()
			Qt4_WRAP_CPP(MOC ${MOC_FILES_1})
			add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC_1} ${MOC})
			set(link_libraries "-L${KDE4_LIB_DIR} -L${QT_LIBRARY_DIR} ${LIBKDEUI} ${LIBKDECORE} ${LIBKWALLETBACKEND}")
		endif()
	else()
		if(SECRET_SUPPORT)
			Qt4_WRAP_CPP(MOC ${MOC_FILES_3})
			add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC_3} ${MOC})
			target_link_libraries(lxqtwallet secretService)
			set(link_libraries "-L${KDE4_LIB_DIR} -L${QT_LIBRARY_DIR}")
		else()
			Qt4_WRAP_CPP(MOC ${MOC_FILES})
			add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC} ${MOC})
			set(link_libraries "-L${KDE4_LIB_DIR} -L${QT_LIBRARY_DIR} ${LIBKDEUI} ${LIBKDECORE}")
		endif()

	endif()
else()
	if(SECRET_SUPPORT)
		Qt4_WRAP_CPP(MOC ${MOC_FILES_3})
		add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC_3} ${MOC})
		target_link_libraries(lxqtwallet secretService)
		set(link_libraries "-L${KDE4_LIB_DIR} -L${QT_LIBRARY_DIR}")
	else()
		Qt4_WRAP_CPP(MOC ${MOC_FILES})
		add_library(lxqtwallet ${LIBRARY_TYPE} ${UI} ${SRC} ${MOC})
	endif()
endif()

endif()

set_target_properties(lxqtwallet PROPERTIES COMPILE_FLAGS "-D_FILE_OFFSET_BITS=64 -Wextra -Wall -s -fPIC -pedantic ")
set_target_properties(lxqtwallet PROPERTIES SOVERSION "${LIBRARY_VERSION}")
TARGET_LINK_LIBRARIES(lxqtwallet "${link_libraries}" lxqtwallet-backend -lQtCore -lQtGui)

if(LIB_SUFFIX)
	set(LIB_PATH "${CMAKE_INSTALL_PREFIX}/${LIB_SUFFIX}")
else()
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(LIB_PATH "${CMAKE_INSTALL_PREFIX}/lib64")
	else()
		set(LIB_PATH "${CMAKE_INSTALL_PREFIX}/lib")
	endif()
endif()

if(BUILD_SHARED)
	install(TARGETS lxqtwallet RUNTIME LIBRARY DESTINATION "${LIB_PATH}")

	install(FILES lxqt_wallet.h DESTINATION "${CMAKE_INSTALL_PREFIX}/include/lxqt")
	
	file( WRITE ${PROJECT_BINARY_DIR}/lxqtwallet.pc
"prefix=${CMAKE_INSTALL_PREFIX}
exec_prefix=${CMAKE_INSTALL_PREFIX}
libdir=${LIB_PATH}
includedir=${CMAKE_INSTALL_PREFIX}/include/lxqt
	
Name: lxqt_wallet
Description: lxqt secure storage system
Version: ${LIBRARY_VERSION}
Libs: -L${LIB_PATH} -lQtGui -lQtCore -lgcrypt
Cflags: -I${CMAKE_INSTALL_PREFIX}/include/lxqt
\n")

	install(FILES ${PROJECT_BINARY_DIR}/lxqtwallet.pc DESTINATION ${LIB_PATH}/pkgconfig/ PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )
endif()
