/* ----------------------------------------------------------------------------------------------
 * pigura/versi.h - Metadata versi
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_VERSI_H
#define PIGURA_VERSI_H

#define PIGURA_VERSI_MAJOR 0
#define PIGURA_VERSI_MINOR 2
#define PIGURA_VERSI_PATCH 0
#define PIGURA_VERSI_STRING "0.2.0"

#define PIGURA_VERSI_MINIMAL(maj,min) \
        ((PIGURA_VERSI_MAJOR > (maj)) || \
         (PIGURA_VERSI_MAJOR == (maj) && PIGURA_VERSI_MINOR >= (min)))

#endif /* PIGURA_VERSI_H */
