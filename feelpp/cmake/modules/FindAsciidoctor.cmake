###  CMakeLists.txt; coding: utf-8 --- 

#  Author(s): Christophe Prud'homme <christophe.prudhomme@feelpp.org>
#       Date: 28 Feb 2017
#
#  Copyright (C) 2017 Feel++ Consortium
#
# Distributed under the GPL(GNU Public License):
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#

include (FindPackageHandleStandardArgs)

find_program( ASCIIDOCTOR_EXECUTABLE
  NAMES asciidoctor
  PATHS
  $ENV{ASCIIDOCTOR_DIR}/bin
  PATH_SUFFIXES bin
  DOC "Asciidoctor"
  NO_DEFAULT_PATH
  )
find_program( ASCIIDOCTOR_PDF_EXECUTABLE
  NAMES asciidoctor-pdf
  PATHS
  $ENV{ASCIIDOCTOR_DIR}/bin
  PATH_SUFFIXES bin
  DOC "Asciidoctor PDF"
  NO_DEFAULT_PATH
  )
if(NOT ASCIIDOCTOR_EXECUTABLE)
find_program( ASCIIDOCTOR_EXECUTABLE
  NAMES asciidoctor
  PATHS
  $ENV{ASCIIDOCTOR_DIR}/bin
  PATH_SUFFIXES bin
  DOC "Asciidoctor" )
endif()
if(NOT ASCIIDOCTOR_PDF_EXECUTABLE)
  find_program( ASCIIDOCTOR_PDF_EXECUTABLE
    NAMES asciidoctor-pdf
    PATHS
    $ENV{ASCIIDOCTOR_DIR}/bin
    PATH_SUFFIXES bin
    DOC "Asciidoctor PDF" )
endif()


FIND_PACKAGE_HANDLE_STANDARD_ARGS (ASCIIDOCTOR DEFAULT_MSG ASCIIDOCTOR_EXECUTABLE ASCIIDOCTOR_PDF_EXECUTABLE)

if ( ASCIIDOCTOR_FOUND )
  MESSAGE( STATUS "Asciidoctor found: executable(${ASCIIDOCTOR_EXECUTABLE})" )
  MESSAGE( STATUS "Asciidoctor PDF found: executable(${ASCIIDOCTOR_PDF_EXECUTABLE})" )
endif()
mark_as_advanced( ASCIIDOCTOR_EXECUTABLE ASCIIDOCTOR_PDF_EXECUTABLE)
  

