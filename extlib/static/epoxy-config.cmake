if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(_EPOXY_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../libepoxy/build64static")
else()
	set(_EPOXY_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../libepoxy/build32static")
endif()
add_library(epoxy::epoxy STATIC IMPORTED)
set_target_properties(epoxy::epoxy PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../libepoxy/include;${_EPOXY_PREFIX}/include"
	INTERFACE_COMPILE_DEFINITIONS "EPOXY_PUBLIC=extern"
	IMPORTED_LOCATION "${_EPOXY_PREFIX}/src/libepoxy.a"
)
set(_EPOXY_PREFIX)
