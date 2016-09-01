fceux_SOURCES += utils/backward.cpp utils/crc32.cpp utils/endian.cpp utils/general.cpp utils/guid.cpp utils/md5.cpp utils/memory.cpp utils/xstring.cpp
if ! SYSTEM_MINIZIP
ZIP_SOURCES = utils/ioapi.cpp utils/unzip.cpp
endif
fceux_SOURCES += $(ZIP_SOURCES)
