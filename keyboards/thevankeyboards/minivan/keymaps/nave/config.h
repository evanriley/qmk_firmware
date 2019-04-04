#ifndef CONFIG_USER_H
#define CONFIG_USER_H

#include "../../config.h"

// place overrides here

#undef PERMISSIVE_HOLD
#undef IGNORE_MOD_TAP_INTERRUPT

#undef RETRO_TAPPING
#define RETRO_TAPPING

#ifdef TAPPING_TERM
#undef TAPPING_TERM
#endif // TAPPING_TERM
#define TAPPING_TERM 120




#endif
