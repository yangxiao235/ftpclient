cmake_minimum_required(VERSION 3.12)

project(ftpclient)

add_executable(test_async_cmd test_async_cmd.cpp)
target_link_libraries(test_async_cmd libftpclient glogd)

add_executable(test_data_trans_task_for_basic_service 
        test_data_trans_task_for_basic_service.cpp)
target_link_libraries(test_data_trans_task_for_basic_service
    libftpclient glogd)
        
add_executable(test_default_thread test_default_thread.cpp)
target_link_libraries(test_default_thread
    libftpclient glogd)
    
add_executable(test_down_file_request test_down_file_request.cpp)
target_link_libraries(test_down_file_request
    libftpclient glogd)
    
#add_executable(test_ftp_client_task test_ftp_client_task.cpp)
#target_link_libraries(test_ftp_client_task
#    libftpclient glogd)
    
add_executable(test_notify test_notify.cpp)
target_link_libraries(test_notify
    libftpclient glogd)
    
add_executable(test_task_error test_task_error.cpp)
target_link_libraries(test_data_trans_task_for_basic_service
    libftpclient glogd)
    
add_executable(test_thread_model test_thread_model.cpp)
target_link_libraries(test_thread_model
    libftpclient glogd)
    
add_executable(test_thread_task test_thread_task.cpp)
target_link_libraries(test_thread_task
    libftpclient glogd)
    
add_executable(test_basic_ftpclient 
   test_basic_ftpclient/input_queue.cpp
   test_basic_ftpclient/monitor_stdio_task.cpp
   test_basic_ftpclient/test_basic_ftp_client.cpp)
target_link_libraries(test_basic_ftpclient
    libftpclient glogd)   

add_executable(test_ftp_notify_msg  test_ftp_notify_msg.cpp)    

add_executable(test_upload_file  test_upload_file.cpp)
target_link_libraries(test_upload_file 
    libftpclient glogd)

add_executable(test_download_file  test_download_file.cpp)
target_link_libraries(test_download_file 
    libftpclient glogd)   

add_executable(test_download_dir  test_download_dir_contents.cpp)
target_link_libraries(test_download_dir 
    libftpclient glogd)      