SET(BIN_NAME efdl)
SET(LIB_NAME efdlcore)

ADD_LIBRARY(
  ${LIB_NAME}

  Util.h
  Util.cpp

  Downloader.h
  Downloader.cpp

  DownloadTask.h
  DownloadTask.cpp

  CommitThread.h
  CommitThread.cpp
  )

QT5_USE_MODULES(${LIB_NAME} Core Network)

ADD_EXECUTABLE(
  ${BIN_NAME}

  main.cpp
  Version.h
  )

QT5_USE_MODULES(${BIN_NAME} Core)
TARGET_LINK_LIBRARIES(${BIN_NAME} ${LIB_NAME})

INSTALL(TARGETS ${BIN_NAME} DESTINATION bin)
