ADD_LIBRARY(
  ${LIB_NAME}
  ${LIBRARY_TYPE}

  ../../include/Range.h
  Range.cpp

  ../../include/Util.h
  Util.cpp

  ../../include/Downloader.h
  Downloader.cpp

  ../../include/DownloadTask.h
  DownloadTask.cpp

  ../../include/CommitThread.h
  CommitThread.cpp
  )

QT5_USE_MODULES(${LIB_NAME} Core Network)