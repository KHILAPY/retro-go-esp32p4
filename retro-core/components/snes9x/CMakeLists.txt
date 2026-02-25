set(COMPONENT_SRCDIRS "src")
set(COMPONENT_ADD_INCLUDEDIRS "." "src")
set(COMPONENT_REQUIRES "retro-go")
set(COMPONENT_PRIV_REQUIRES "esp_driver_ppa")
register_component()
rg_setup_compile_options(
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-array-bounds
    -Wno-implicit-fallthrough
    -DRIGHTSHIFT_IS_SAR
    -DFAST_LSB_WORD_ACCESS
    -DNO_ZERO_LUT
    -O3
    -ffunction-sections
    -fdata-sections
    -fomit-frame-pointer
)
