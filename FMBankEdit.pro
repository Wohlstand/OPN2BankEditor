#----------------------------------------------------------------------------
# OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
# Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#----------------------------------------------------------------------------

#-------------------------------------------------
#
# Project created by QtCreator 2016-06-07T13:26:28
#
#-------------------------------------------------

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4):{
    QT += widgets concurrent
    DEFINES += ENABLE_AUDIO_TESTING
    CONFIG += c++11
} else {
    QMAKE_CXXFLAGS += -std=c++11
    DEFINES += IS_QT_4
    win32: {
        CONFIG += static
        QMAKE_LFLAGS += -static-libgcc -static-libstdc++ -static
        #DEFINES += snprintf=_snprintf
        DEFINES += NO_NATIVE_OPEN_DIALOGS
        INCLUDEPATH += $$PWD/src/audio/for-mingw-9x
    }
}
win32 {
    DEFINES += _USE_MATH_DEFINES
}

CONFIG += rtmidi
CONFIG += rtaudio
#CONFIG += plots

!macx:{
QMAKE_CXXFLAGS += -fopenmp
}

TEMPLATE = app
TARGET = opn2_bank_editor
INCLUDEPATH += $$PWD/

android:{
    ARCH=android_arm
} else {
    !contains(QMAKE_TARGET.arch, x86_64) {
        ARCH=x32
    } else {
        ARCH=x64
    }
}

debug: {
BUILDTP=debug
    DEFINES += DEBUG_BUILD=1
    DESTDIR = $$PWD/bin-debug/
} else: release: {
    BUILDTP=release
    DESTDIR = $$PWD/bin-release/
}

BUILD_OBJ_DIR = $$PWD/_build_data

OBJECTS_DIR = $$BUILD_OBJ_DIR/_build_$$ARCH/$$TARGET/_$$BUILDTP/.obj
MOC_DIR     = $$BUILD_OBJ_DIR/_build_$$ARCH/$$TARGET/_$$BUILDTP/.moc
RCC_DIR     = $$BUILD_OBJ_DIR/_build_$$ARCH/$$TARGET/_$$BUILDTP/.rcc
UI_DIR      = $$BUILD_OBJ_DIR/_build_$$ARCH/$$TARGET/_$$BUILDTP/.ui

win32: RC_FILE = $$PWD/src/resources/res.rc
macx: ICON = $$PWD/src/resources/opn2.icns

rtaudio {
    include("src/audio/ao_rtaudio.pri")
}
rtmidi {
    DEFINES += ENABLE_MIDI
    include("src/midi/midi_rtmidi.pri")
}

SOURCES += \
    src/audio.cpp \
    src/bank.cpp \
    src/bank_editor.cpp \
    src/common.cpp \
    src/controlls.cpp \
    src/FileFormats/ffmt_base.cpp \
    src/FileFormats/ffmt_factory.cpp \
    src/FileFormats/format_deflemask_dmp.cpp \
    src/FileFormats/format_tfi.cpp \
    src/FileFormats/format_vgm_import.cpp \
    src/FileFormats/format_wohlstand_opn2.cpp \
    src/formats_sup.cpp \
    src/importer.cpp \
    src/latency.cpp \
    src/ins_names.cpp \
    src/main.cpp \
    src/opl/generator.cpp \
    src/opl/generator_realtime.cpp \
    src/opl/realtime/ring_buffer.cpp \
    src/opl/measurer.cpp \
    src/opl/chips/gens/Ym2612_Emu.cpp \
    src/piano.cpp \
    src/opl/chips/gens_opn2.cpp \
    src/opl/chips/nuked_opn2.cpp \
    src/opl/chips/nuked/ym3438.c \
    src/opl/chips/mame_opn2.cpp \
    src/opl/chips/mame/mame_ym2612fm.c \
    src/opl/chips/gx_opn2.cpp \
    src/opl/chips/gx/gx_ym2612.c

HEADERS += \
    src/bank_editor.h \
    src/bank.h \
    src/common.h \
    src/FileFormats/ffmt_base.h \
    src/FileFormats/ffmt_enums.h \
    src/FileFormats/ffmt_factory.h \
    src/FileFormats/format_deflemask_dmp.h \
    src/FileFormats/format_tfi.h \
    src/FileFormats/format_vgm_import.h \
    src/FileFormats/format_wohlstand_opn2.h \
    src/formats_sup.h \
    src/importer.h \
    src/latency.h \
    src/ins_names.h \
    src/main.h \
    src/opl/generator.h \
    src/opl/generator_realtime.h \
    src/opl/measurer.h \
    src/opl/realtime/ring_buffer.h \
    src/opl/realtime/ring_buffer.tcc \
    src/opl/chips/gens/Ym2612_Emu.h \
    src/piano.h \
    src/version.h \
    src/opl/chips/gens_opn2.h \
    src/opl/chips/opn_chip_base.h \
    src/opl/chips/opn_chip_base.tcc \
    src/opl/chips/nuked_opn2.h \
    src/opl/chips/nuked/ym3438.h \
    src/opl/chips/mame_opn2.h \
    src/opl/chips/gx_opn2.h \
    src/opl/chips/mame/mamedef.h \
    src/opl/chips/mame/mame_ym2612fm.h \
    src/opl/chips/gx/gx_ym2612.h

FORMS += \
    src/bank_editor.ui \
    src/formats_sup.ui \
    src/importer.ui

RESOURCES += \
    src/resources/resources.qrc

TRANSLATIONS += \
    src/translations/opn2bankeditor_fr_FR.ts

plots {
    SOURCES += src/delay_analysis.cpp
    HEADERS += src/delay_analysis.h
    FORMS += src/delay_analysis.ui
    CONFIG += qwt
    DEFINES += ENABLE_PLOTS
}
