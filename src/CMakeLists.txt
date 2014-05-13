SET(NAME efdl)

ADD_EXECUTABLE(
  ${NAME}

  main.cpp
  Version.h

  Downloader.h
  Downloader.cpp
  )

QT5_USE_MODULES(${NAME} Core Network)
