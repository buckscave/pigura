/* ----------------------------------------------------------------------------------------------
 * pigura/versi.h - Metadata versi
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_VERSI_H
#define PIGURA_VERSI_H

#define PIGURA_VERSI_MAJOR 0
#define PIGURA_VERSI_MINOR 39
#define PIGURA_VERSI_PATCH 1
#define PIGURA_VERSI_STRING "0.39.1"

#define PIGURA_VERSI_MINIMAL(maj,min) \
        ((PIGURA_VERSI_MAJOR > (maj)) || \
         (PIGURA_VERSI_MAJOR == (maj) && PIGURA_VERSI_MINOR >= (min)))

#endif /* PIGURA_VERSI_H */
