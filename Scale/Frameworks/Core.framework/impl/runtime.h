#ifndef __SCL_REFLECTION_API_H__
#define __SCL_REFLECTION_API_H__

#include <scale_internal.h>

struct Array;

sclDefFunc(Runtime_hasFramework, scl_int);
sclDefFunc(Runtime_listFrameworks, void);
sclDefFunc(Runtime_getFunctionID, scl_int);
sclDefFunc(Runtime_callByID, void);
sclDefFunc(Runtime_objectsByType, struct Array*);

#endif
