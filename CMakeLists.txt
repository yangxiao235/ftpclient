cmake_minimum_required(VERSION 3.12)

project(ftpclient)

set(GLOG_INC_DIR   "E:\\Source Code Repositories\\git\\glog\\src\\windows")
set(GLOG_LIB_DIR   "E:\\Source Code Repositories\\git\\glog\\__build\\Debug")
set(BOOST_INC_DIR  "E:\\Source Code Repositories\\git\\boost_1_71_0\\boost_1_71_0")
set(BOOST_LIB_DIR  "E:\\Source Code Repositories\\git\\boost_1_71_0\\boost_1_71_0\\lib")
set(ASIO_INC_DIR   "E:\\Source Code Repositories\\git\\asio\\asio\\include")
set(PROECT_INC_DIR "E:\\My Projects\\worker_thread\\prototype")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)
# Windows DLLs are "runtime" for CMake. Output them to "bin" like the Visual Studio projects do.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

include_directories(
    ${GLOG_INC_DIR}
    ${BOOST_INC_DIR}  
    ${ASIO_INC_DIR}
    ${PROECT_INC_DIR}
     )
link_directories(
    ${GLOG_LIB_DIR}  
    ${BOOST_LIB_DIR}  
    )
     
add_library(libftpclient STATIC 
    ftp.cpp
    async_cmd/async_cmd.cpp
    cmd_task/ftp_cmd_queue.cpp
    data_trans_task/file_save_policy.cpp
    notify/ftp_notify_queue.cpp
    notify/ftp_notify_msg.cpp
    thread_model/thread_task_queue.cpp
    thread_model/thread_task.cpp
    thread_model/thread_state.cpp
    thread_model/thread_model.cpp
    )

add_subdirectory(test)