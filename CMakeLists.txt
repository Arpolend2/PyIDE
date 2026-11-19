cmake_minimum_required(VERSION 3.12)
project(ru.arpolend.PyIDE VERSION 0.1.0 LANGUAGES CXX)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "Install prefix" FORCE)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AURORA REQUIRED IMPORTED_TARGET auroraapp)
pkg_check_modules(RUNTIME_MANAGER REQUIRED IMPORTED_TARGET runtime-manager-qt5)

find_program(LRELEASE_EXECUTABLE NAMES lrelease NO_CMAKE_FIND_ROOT_PATH)
if(NOT LRELEASE_EXECUTABLE)
    message(FATAL_ERROR "Qt lrelease is required to build translations")
endif()
set(RUSSIAN_TRANSLATION "${CMAKE_CURRENT_BINARY_DIR}/translations/PythonRunner_ru.qm")
add_custom_command(
    OUTPUT "${RUSSIAN_TRANSLATION}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/translations"
    COMMAND "${LRELEASE_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/translations/PythonRunner_ru.ts"
            -qm "${RUSSIAN_TRANSLATION}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/translations/PythonRunner_ru.ts"
    VERBATIM)
add_custom_target(pythonrunner_translations ALL DEPENDS "${RUSSIAN_TRANSLATION}")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
    set(PYTHON_PACKAGE_ID "adb6fd20e2b7b7e15e878a697901cb37891abc6c/6905c0127a32d960c82b6ff306772617")
    set(PYTHON_SHA256 "f47f096bd4135b5b1154368b9541a040f8c264bb1f70f5ea30c994cb9d363f7d")
    set(SSL_PACKAGE_ID "916fde08d36eaa406b93dd4b10b8f305f6bbcad2/4fc7b6d9c49278a019d2ad61d53933f3")
    set(SSL_SHA256 "7edd250b385aebbbdd81441fe7be2e4917b6883543e453523db49e4278a893d0")
    set(FFI_PACKAGE_ID "6e7e36f851f18dc1323af3b92d24b61de13dfe46/861523621c608c69f6b0701d0c0e853d")
    set(FFI_SHA256 "ea120e35da51007280a43398e0b949c0a6d1aac9c3580678bafc24077229a091")
    set(DECIMAL_PACKAGE_ID "86e9f8888c432cd3c3c613174b92bba6b13610a0/28a0901d733e7a1289f31ec1ecac4a7c")
    set(DECIMAL_SHA256 "acbf0788b96a3b45841e88854ad92a62c0bb5023b1f1c09b7186c7cd811f1c5a")
    set(SQLITE_PACKAGE_ID "df4c6b4a6f27a9e77706206434511dfc1f67c70f/7f68399dba3bf70574159839b0d124dc")
    set(SQLITE_SHA256 "7bf849c3efe24f7b0081a51ae5be62b1f5bc18f0c4ebba9b2dbaeae52e1ad814")
    set(XZ_PACKAGE_ID "6e7e36f851f18dc1323af3b92d24b61de13dfe46/6d2d1d4d5be56b3386032de51aed3d9a")
    set(XZ_SHA256 "cfa546856e24876733b77bba50733f1338e0115a71a65b94b3536d7552a8bbad")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(armv7|arm)")
    set(PYTHON_PACKAGE_ID "9bc11542de1da86dc1b490bd28345fa602eab3ef/90a8be921744df815a98f6ca3e80f51c")
    set(PYTHON_SHA256 "0aeb5d229f6be208a4912fe3c20a3f4fefd79cc5208c0b6754689e777ea37a54")
    set(SSL_PACKAGE_ID "1b5715f41da350b5fb9024e066fa5f92b3d169bf/405079538eea4e05bc5f954cf3a69ef9")
    set(SSL_SHA256 "916cd87e988d0ec1be1ff2b3986d2f35750e5b9b728d85e6263947458a550f0")
    set(FFI_PACKAGE_ID "67800c3cdfc524bb830bacd9d730743560a64078/46f6435b79ed4a38219a4038f280e00e")
    set(FFI_SHA256 "3082d80d41d2b1c52ad98bc101c200548fbcd961c10b49dc75dc41fcc7abd2ac")
    set(DECIMAL_PACKAGE_ID "07929cd2e8919175fa20750de72890231fe657f8/d8a4c42f2fa4a474212348a43c1b1b57")
    set(DECIMAL_SHA256 "dd2976f55d2337532ae11ea80032c4c6aa093c8121ba6a309508709aa0dbd21b")
    set(SQLITE_PACKAGE_ID "17ec00c396a049526eb8f16028c0f2631e6ef8c9/4d8c7b0cd6986305f1bdc2ac135dbd72")
    set(SQLITE_SHA256 "8e0651cbc57d173b6a9913a3dbdd824ee12c735673cb78f53ba727abce3cbeb0")
    set(XZ_PACKAGE_ID "67800c3cdfc524bb830bacd9d730743560a64078/1b463df0542f5e6de703776da5927795")
    set(XZ_SHA256 "93b8ac5447ddef467dd5973fc2038d5f62a6bddb965f10fb43280d9125c73e26")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)")
    set(PYTHON_PACKAGE_ID "f841c177ec1afbf2f05a01b14e1bb445a960c39b/05c880f8bb5b81229b0d11f6820df8b1")
    set(PYTHON_SHA256 "5426700df98e0981a31dd30cb789c5baa2fb6a6e79414bc51ec5533faf8593c4")
    set(SSL_PACKAGE_ID "f5d4b553a386b1725d18169a11ae3a08a92e90bf/90c7c70c63101034df0a5d0e30430879")
    set(SSL_SHA256 "48390b01793e6425e38c7f171955131e3008969a26bf269d7d7144ff1f491e67")
    set(FFI_PACKAGE_ID "f940c1f7889f9018080dbeebdede5a67d6c3b30e/3e4c17c685f934b2fdf8b25d39f8b6f1")
    set(FFI_SHA256 "96c4e232baa1b4baac38e8c29606c9cbc2500aaf80e2a5fcde4aa84528a35543")
    set(DECIMAL_PACKAGE_ID "809552acd9e047bdafe3121671d702dd6854c0cf/c54b4d6f161dbf645262f169e9b1b3de")
    set(DECIMAL_SHA256 "96140d327bc09b244a64c2a7eade2a5187c5b668d994897d59eee7b11660447e")
    set(SQLITE_PACKAGE_ID "a4873eda09e896f69dc44f96c7c73055481f65ab/0e2dfb3e1010bf8f69ea7c4010d36898")
    set(SQLITE_SHA256 "2ae66f4e76b0e298ba7d76afaffff685e322ae72f9c664caa619e47eb0f4d43d")
    set(XZ_PACKAGE_ID "f940c1f7889f9018080dbeebdede5a67d6c3b30e/04f6bf07a428deb14de05371d76fb0ff")
    set(XZ_SHA256 "13af450f41469d768590aa9ab892e07fee26633f34452aa0827dce13fdb2809e")
