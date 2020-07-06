# Copyright (c) 2020 ARM Limited. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

# TODO: @mbed-os-tools All these should be autogenerated, so this can be removed later
set(CMAKE_SYSTEM_NAME         Generic)
set(CMAKE_CROSSCOMPILING      TRUE)

# TODO: @mbed tools to pass the processor type
set(CMAKE_SYSTEM_PROCESSOR    cortex-m4)

set(CMAKE_C_COMPILER_WORKS    TRUE)
set(CMAKE_CXX_COMPILER_WORKS  TRUE)


set(CMAKE_ASM_COMPILER    "armasm")
set(CMAKE_C_COMPILER      "armclang")
set(CMAKE_CXX_COMPILER    "armclang")
set(CMAKE_AR              "armar")
set(ELF2BIN               "fromelf")

# TODO: @mbed-os-tools get flags from mbed-os/tools/profiles/,
#       mbed-os/tools/toolchains/arm.py, and target config in mbed-os/targets/targets.json
set(CMAKE_C_FLAGS
    "--target=arm-arm-none-eabi -mthumb -g -O1 -Wno-armcc-pragma-push-pop -Wno-armcc-pragma-anon-unions -Wno-reserved-user-defined-literal -Wno-deprecated-register -DMULADDC_CANNOT_USE_R7 -fdata-sections -fno-exceptions -MMD -fshort-enums -fshort-wchar -DMBED_DEBUG -DMBED_TRAP_ERRORS_ENABLED=1 -D__ASSERT_MSG -std=gnu11 -mfpu=none -mcpu=cortex-m4 -DMBED_ROM_START=0x0 -DMBED_ROM_SIZE=0x100000 -DMBED_RAM1_START=0x1fff0000 -DMBED_RAM1_SIZE=0x10000 -DMBED_RAM_START=0x20000000 -DMBED_RAM_SIZE=0x30000 -include mbed_config.h"
)
set(CMAKE_CXX_FLAGS
    "--target=arm-arm-none-eabi -mthumb -g -O1 -Wno-armcc-pragma-push-pop -Wno-armcc-pragma-anon-unions -Wno-reserved-user-defined-literal -Wno-deprecated-register -DMULADDC_CANNOT_USE_R7 -fdata-sections -fno-exceptions -MMD -fshort-enums -fshort-wchar -DMBED_DEBUG -DMBED_TRAP_ERRORS_ENABLED=1 -fno-rtti -fno-c++-static-destructors -std=gnu++14 -mfpu=none -mcpu=cortex-m4 -DMBED_ROM_START=0x0 -DMBED_ROM_SIZE=0x100000 -DMBED_RAM1_START=0x1fff0000 -DMBED_RAM1_SIZE=0x10000 -DMBED_RAM_START=0x20000000 -DMBED_RAM_SIZE=0x30000  -include mbed_config.h"
)
set(CMAKE_ASM_FLAGS
    "--cpu=Cortex-M4 --cpreproc --cpreproc_opts=--target=arm-arm-none-eabi,-D,__FPU_PRESENT"
)
set(CMAKE_CXX_LINK_FLAGS
    "--verbose --remove --show_full_path --legacyalign --any_contingency --keep=os_cb_sections --cpu=Cortex-M4 --predefine=-DMBED_ROM_START=0x0 --predefine=-DMBED_ROM_SIZE=0x100000 --predefine=-DMBED_RAM1_START=0x1fff0000 --predefine=-DMBED_RAM1_SIZE=0x10000 --predefine=-DMBED_RAM_START=0x20000000 --predefine=-DMBED_RAM_SIZE=0x30000 --predefine=-DMBED_BOOT_STACK_SIZE=1024 --predefine=-DXIP_ENABLE=0"
)
