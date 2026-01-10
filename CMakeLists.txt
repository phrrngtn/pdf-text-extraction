cmake_minimum_required (VERSION 3.15)
project(TextExtraction VERSION 1.1.9)

# options
option(USE_BIDI  "should support bi-directional text")
option(SHOULD_PARSE_INTERNAL_TABLES  "should table parsing read internal tables")

# Dependencies
include(FetchContent)

# PDFHummus
FetchContent_Declare(
  PDFHummus
  URL https://github.com/galkahana/PDF-Writer/archive/refs/tags/v4.8.1.tar.gz
  URL_HASH SHA256=9b96c040cb04ca116450c0ed53462b6aa66b299c555401bf498b14db847f0898
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
  FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(PDFHummus)

# nlohmann/json for JSON output
FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  URL_HASH SHA256=d6c65aca6b1ed68e7a182f4757257b107ae403032760ed6ef121c9d55e81757d
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
  FIND_PACKAGE_ARGS NAMES nlohmann_json
)
FetchContent_MakeAvailable(json)

# local code
ADD_SUBDIRECTORY(TextExtraction)
ADD_SUBDIRECTORY(TextExtractionCLI)

if(PROJECT_IS_TOP_LEVEL AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/TextExtractionTesting)
    # avoid installing the testing materials altogether when included in another project.
    # it's annoying when in parent all, and more annoying to then get the tests added
    # to the parent project ctest.
    enable_testing()
    ADD_SUBDIRECTORY(TextExtractionTesting)
endif()

# cpack
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${TextExtraction_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${TextExtraction_VERSION_MINOR}")
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_GENERATOR "ZIP")
include(CPack)

# package install
install(EXPORT TextExtractionTargets
  FILE TextExtractionTargets.cmake
  DESTINATION lib/cmake/TextExtraction
  NAMESPACE TextExtraction::
)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/TextExtractionConfig.cmake"
  INSTALL_DESTINATION "lib/cmake/TextExtraction"
  )

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TextExtractionConfigVersion.cmake"
    VERSION "${TextExtraction_VERSION_MAJOR}.${TextExtraction_VERSION_MINOR}"
    COMPATIBILITY AnyNewerVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/TextExtractionConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/TextExtractionConfigVersion.cmake
  DESTINATION lib/cmake/TextExtraction
  )

export(EXPORT TextExtractionTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/TextExtractionTargets.cmake"
  NAMESPACE TextExtraction::
)