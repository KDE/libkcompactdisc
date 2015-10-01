# test - sample test everything app

add_executable(ripdisc ripdisc.cpp)
target_link_libraries(ripdisc KF5::CompactDisc ${CDIO_LIBRARIES})
