idf_component_register(
    SRCS "sljit_src/sljitLir.c"
         "jit_cache.c"
         "jit_decode.c"
         "jit_translate.c"
         "jit_execute.c"
    INCLUDE_DIRS "." "sljit_src"
    REQUIRES esp_timer gbsp-libretro
)
