# Elderfier Service CMakeLists.txt
# Copyright (c) 2024 Fuego Developers

set(ELDERFIER_SERVICE_SOURCES
    ElderfierServiceModifierBuilder.cpp
    ElderfierServiceKernelBuilder.cpp
    ElderfierServiceProofGenerator.cpp
    ElderfierServiceProofVerifier.cpp
    ElderfierServiceUtils.cpp
)

set(ELDERFIER_SERVICE_HEADERS
    ../../include/ElderfierServiceTypesSimple.h
    ../../include/ElderfierServiceModifierBuilder.h
    ../../include/ElderfierServiceKernelBuilder.h
    ../../include/ElderfierServiceProofGenerator.h
    ../../include/ElderfierServiceProofVerifier.h
)

# Create Elderfier service library
add_library(ElderfierService STATIC ${ELDERFIER_SERVICE_SOURCES})

# Set include directories
target_include_directories(ElderfierService PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
)

# Link dependencies
target_link_libraries(ElderfierService
    CryptoNoteCore
    Logging
    Common
)

# Set compile definitions
target_compile_definitions(ElderfierService PRIVATE
    ELDERFIER_SERVICE_MODIFIER_INTERVAL=1000
    ELDERFIER_SERVICE_PROOF_WINDOW=3600
    ELDERFIER_SERVICE_PROOF_STEP=300
)

# Set C++ standard
set_target_properties(ElderfierService PROPERTIES
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED ON
)

# Install rules
install(TARGETS ElderfierService
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES ${ELDERFIER_SERVICE_HEADERS}
    DESTINATION include/ElderfierService
)
