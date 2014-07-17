macro(test name)
add_executable(${name} ${name.cpp})
endmacro()
test(test_monitr)
