set(SOURCES types.cc
            utils.cc)

set(HEADERS channel.h
            config_utils.h
            exceptions.h
            types.h
            utils.h)


add_library(utilities OBJECT ${SOURCES})
aktualizr_source_file_checks(${SOURCES} ${HEADERS})