else()
    message(FATAL_ERROR "Unsupported Python architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

function(fetch_runtime name url hash)
    set(archive "${CMAKE_BINARY_DIR}/${name}.tgz")
    if(NOT EXISTS "${archive}")
        file(DOWNLOAD "${url}" "${archive}" EXPECTED_HASH "SHA256=${hash}"
             TLS_VERIFY ON STATUS download_status)
        list(GET download_status 0 download_code)
        if(NOT download_code EQUAL 0)
            message(FATAL_ERROR "Failed to download ${name}: ${download_status}")
        endif()
    endif()
    file(SHA256 "${archive}" actual_hash)
    if(NOT actual_hash STREQUAL hash)
        message(FATAL_ERROR "Checksum mismatch for ${archive}")
    endif()
endfunction()

set(CONAN_BASE "https://conan.omp.ru/artifactory/public/aurora")
fetch_runtime(cpython "${CONAN_BASE}/cpython/3.11.9/_/6562ae1135aa674a8d485123b04e5787/package/${PYTHON_PACKAGE_ID}/conan_package.tgz" "${PYTHON_SHA256}")
fetch_runtime(openssl "${CONAN_BASE}/openssl/3.3.3/_/3e8665a701434e9726219392472dd9cf/package/${SSL_PACKAGE_ID}/conan_package.tgz" "${SSL_SHA256}")
fetch_runtime(libffi "${CONAN_BASE}/libffi/3.4.8/_/37de394dd69a35a2b58a5b47c76a1a59/package/${FFI_PACKAGE_ID}/conan_package.tgz" "${FFI_SHA256}")
fetch_runtime(mpdecimal "${CONAN_BASE}/mpdecimal/2.5.1/_/88f507ce21215e0557d71e64fda7a98e/package/${DECIMAL_PACKAGE_ID}/conan_package.tgz" "${DECIMAL_SHA256}")
fetch_runtime(sqlite "${CONAN_BASE}/sqlite3/3.51.0/_/d076260bbbdb3a1bc633bb79c1c5b0c7/package/${SQLITE_PACKAGE_ID}/conan_package.tgz" "${SQLITE_SHA256}")
fetch_runtime(xz "${CONAN_BASE}/xz_utils/5.8.1/_/a85abdbb088593b415d0b923adca8de4/package/${XZ_PACKAGE_ID}/conan_package.tgz" "${XZ_SHA256}")

function(fetch_wheel path hash)
    get_filename_component(name "${path}" NAME)
    set(archive "${CMAKE_BINARY_DIR}/wheels/${name}")
    if(NOT EXISTS "${archive}")
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/wheels")
        file(DOWNLOAD "https://files.pythonhosted.org/packages/${path}" "${archive}"
             EXPECTED_HASH "SHA256=${hash}" TLS_VERIFY ON STATUS download_status)
        list(GET download_status 0 download_code)
        if(NOT download_code EQUAL 0)
            message(FATAL_ERROR "Failed to download ${name}: ${download_status}")
        endif()
    endif()
    file(SHA256 "${archive}" actual_hash)
    if(NOT actual_hash STREQUAL hash)
        message(FATAL_ERROR "Checksum mismatch for ${archive}")
    endif()
    set(SCIENTIFIC_WHEELS ${SCIENTIFIC_WHEELS} "${archive}" PARENT_SCOPE)
endfunction()

include(tools/scientific_wheels.cmake)
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_runtime.py" PREPARE_SHA256)
file(SHA256 "${CMAKE_CURRENT_SOURCE_DIR}/tools/scientific_wheels.cmake" WHEELS_SHA256)
string(SHA256 RUNTIME_SHA256 "${PREPARE_SHA256}${WHEELS_SHA256}")
set(RUNTIME_DIR "${CMAKE_BINARY_DIR}/python-runtime-${CMAKE_SYSTEM_PROCESSOR}-${RUNTIME_SHA256}")
if(NOT EXISTS "${RUNTIME_DIR}/.complete")
    find_program(BUILD_PYTHON NAMES python3 NO_CMAKE_FIND_ROOT_PATH)
    if(NOT BUILD_PYTHON)
        message(FATAL_ERROR "Python 3 is required in the build environment to prepare the bundled runtime")
    endif()
    execute_process(COMMAND "${BUILD_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/prepare_runtime.py"
        "${CMAKE_BINARY_DIR}/cpython.tgz" "${CMAKE_BINARY_DIR}/openssl.tgz"
        "${CMAKE_BINARY_DIR}/libffi.tgz" "${CMAKE_BINARY_DIR}/mpdecimal.tgz"
        "${CMAKE_BINARY_DIR}/sqlite.tgz" "${CMAKE_BINARY_DIR}/xz.tgz" "${RUNTIME_DIR}"
        ${SCIENTIFIC_WHEELS}
        RESULT_VARIABLE prepare_result)
    if(NOT prepare_result EQUAL 0)
        message(FATAL_ERROR "Failed to prepare the bundled Python runtime")
    endif()
    file(WRITE "${RUNTIME_DIR}/.complete" "")
endif()

add_executable(${PROJECT_NAME} src/main.cpp src/runner.cpp src/runner.h
    src/editordocument.cpp src/editordocument.h src/pythonhighlighter.cpp src/pythonhighlighter.h
    src/textedithelper.cpp src/textedithelper.h src/fileopenreceiver.cpp src/fileopenreceiver.h
    src/newfile.cpp src/newfile.h resources.qrc)
add_dependencies(${PROJECT_NAME} pythonrunner_translations)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Gui Qt5::Qml Qt5::Quick PkgConfig::AURORA PkgConfig::RUNTIME_MANAGER)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
    PYTHON_EXECUTABLE_PATH="${CMAKE_INSTALL_PREFIX}/libexec/${PROJECT_NAME}/python3.11")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(PROGRAMS "${RUNTIME_DIR}/bin/python3.11"
    DESTINATION libexec/${PROJECT_NAME})
