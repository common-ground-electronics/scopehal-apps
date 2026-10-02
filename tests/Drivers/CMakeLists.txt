add_executable(Drivers
	main.cpp

	TektronixPreamble.cpp
)

target_link_libraries(Drivers
	scopehal
	Catch2::Catch2
	)

#Needed because Windows does not support RPATH and will otherwise not be able to find DLLs when catch_discover_tests runs the executable
if(WIN32)
add_custom_command(TARGET Drivers POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:Drivers> $<TARGET_FILE_DIR:Drivers>
	COMMAND_EXPAND_LISTS
	)
endif()

catch_discover_tests(Drivers)
