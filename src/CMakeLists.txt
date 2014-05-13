SET(NAME efdl)

ADD_EXECUTABLE(
  ${NAME}

  main.cpp
  Version.h
  )

QT5_USE_MODULES(${NAME} Core Network)
