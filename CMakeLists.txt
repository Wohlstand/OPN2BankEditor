cmake_minimum_required(VERSION 3.2)
project(OPN2BankEditor LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

set(USE_RTMIDI "ON" CACHE STRING "Use RtMidi")
set(USE_RTAUDIO "ON" CACHE STRING "Use RtAudio")

include(FindPkgConfig)

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Threads REQUIRED)

message("!! Optional feature summary:")
message("!!   RtMidi: ${USE_RTMIDI}")
message("!!   RtAudio: ${USE_RTAUDIO}")

if(USE_RTAUDIO)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    pkg_check_modules(JACK "jack")
    message("!!   -- Jack driver: ${JACK_FOUND}")
    pkg_check_modules(PULSEAUDIO "libpulse-simple")
    message("!!   -- Pulseaudio driver: ${PULSEAUDIO_FOUND}")
  endif()
endif()

include_directories("src")

set(SOURCES
  "src/audio.cpp"
  "src/bank.cpp"
  "src/bank_editor.cpp"
  "src/common.cpp"
  "src/controlls.cpp"
  "src/FileFormats/ffmt_base.cpp"
  "src/FileFormats/ffmt_factory.cpp"
  "src/FileFormats/format_deflemask_dmp.cpp"
  "src/FileFormats/format_tfi.cpp"
  "src/FileFormats/format_vgm_import.cpp"
  "src/FileFormats/format_wohlstand_opn2.cpp"
  "src/formats_sup.cpp"
  "src/importer.cpp"
  "src/latency.cpp"
  "src/ins_names.cpp"
  "src/main.cpp"
  "src/opl/generator.cpp"
  "src/opl/generator_realtime.cpp"
  "src/opl/realtime/ring_buffer.cpp"
  "src/piano.cpp"
  "src/opl/measurer.cpp"
  "src/opl/chips/gens_opn2.cpp"
  "src/opl/chips/gens/Ym2612_Emu.cpp"
  "src/opl/chips/nuked_opn2.cpp"
  "src/opl/chips/nuked/ym3438.c"
  "src/opl/chips/mame_opn2.cpp"
  "src/opl/chips/mame/mame_ym2612fm.c"
  "src/opl/chips/gx_opn2.cpp"
  "src/opl/chips/gx/gx_ym2612.c")

qt5_wrap_ui(FORMS
  "src/bank_editor.ui"
  "src/formats_sup.ui"
  "src/importer.ui")

qt5_add_resources(RESOURCES
  "src/resources/resources.qrc")

add_executable(OPN2BankEditor WIN32 MACOSX_BUNDLE ${SOURCES} ${FORMS} ${RESOURCES})
target_link_libraries(OPN2BankEditor PRIVATE Qt5::Widgets Qt5::Concurrent ${CMAKE_THREAD_LIBS_INIT})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(ICON "${CMAKE_SOURCE_DIR}/src/resources/res.rc")
  target_sources(OPN2BankEditor PRIVATE "${ICON}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(MACOSX_BUNDLE_ICON_FILE "opn2.icns")
  set(ICON "${CMAKE_SOURCE_DIR}/src/resources/opn2.icns")
  set_source_files_properties("${ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(OPN2BankEditor PRIVATE "${ICON}")
endif()

if(USE_RTMIDI)
  add_library(RtMidi STATIC "src/midi/external/rtmidi/RtMidi.cpp")
  target_include_directories(RtMidi PUBLIC "src/midi/external/rtmidi")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(RtMidi PUBLIC "__LINUX_ALSA__")
    target_link_libraries(RtMidi PUBLIC "asound")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(RtMidi PUBLIC "__WINDOWS_MM__")
    target_link_libraries(RtMidi PUBLIC "winmm")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(RtMidi PUBLIC "__MACOSX_CORE__")
    find_library(COREMIDI_LIBRARY "CoreMidi")
    target_link_libraries(RtMidi PUBLIC "${COREMIDI_LIBRARY}")
  endif()
  target_sources(OPN2BankEditor PRIVATE "src/midi/midi_rtmidi.cpp")
  target_compile_definitions(OPN2BankEditor PRIVATE "ENABLE_MIDI")
  target_link_libraries(OPN2BankEditor PRIVATE RtMidi)
endif()

if(USE_RTAUDIO)
  add_library(RtAudio STATIC "src/audio/external/rtaudio/RtAudio.cpp")
  target_include_directories(RtAudio PUBLIC "src/audio/external/rtaudio")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(RtAudio PUBLIC "__LINUX_ALSA__")
    target_link_libraries(RtAudio PUBLIC "asound")
    if(JACK_FOUND)
      target_compile_definitions(RtAudio PUBLIC "__UNIX_JACK__")
      target_include_directories(RtAudio PUBLIC ${JACK_INCLUDE_DIRS})
      link_directories(${JACK_LIBRARY_DIRS})
      target_link_libraries(RtAudio PUBLIC ${JACK_LIBRARIES})
    endif()
    if(PULSEAUDIO_FOUND)
      target_compile_definitions(RtAudio PUBLIC "__LINUX_PULSE__")
      target_include_directories(RtAudio PUBLIC ${PULSEAUDIO_INCLUDE_DIRS})
      link_directories(${PULSEAUDIO_LIBRARY_DIRS})
      target_link_libraries(RtAudio PUBLIC ${PULSEAUDIO_LIBRARIES})
    endif()
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(RtAudio PUBLIC "__WINDOWS_DS__")
    target_link_libraries(RtAudio PUBLIC "dsound" "ole32")
    target_compile_definitions(RtAudio PUBLIC "__WINDOWS_WASAPI__")
    target_link_libraries(RtAudio PUBLIC "ksguid")
    target_compile_definitions(RtAudio PUBLIC "__WINDOWS_ASIO__")
    target_include_directories(RtAudio PRIVATE
      "src/audio/external/rtaudio/include")
    target_sources(RtAudio PRIVATE
      "src/audio/external/rtaudio/include/asio.cpp"
      "src/audio/external/rtaudio/include/asiodrivers.cpp"
      "src/audio/external/rtaudio/include/asiolist.cpp"
      "src/audio/external/rtaudio/include/iasiothiscallresolver.cpp")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(RtAudio PUBLIC "__MACOSX_CORE__")
    find_library(COREAUDIO_LIBRARY "CoreAudio")
    target_link_libraries(RtAudio PUBLIC "${COREAUDIO_LIBRARY}")
  endif()
  target_sources(OPN2BankEditor PRIVATE "src/audio/ao_rtaudio.cpp")
  target_compile_definitions(OPN2BankEditor PRIVATE "ENABLE_AUDIO_TESTING")
  target_link_libraries(OPN2BankEditor PRIVATE RtAudio)
endif()
