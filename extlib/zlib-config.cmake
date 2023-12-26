if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(_ZLIB_PREFIX "${CMAKE_CURRENT_LIST_DIR}/zlib/build64")
else()
	set(_ZLIB_PREFIX "${CMAKE_CURRENT_LIST_DIR}/zlib/build32")
endif()
set(ZLIB_INCLUDE_DIRS "${_ZLIB_PREFIX}/include")
set(ZLIB_LIBRARIES "${_ZLIB_PREFIX}/Release/zlib.lib")
add_library(ZLIB::ZLIB SHARED IMPORTED)
set_target_properties(ZLIB::ZLIB PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${_ZLIB_PREFIX}/include"
	IMPORTED_LOCATION "${_ZLIB_PREFIX}/Release/zlib.dll"
	IMPORTED_IMPLIB "${_ZLIB_PREFIX}/Release/zlib.lib"
)
set(_ZLIB_PREFIX)
