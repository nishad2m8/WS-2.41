SET(SOURCES screens/ui_Screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_1015890679.c
    images/ui_img_1565510499.c
    images/ui_img_1819326991.c
    images/ui_img_images_lamp_png.c
    images/ui_img_images_lux_png.c
    images/ui_img_403521860.c
    images/ui_img_310970492.c
    images/ui_img_images_ac_png.c
    images/ui_img_330396823.c
    images/ui_img_1844661879.c
    images/ui_img_images_vacuum_cleaner_png.c
    images/ui_img_580765743.c
    fonts/ui_font_KR20.c
    fonts/ui_font_KR25.c
    fonts/ui_font_KR40.c)

add_library(ui ${SOURCES})
