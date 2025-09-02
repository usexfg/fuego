set(BURN_DEPOSIT_VALIDATION_SERVICE_SOURCES
    BurnDepositValidationService.cpp
)

add_library(BurnDepositValidationService STATIC ${BURN_DEPOSIT_VALIDATION_SERVICE_SOURCES})

target_include_directories(BurnDepositValidationService PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(BurnDepositValidationService
    CryptoNoteCore
    Common
    Crypto
    Logging
)
