project(Chips)

add_executable(Chips src/main.c)

add_subdirectory(deps/minifb)

target_link_libraries(Chips
	minifb
)
