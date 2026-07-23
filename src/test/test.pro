# Unit tests for the Jamulus core (QtTest based)
#
# Build and run (out of tree builds are recommended):
#   mkdir build-test && cd build-test
#   qmake ../src/test/test.pro
#   make && ./jamulus-test
#
# "make check" is supported as well (CONFIG += testcase).

TARGET   = jamulus-test
TEMPLATE = app

CONFIG += qt \
    thread \
    console \
    testcase

CONFIG -= app_bundle

# the sources under test are compiled with the same language level as the
# regular unix build of Jamulus.pro (C++11)
CONFIG += c++11

QT += network \
    testlib

QT -= gui

# compile the sources under test in a headless server-only configuration so
# that no GUI or sound interface dependencies are pulled in
DEFINES += APP_VERSION=\\\"unittest\\\" \
    SERVER_ONLY \
    HEADLESS \
    NO_JSON_RPC \
    HAVE_STDINT_H

# same as in Jamulus.pro: prevent the windows.h min/max macros from breaking
# std::min/std::max usage in the sources under test
win32 {
    DEFINES += NOMINMAX
}

INCLUDEPATH += ..

HEADERS += ../global.h \
    ../protocol.h \
    ../util.h \
    protocoltester.h

SOURCES += ../protocol.cpp \
    ../util.cpp \
    tst_protocol.cpp
