# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/ESP32/esp-idf/components/bootloader/subproject"
  "C:/ESP32/Ejemplos/UART_NMEA0183/nmea0183_parser/build/bootloader"
  "C:/ESP32/Ejemplos/UART_NMEA0183/nmea0183_parser/build/bootloader-prefix"
  "C:/ESP32/Ejemplos/UART_NMEA0183/nmea0183_parser/build/bootloader-prefix/tmp"
  "C:/ESP32/Ejemplos/UART_NMEA0183/nmea0183_parser/build/bootloader-prefix/src/bootloader-stamp"
  "C:/ESP32/Ejemplos/UART_NMEA0183/nmea0183_parser/build/bootloader-prefix/src"
  "C:/ESP32/Ejemplos/UART_NMEA0183/nmea0183_parser/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/ESP32/Ejemplos/UART_NMEA0183/nmea0183_parser/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/ESP32/Ejemplos/UART_NMEA0183/nmea0183_parser/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
