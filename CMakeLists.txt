cmake_minimum_required(VERSION 3.12...3.22)
project(BasicHttpServer
  VERSION 0.0.1
  LANGUAGES CXX C
)



set(CMAKE_CXX_STANDARD 20)
set(SOURCE_FILES 
  "src/main.cpp"
  "src/Server.cpp"
  "src/HttpRequest.cpp"
)
set(HEADER_FILES 
  "src/Server.h"
  "src/HttpRequest.h"
  "src/HttpResponse.h"
)


add_executable("${PROJECT_NAME}" "${SOURCE_FILES}" "${HEADER_FILES}")

target_include_directories("${PROJECT_NAME}" 
  PRIVATE
  "${PROJECT_SOURCE_DIR}"
)
set_target_properties("${PROJECT_NAME}"
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
)
target_include_directories("${PROJECT_NAME}"
  PRIVATE "${PROJECT_SOURCE_DIR}/src"
)


