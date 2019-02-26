#ifndef DRE_MASTER_LOADER_GLOBAL_H
#define DRE_MASTER_LOADER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DRE_MASTER_LOADER_LIBRARY)
#  define DRE_MASTER_LOADERSHARED_EXPORT Q_DECL_EXPORT
#else
#  define DRE_MASTER_LOADERSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // DRE_MASTER_LOADER_GLOBAL_H
