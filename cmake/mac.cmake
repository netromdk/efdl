SET(SDK_MIN "10.8")
SET(SDK "10.8")
SET(DEV_SDK "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${SDK}.sdk")

ADD_DEFINITIONS(
  -DMAC
  -DGCC_VISIBILITY
  -mmacosx-version-min=${SDK_MIN}
  )

SET(CMAKE_OSX_SYSROOT ${DEV_SDK})
