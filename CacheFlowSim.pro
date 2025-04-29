QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CacheFlowSim
TEMPLATE = app

SOURCES += \
    SimulatorWindow.cpp \
    basicsimulator.cpp \
    memoryUI.cpp \
    main.cpp

HEADERS += \
    SimulatorWindow.h

# Include path if needed (usually not necessary if files are in same folder)
# INCLUDEPATH += .

CONFIG += c++11

# Optional: faster compilation
QMAKE_CXXFLAGS += -O2
