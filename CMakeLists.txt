cmake_minimum_required (VERSION 3.0)
project (a1menu)

#set (a1menu_VERSION_MAJOR 0)
#set (a1menu_VERSION_MINOR 0)
#set (a1menu_VERSION_PATCH 0)

#extracting a version from the source code

file(STRINGS ${CMAKE_CURRENT_BINARY_DIR}/src/application.cpp v)
string(REPLACE " " ""   v ${v})
string(REPLACE "\"" "~" v ${v})
string(REPLACE "." "~"  v ${v})
string(REPLACE "~" ";"  v ${v})

foreach(s in ${v})
   string(FIND "${s}" ":A1MENU_VERSION=" pos)
   if(${pos} GREATER 0)
        set(v0123 0)
   endif()
   if(DEFINED v0123)
    #message(STATUS "v=" ${v0123})
    if(${v0123} EQUAL 1)
        set(a1menu_VERSION_MAJOR ${s})
    endif()
    if(${v0123} EQUAL 2)
        set(a1menu_VERSION_MINOR ${s})
    endif() 
    if(${v0123} EQUAL 3)
        set(a1menu_VERSION_PATCH ${s})
        break()
    endif()
    MATH(EXPR v0123 "${v0123}+1")
   endif()
endforeach()    


if(DEFINED a1menu_VERSION_MAJOR AND DEFINED a1menu_VERSION_MINOR AND DEFINED a1menu_VERSION_PATCH)
   set(verstr ${a1menu_VERSION_MAJOR}.${a1menu_VERSION_MINOR}.${a1menu_VERSION_PATCH})
   message( STATUS "Version number from the source code: " ${verstr})
else()
   message( FATAL_ERROR "Cannot extract a version number from the source code (src/applications.cpp)")
endif()   




# comment this, if you want a GTK+2 version
set (GTK_VERSION_3 TRUE)


include(GNUInstallDirs)

