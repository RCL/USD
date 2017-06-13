# [RCL] disable building binaries for now since we aren't redistributing them
set(DIRS
    lib
#    bin
    plugin
)

foreach(d ${DIRS})
    add_subdirectory(${d})
endforeach()

