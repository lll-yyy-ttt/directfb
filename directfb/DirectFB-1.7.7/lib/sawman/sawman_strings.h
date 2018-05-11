#ifndef __SAWMAN__SAWMAN_STRINGS_H__
#define __SAWMAN__SAWMAN_STRINGS_H__
#include <sawman.h>


struct DFBSaWManProcessFlagsName {
     SaWManProcessFlags flag;
     const char *name;
};

#define DirectFBSaWManProcessFlagsNames(Identifier) struct DFBSaWManProcessFlagsName Identifier[] = { \
     { SWMPF_MASTER, "MASTER" }, \
     { SWMPF_MANAGER, "MANAGER" }, \
     { SWMPF_EXITING, "EXITING" }, \
     { (SaWManProcessFlags) SWMPF_NONE, "NONE" } \
};

#endif