FILE(GLOB srclist src/*.cpp )
ADD_EXECUTABLE(a1menu ${srclist})

set_target_properties(a1menu PROPERTIES OUTPUT_NAME "a1menu-gtk.run")

add_definitions("-O3 -Wall -c -fmessage-length=0 -std=c++11")

find_package(PkgConfig REQUIRED)
find_package(LibXml2 REQUIRED)
find_package (Threads REQUIRED)
if(GTK_VERSION_3)
  pkg_check_modules(GTK REQUIRED gtk+-3.0)
  add_definitions("-DGTK3")
  set (GTK_STR gtk3)
else()
  pkg_check_modules(GTK REQUIRED gtk+-2.0)
  pkg_check_modules(GDK REQUIRED gdk-2.0)
  set (GTK_STR gtk2)
endif()  

pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(MATE REQUIRED libmatepanelapplet-4.0)

find_program(MSGEXE msgfmt)
if(${MSGEXE} STREQUAL "MSGEXE-NOTFOUND")
    message(FATAL_ERROR "Translation tool msgfmt not found!")
endif()




include_directories(${LIBXML2_INCLUDE_DIR})
include_directories(${GTK_INCLUDE_DIRS})
include_directories(${GDK_INCLUDE_DIRS})
include_directories(${X11_INCLUDE_DIRS})
include_directories(${MATE_INCLUDE_DIRS})


set( CMAKE_EXE_LINKER_FLAGS "-std=c++11")

target_link_libraries(a1menu 
    ${GTK_LIBRARIES} 
    ${GDK_LIBRARIES} 
    ${X11_LIBRARIES} 
    ${MATE_LIBRARIES} 
    ${LIBXML2_LIBRARIES} 
    ${CMAKE_THREAD_LIBS_INIT} 
    Xi )
    
set (TMPDIR ${CMAKE_CURRENT_BINARY_DIR}/localtmp)
file(MAKE_DIRECTORY ${TMPDIR})
    
function(setdir dirvar dirname)  
    set(${dirvar} ${dirname} PARENT_SCOPE)
    #message(STATUS "set install directory " ${dirvar} " to " ${dirname})
    if(NOT EXISTS ${dirname})
        message(FATAL_ERROR "\n*** directory " ${dirname} " NOT FOUND ***\n")
    endif()  
endfunction()

setdir(DIR_INST_BIN          /usr/bin)
setdir(DIR_INST_TRANSLATIONS /usr/share/locale)
setdir(DIR_INST_SERVICES     /usr/share/dbus-1/services)
setdir(DIR_INST_FACTORY      /usr/share/mate-panel/applets)

#define_property(GLOBAL PROPERTY RPM_IGNORE_LIST BRIEF_DOCS "x" FULL_DOCS "x")
#set_property(GLOBAL PROPERTY RPM_IGNORE_LIST "/usr/bin")
SET(F_IGNORE_RPM  /usr/bin)


function(IGNORE_RPM_PATH dirname)  
    while(TRUE)
        string(LENGTH ${dirname} slen)
        if(${slen} LESS 2)
            break()
        endif()
        LIST(APPEND F_IGNORE_RPM  "${dirname}")
        LIST(REMOVE_DUPLICATES F_IGNORE_RPM)
        SET(F_IGNORE_RPM ${F_IGNORE_RPM} PARENT_SCOPE)
        #message(STATUS " list " ${F_IGNORE_RPM})
        get_filename_component(dirname ${dirname} DIRECTORY) 
    endwhile()
endfunction()


    


FILE(GLOB POLIST src/po/*.po )

foreach (POFILE ${POLIST})
    get_filename_component(LANG ${POFILE} NAME_WE)
    #message(STATUS "po file: " ${POFILE}  " language: " ${LANG})
    if (EXISTS ${POFILE})
        set (MOFILE ${TMPDIR}/${LANG}.mo)
        message(STATUS "creating translation " ${MOFILE})
        execute_process ( COMMAND ${MSGEXE} -o ${MOFILE} ${POFILE} )	  
        install (FILES ${MOFILE} DESTINATION ${DIR_INST_TRANSLATIONS}/${LANG}/LC_MESSAGES RENAME a1menu-gtk.mo)
        IGNORE_RPM_PATH(${DIR_INST_TRANSLATIONS}/${LANG}/LC_MESSAGES)
    endif()
endforeach()
    
    

file(WRITE ${TMPDIR}/factory
"[Applet Factory]
Id=A1MenuGtkFactory
Location=${DIR_INST_BIN}/a1menu-gtk.run
Name=A1MenuGtkFactory Factory
Description=A1MenuGtkFactory Factory

[A1MenuGtk]
Name=a1menu-gtk
Description=Searchable menu for MATE Desktop
Icon=terminal"
)

file(WRITE ${TMPDIR}/service
"[D-BUS Service]
Name=org.mate.panel.applet.A1MenuGtkFactory
Exec=${DIR_INST_BIN}/a1menu-gtk.run"
)

set(DEFAULT_PERMISSIONS
    OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)


install (FILES ${TMPDIR}/factory DESTINATION ${DIR_INST_FACTORY}   RENAME org.mate.applets.a1menu-gtk.mate-panel-applet)
install (FILES ${TMPDIR}/service DESTINATION ${DIR_INST_SERVICES}  RENAME org.mate.panel.applet.A1MenuGtkFactory.service)
install (FILES "a1menu-gtk.run"  DESTINATION ${DIR_INST_BIN}       RENAME a1menu-gtk.run PERMISSIONS  ${DEFAULT_PERMISSIONS})


EXECUTE_PROCESS( COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE ARCHITECTURE )


set(packgen "DEB")
find_program(RPMBUILD rpmbuild)
message(STATUS "rpmbuild tool? " ${RPMBUILD})
if(NOT (${RPMBUILD} STREQUAL "RPMBUILD-NOTFOUND"))
  list(APPEND packgen "RPM")
endif()


SET(CPACK_GENERATOR ${packgen})
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "appdevsw@wp.pl") #required
SET(CPACK_PACKAGE_CONTACT "appdevsw@wp.pl")
SET(CPACK_PACKAGE_VERSION ${a1menu_VERSION_MAJOR}.${a1menu_VERSION_MINOR}.${a1menu_VERSION_PATCH})
SET(CPACK_PACKAGE_VERSION_MAJOR ${a1menu_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${a1menu_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${a1menu_VERSION_PATCH})
SET(CPACK_PACKAGE_FILE_NAME a1menu-${CPACK_PACKAGE_VERSION}-${GTK_STR}-${ARCHITECTURE})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A menu for MATE desktop")
SET(CPACK_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION_SUMMARY})
SET(CPACK_DEB_DEFAULT_FILE_PERMISSIONS ${DEFAULT_PERMISSIONS})
SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

#set(CPACK_SOURCE_GENERATOR "RPM")
#SET(CPACK_SOURCE_RPM ON)
#set(CPACK_RPM_PACKAGE_SOURCES "ON")

IGNORE_RPM_PATH( ${DIR_INST_FACTORY} )
IGNORE_RPM_PATH( ${DIR_INST_SERVICES} )

SET(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION ${F_IGNORE_RPM})


INCLUDE(CPack)


#generate source RPM

option(srpm "srpm" OFF)
message(STATUS "srpm option: " ${srpm})
if(srpm)
   set(buildroot /tmp/a1menu-srpm)
   execute_process ( COMMAND rm -r ${buildroot})
   file(MAKE_DIRECTORY ${buildroot}/SPECS)
   file(MAKE_DIRECTORY ${buildroot}/SOURCES)
   set(srctmp ${buildroot}/work/a1menu-${verstr})
   file(MAKE_DIRECTORY ${srctmp})
   
   #preparing spec file
   file(READ ${CMAKE_CURRENT_BINARY_DIR}/src/install/a1menu.spec vspec)
   string(REPLACE "versionundefined" ${verstr} vspec ${vspec})
   file(WRITE ${buildroot}/SPECS/a1menu.spec ${vspec})
   
   
   #file(COPY ${CMAKE_CURRENT_BINARY_DIR}/src/install/a1menu.spec DESTINATION ${buildroot}/SPECS)
   
   execute_process (COMMAND cp -r ${CMAKE_CURRENT_BINARY_DIR}/src ${srctmp})
   execute_process (COMMAND cp -r ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt ${srctmp})
   
   set(xcmd "tar  -czhf ${buildroot}/SOURCES/a1menu-${verstr}.tar.gz -C ${buildroot}/work ./a1menu-${verstr}/")
   message(STATUS ${xcmd})
   execute_process (COMMAND bash -c "${xcmd}")
			     
   execute_process ( COMMAND rpmbuild 
			     --define "_topdir ${buildroot}" 
			     -ba ${buildroot}/SPECS/a1menu.spec)
endif()
