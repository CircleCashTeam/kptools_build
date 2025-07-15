if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/.git")
    # 读取 .git 文件内容（子模块会指向真实 Git 目录）
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/.git" GIT_FILE_CONTENT)
    string(STRIP "${GIT_FILE_CONTENT}" GIT_FILE_CONTENT)

    # 判断是否为子模块（内容格式如 `gitdir: ../.git/modules/src`）
    if(GIT_FILE_CONTENT MATCHES "gitdir: .*modules")
        message(STATUS "Detected Git submodule in src/")

        # 获取子模块的当前提交哈希
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            OUTPUT_VARIABLE COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE RESULT
        )
        if(NOT RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to get commit hash for submodule in src/")
        endif()

        # 重置子模块到该提交哈希
        execute_process(
            COMMAND ${GIT_EXECUTABLE} reset --hard ${COMMIT_HASH}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE RESET_RESULT
        )
        if(NOT RESET_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to reset submodule in src/ to commit ${COMMIT_HASH}")
        endif()

        message(STATUS "Successfully reset submodule in src/ to commit ${COMMIT_HASH}")
    endif()
endif()