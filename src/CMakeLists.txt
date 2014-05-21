SET(BIN_NAME efdl)
SET(LIB_NAME efdlcore)

INCLUDE_DIRECTORIES( ../include )

ADD_LIBRARY(
  ${LIB_NAME}
  ${LIBRARY_TYPE}

  ../include/Util.h
  Util.cpp

  ../include/Downloader.h
  Downloader.cpp

  ../include/DownloadTask.h
  DownloadTask.cpp

  ../include/CommitThread.h
  CommitThread.cpp
  )

QT5_USE_MODULES(${LIB_NAME} Core Network)

ADD_EXECUTABLE(
  ${BIN_NAME}

  main.cpp
  ../include/Version.h

  DownloadManager.h
  DownloadManager.cpp
  )

QT5_USE_MODULES(${BIN_NAME} Core)
TARGET_LINK_LIBRARIES(${BIN_NAME} ${LIB_NAME})

INSTALL(TARGETS ${BIN_NAME} DESTINATION bin)