install(DIRECTORY "${RUNTIME_DIR}/lib/" DESTINATION share/${PROJECT_NAME}/lib)
install(DIRECTORY "${RUNTIME_DIR}/licenses/" DESTINATION share/${PROJECT_NAME}/licenses)
install(FILES python/aurora_plot_backend.py python/scientific_check.py DESTINATION share/${PROJECT_NAME}/python)
install(FILES "${RUSSIAN_TRANSLATION}" DESTINATION share/${PROJECT_NAME}/translations)
install(FILES ${PROJECT_NAME}.desktop DESTINATION share/applications)
install(FILES ${PROJECT_NAME}.svg DESTINATION share/icons/hicolor/scalable/apps)
foreach(size 86 108 128 172)
    install(FILES icons/${size}x${size}/apps/${PROJECT_NAME}.png
        DESTINATION share/icons/hicolor/${size}x${size}/apps)
endforeach()

add_custom_target(project_files SOURCES
    qml/Main.qml qml/EditorPage.qml qml/ResultsPage.qml translations/PythonRunner_ru.ts python/aurora_plot_backend.py python/scientific_check.py examples/plot.py tests/test_backend.py tests/test_runtime.py tests/test_elf_alignment.py tests/test_scientific_check.py tests/editor_tests.cpp tests/editor_tests.pro tools/prepare_runtime.py tools/check_elf_alignment.py tools/scientific_wheels.cmake README.md
    ${PROJECT_NAME}.desktop ${PROJECT_NAME}.svg rpm/${PROJECT_NAME}.spec)

