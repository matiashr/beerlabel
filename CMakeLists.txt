cmake_minimum_required(VERSION 3.16)
project(BeerLabelDesigner CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets REQUIRED COMPONENTS core base xml html)
include(${wxWidgets_USE_FILE})

add_executable(beerlabel
    src/main.cpp
    src/MainFrame.cpp
    src/LabelModel.cpp
    src/LabelCanvas.cpp
    src/SvgIO.cpp
    src/LabelPrintout.cpp
)

target_link_libraries(beerlabel ${wxWidgets_LIBRARIES})
