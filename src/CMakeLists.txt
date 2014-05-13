SET(NAME efdl)

ADD_EXECUTABLE(
  ${NAME}

  main.cpp
  )

QT5_USE_MODULES(${NAME} Core Network)
