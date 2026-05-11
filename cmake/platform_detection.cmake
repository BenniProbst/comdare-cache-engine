# cmake/platform_detection.cmake
# REV 5.3 Phase 6 - Plattform-Erkennung fuer Multi-OS-Build
# Erkennt OS, Architektur, Block-AO-Plattform, Cache-Line-Size, ISA-Features

# -----------------------------------------------------------------------------
# OS-Erkennung
# -----------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(COMDARE_OS "Windows")
    set(COMDARE_OS_WINDOWS ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(COMDARE_OS "Linux")
    set(COMDARE_OS_LINUX ON)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(COMDARE_OS "Darwin")
    set(COMDARE_OS_MACOS ON)
else()
    message(WARNING "Unbekanntes Betriebssystem: ${CMAKE_SYSTEM_NAME} - generische Defaults")
    set(COMDARE_OS "Other")
endif()

# -----------------------------------------------------------------------------
# Architektur-Erkennung
# -----------------------------------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    set(COMDARE_ARCH "x86_64")
    set(COMDARE_ARCH_X86_64 ON)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(COMDARE_ARCH "arm64")
    set(COMDARE_ARCH_ARM64 ON)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^riscv64$")
    set(COMDARE_ARCH "riscv64")
    set(COMDARE_ARCH_RISCV64 ON)
else()
    set(COMDARE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    message(WARNING "Unbekannte Architektur: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# -----------------------------------------------------------------------------
# Block AO Plattform-Identifikation (best effort)
# Hinweis: dies ist eine GROBE Identifikation - exakte Modell-Erkennung
# erfolgt zur Laufzeit via CPUID (siehe cache_engine/platform_probe/cpuid_probe).
# -----------------------------------------------------------------------------
set(COMDARE_BLOCK_AO_PLATFORM "unknown")

if(COMDARE_OS_WINDOWS AND COMDARE_ARCH_X86_64)
    # Auf Windows: WMI-Abfrage des CPU-Namens (best-effort, kann fehlschlagen)
    execute_process(
        COMMAND powershell -NoProfile -Command
            "(Get-CimInstance -ClassName Win32_Processor).Name.Trim()"
        OUTPUT_VARIABLE _cpu_name
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        TIMEOUT 5)

    if(_cpu_name MATCHES "i7-1270P")
        set(COMDARE_BLOCK_AO_PLATFORM "i7-1270P")
    elseif(_cpu_name MATCHES "i9-14900KS")
        set(COMDARE_BLOCK_AO_PLATFORM "i9-14900KS")
    elseif(_cpu_name MATCHES "9950X3D")
        set(COMDARE_BLOCK_AO_PLATFORM "Ryzen-9-9950X3D")
    elseif(_cpu_name MATCHES "Xeon.*8470")
        set(COMDARE_BLOCK_AO_PLATFORM "Sapphire-Rapids-8470")
    endif()
    set(COMDARE_CPU_NAME "${_cpu_name}")
elseif(COMDARE_OS_LINUX AND EXISTS "/proc/cpuinfo")
    file(READ "/proc/cpuinfo" _cpuinfo LIMIT 4096)
    if(_cpuinfo MATCHES "i7-1270P")
        set(COMDARE_BLOCK_AO_PLATFORM "i7-1270P")
    elseif(_cpuinfo MATCHES "Xeon.*8470")
        set(COMDARE_BLOCK_AO_PLATFORM "Sapphire-Rapids-8470")
    elseif(_cpuinfo MATCHES "Cortex-A76")
        set(COMDARE_BLOCK_AO_PLATFORM "RaspberryPi5")
    elseif(_cpuinfo MATCHES "Neoverse")
        set(COMDARE_BLOCK_AO_PLATFORM "Grace-Hopper-GH200")
    elseif(_cpuinfo MATCHES "JH7110|StarFive")
        set(COMDARE_BLOCK_AO_PLATFORM "VisionFive2")
    endif()
elseif(COMDARE_OS_MACOS)
    execute_process(
        COMMAND sysctl -n machdep.cpu.brand_string
        OUTPUT_VARIABLE _cpu_name
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        TIMEOUT 5)
    if(_cpu_name MATCHES "Apple M1")
        set(COMDARE_BLOCK_AO_PLATFORM "Mac-M1")
    elseif(_cpu_name MATCHES "Apple M2")
        set(COMDARE_BLOCK_AO_PLATFORM "Mac-M2")
    elseif(_cpu_name MATCHES "Apple M3")
        set(COMDARE_BLOCK_AO_PLATFORM "Mac-M3")
    elseif(_cpu_name MATCHES "Apple M4")
        set(COMDARE_BLOCK_AO_PLATFORM "Mac-M4")
    endif()
    set(COMDARE_CPU_NAME "${_cpu_name}")
endif()

# -----------------------------------------------------------------------------
# Cache-Line-Size (KRITISCH! NIE hardcoded 64 B)
# -----------------------------------------------------------------------------
if(COMDARE_ARCH_X86_64)
    set(COMDARE_CACHE_LINE_SIZE 64)
elseif(COMDARE_ARCH_ARM64)
    if(COMDARE_OS_MACOS)
        # Apple Silicon: P-Cores haben 128 B
        set(COMDARE_CACHE_LINE_SIZE 128)
    else()
        # Generisch ARM Cortex-A: 64 B; A64FX hat 256 B (Fugaku)
        set(COMDARE_CACHE_LINE_SIZE 64)
    endif()
elseif(COMDARE_ARCH_RISCV64)
    set(COMDARE_CACHE_LINE_SIZE 64)
else()
    set(COMDARE_CACHE_LINE_SIZE 64)
    message(WARNING "Unbekannte Architektur - Cache-Line-Size auf 64 B Default; Runtime-Probe via std::hardware_destructive_interference_size empfohlen")
endif()
