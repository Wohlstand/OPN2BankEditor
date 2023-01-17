#----------------------------------------------------------------------------
# OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
# Copyright (c) 2023 Vitaly Novichkov <admin@wohlnet.ru>
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
LIBS += -lz

!macx:{
QMAKE_CXXFLAGS += -fopenmp
}

TEMPLATE = app
TARGET = opn2_bank_editor
INCLUDEPATH += $$PWD/src

macx: TARGET = "OPN2 Bank Editor"

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

include(src/opl/chips/chipset.pri)

SOURCES += \
    src/audio.cpp \
    src/bank.cpp \
    src/bank_editor.cpp \
    src/inst_list_model.cpp \
    src/operator_editor.cpp \
    src/bank_comparison.cpp \
    src/common.cpp \
    src/controlls.cpp \
    src/proxystyle.cpp \
    src/FileFormats/ffmt_base.cpp \
    src/FileFormats/ffmt_enums.cpp \
    src/FileFormats/ffmt_factory.cpp \
    src/FileFormats/format_deflemask_dmp.cpp \
    src/FileFormats/format_tfi.cpp \
    src/FileFormats/format_gems_pat.cpp \
    src/FileFormats/format_m2v_gyb.cpp \
    src/FileFormats/format_tomsoft_gin.cpp \
    src/FileFormats/format_saxman_ymx.cpp \
    src/FileFormats/format_vgm_import.cpp \
    src/FileFormats/format_gym_import.cpp \
    src/FileFormats/format_s98_import.cpp \
    src/FileFormats/format_tx81z_import.cpp \
    src/FileFormats/format_gens_y12.cpp \
    src/FileFormats/format_opm.cpp \
    src/FileFormats/format_mucom88_dat.cpp \
    src/FileFormats/format_vgi.cpp \
    src/FileFormats/format_bamboo_bti.cpp \
    src/FileFormats/format_wohlstand_opn2.cpp \
    src/FileFormats/vgm_import_options.cpp \
    src/FileFormats/ym2612_to_wopi.cpp \
    src/FileFormats/ym2151_to_wopi.cpp \
    src/FileFormats/text_format.cpp \
    src/FileFormats/text_format_tokens.cpp \
    src/formats_sup.cpp \
    src/importer.cpp \
    src/audio_config.cpp \
    src/register_editor.cpp \
    src/ins_names.cpp \
    src/main.cpp \
    src/opl/generator.cpp \
    src/opl/generator_realtime.cpp \
    src/opl/realtime/ring_buffer.cpp \
    src/opl/measurer.cpp \
    src/piano.cpp

HEADERS += \
    src/bank_editor.h \
    src/inst_list_model.h \
    src/operator_editor.h \
    src/bank_comparison.h \
    src/bank.h \
    src/common.h \
    src/proxystyle.h \
    src/FileFormats/ffmt_base.h \
    src/FileFormats/ffmt_enums.h \
    src/FileFormats/ffmt_factory.h \
    src/FileFormats/format_deflemask_dmp.h \
    src/FileFormats/format_tfi.h \
    src/FileFormats/format_gems_pat.h \
    src/FileFormats/format_m2v_gyb.h \
    src/FileFormats/format_tomsoft_gin.h \
    src/FileFormats/format_saxman_ymx.h \
    src/FileFormats/format_vgm_import.h \
    src/FileFormats/format_gym_import.h \
    src/FileFormats/format_s98_import.h \
    src/FileFormats/format_tx81z_import.h \
    src/FileFormats/format_gens_y12.h \
    src/FileFormats/format_opm.h \
    src/FileFormats/format_mucom88_dat.h \
    src/FileFormats/format_vgi.h \
    src/FileFormats/format_bamboo_bti.h \
    src/FileFormats/format_wohlstand_opn2.h \
    src/FileFormats/vgm_import_options.h \
    src/FileFormats/ym2612_to_wopi.h \
    src/FileFormats/ym2151_to_wopi.h \
    src/FileFormats/text_format.h \
    src/FileFormats/text_format_tokens.h \
    src/formats_sup.h \
    src/importer.h \
    src/audio_config.h \
    src/register_editor.h \
    src/ins_names.h \
    src/ins_names_data.h \
    src/main.h \
    src/opl/generator.h \
    src/opl/generator_realtime.h \
    src/opl/measurer.h \
    src/opl/realtime/ring_buffer.h \
    src/opl/realtime/ring_buffer.tcc \
    src/piano.h \
    src/version.h

FORMS += \
    src/bank_editor.ui \
    src/operator_editor.ui \
    src/bank_comparison.ui \
    src/formats_sup.ui \
    src/importer.ui \
    src/audio_config.ui \
    src/register_editor.ui

RESOURCES += \
    src/resources/resources.qrc

TRANSLATIONS += \
    src/translations/opn2bankeditor_fr_FR.ts \
    src/translations/opn2bankeditor_ru_RU.ts \
    src/translations/opn2bankeditor_pl_PL.ts


plots {
    SOURCES += src/delay_analysis.cpp
    HEADERS += src/delay_analysis.h
    FORMS += src/delay_analysis.ui
    CONFIG += qwt
    DEFINES += ENABLE_PLOTS
}
