/*
 * PIGURA OS - PERMUKAAN_BLIT.C
 * ==============================
 * Operasi blitting permukaan.
 */

#include "../../inti/kernel.h"

void permukaan_blit_copy(permukaan_t *src, permukaan_t *dst,
                          tak_bertanda32_t sx, tak_bertanda32_t sy,
                          tak_bertanda32_t dx, tak_bertanda32_t dy,
                          tak_bertanda32_t w, tak_bertanda32_t h)
{
    /* Placeholder - implementasi blit copy */
    (void)src; (void)dst; (void)sx; (void)sy; (void)dx; (void)dy; (void)w; (void)h;
}

void permukaan_blit_stretch(permukaan_t *src, permukaan_t *dst,
                             tak_bertanda32_t sx, tak_bertanda32_t sy,
                             tak_bertanda32_t sw, tak_bertanda32_t sh,
                             tak_bertanda32_t dx, tak_bertanda32_t dy,
                             tak_bertanda32_t dw, tak_bertanda32_t dh)
{
    /* Placeholder - implementasi stretch blit */
    (void)src; (void)dst; (void)sx; (void)sy; (void)sw; (void)sh;
    (void)dx; (void)dy; (void)dw; (void)dh;
}
