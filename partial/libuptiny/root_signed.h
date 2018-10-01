#ifndef LIBUPTINY_ROOT_SIGNED_H
#define LIBUPTINY_ROOT_SIGNED_H

#include "state_api.h"

#ifdef __cplusplus
extern "C" {
#endif

bool uptane_parse_root_signed(const char *metadata_str, int16_t *pos, uptane_root_t *out_root);

#ifdef __cplusplus
}
#endif
#endif  // LIBUPTINY_ROOT_SIGNED_H
