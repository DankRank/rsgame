if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(_EPOXY_PREFIX "${CMAKE_CURRENT_LIST_DIR}/libepoxy/build64")
else()
	set(_EPOXY_PREFIX "${CMAKE_CURRENT_LIST_DIR}/libepoxy/build32")
endif()
add_library(epoxy::epoxy SHARED IMPORTED)
set_target_properties(epoxy::epoxy PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/libepoxy/include;${_EPOXY_PREFIX}/include"
	IMPORTED_LOCATION "${_EPOXY_PREFIX}/src/epoxy-0.dll"
	IMPORTED_IMPLIB "${_EPOXY_PREFIX}/src/epoxy.lib"
)
set(_EPOXY_PREFIX)
