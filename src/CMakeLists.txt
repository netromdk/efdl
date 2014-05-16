SET(NAME efdl)

ADD_EXECUTABLE(
  ${NAME}

  main.cpp
  Version.h

  Util.h
  Util.cpp

  Downloader.h
  Downloader.cpp

  DownloadTask.h
  DownloadTask.cpp
  )

QT5_USE_MODULES(${NAME} Core Network)
