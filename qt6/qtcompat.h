/*
 * Copyright (C) 2026 Wang Bin - wbsecg1 at gmail.com
 * https://github.com/wang-bin/qtmultimedia-plugins-mdk
 * MIT License
 *
 * Shims for QPlatformMediaIntegration::create*() return types across Qt 6 minors.
 */
#pragma once

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
#  define MDK_MEDIA_RESULT(T) T *
#  define MDK_MEDIA_OK(ptr) (ptr)
#  define MDK_MEDIA_FAIL(msg) nullptr
#elif QT_VERSION < QT_VERSION_CHECK(6, 10, 0)
#  include <QtMultimedia/private/qmaybe_p.h>
#  define MDK_MEDIA_RESULT(T) QMaybe<T *>
#  define MDK_MEDIA_OK(ptr) (ptr)
#  define MDK_MEDIA_FAIL(msg) QString(msg)
#else
#  include <QtCore/private/qexpected_p.h>
#  define MDK_MEDIA_RESULT(T) q23::expected<T *, QString>
#  define MDK_MEDIA_OK(ptr) (ptr)
#  define MDK_MEDIA_FAIL(msg) q23::unexpected{ QString(msg) }
#endif