file(WRITE "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
    "${CMAKE_INSTALL_PREFIX}\n"
    "${CMAKE_BINARY_DIR}/${PROJECT_NAME}:bin\n"
    "${RUNTIME_DIR}/bin/python3.11:libexec/${PROJECT_NAME}\n"
    "${CMAKE_SOURCE_DIR}/python/aurora_plot_backend.py:share/${PROJECT_NAME}/python\n"
    "${CMAKE_SOURCE_DIR}/python/scientific_check.py:share/${PROJECT_NAME}/python\n"
    "${RUSSIAN_TRANSLATION}:share/${PROJECT_NAME}/translations\n"
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.desktop:share/applications\n"
    "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}.svg:share/icons/hicolor/scalable/apps\n")
file(GLOB_RECURSE RUNTIME_FILES LIST_DIRECTORIES false
    "${RUNTIME_DIR}/lib/*" "${RUNTIME_DIR}/licenses/*")
foreach(runtime_file ${RUNTIME_FILES})
    file(RELATIVE_PATH relative_path "${RUNTIME_DIR}" "${runtime_file}")
    get_filename_component(relative_dir "${relative_path}" DIRECTORY)
    file(APPEND "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
        "${runtime_file}:share/${PROJECT_NAME}/${relative_dir}\n")
endforeach()
foreach(size 86 108 128 172)
    file(APPEND "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
        "${CMAKE_SOURCE_DIR}/icons/${size}x${size}/apps/${PROJECT_NAME}.png:share/icons/hicolor/${size}x${size}/apps\n")
endforeach()
