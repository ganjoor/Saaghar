/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <QApplication>
#include <QVariant>
#include <QMainWindow>
#include <QFont>
#include <QFontMetrics>
#include <QPixmap>
#include <QPainter>
#include <QLabel>

#if QT_VERSION >= 0x050000
#    include <QGuiApplication>
#    include <QWindow>
#    include <qpa/qplatformnativeinterface.h>
#endif

#include <stylehelper.h>

#include "progressmanager_p.h"

// for windows progress bar
#ifndef __GNUC__
#    include <shobjidl.h>
#endif

// Windows 7 SDK required
#ifdef __ITaskbarList3_INTERFACE_DEFINED__

namespace {
    int total = 0;
    ITaskbarList3* pITask = 0;
}

QT_BEGIN_NAMESPACE
Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &p);
QT_END_NAMESPACE

#if QT_VERSION >= 0x050000

HICON qt_pixmapToWinHICON(const QPixmap &p);

static inline QWindow *windowOfWidget(const QWidget *widget)
{
    if (QWindow *window = widget->windowHandle())
        return window;
    if (QWidget *topLevel = widget->nativeParentWidget())
        return topLevel->windowHandle();
    return 0;
}

static inline HWND hwndOfWidget(const QWidget *w)
{
    void *result = 0;
    if (QWindow *window = windowOfWidget(w))
        result = QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", window);
    return static_cast<HWND>(result);
}
#else
static inline HWND hwndOfWidget(const QWidget *w)
{
    return w->winId();
}
#endif

void Core::Internal::ProgressManagerPrivate::initInternal()
{
    CoInitialize(NULL);
    HRESULT hRes = CoCreateInstance(CLSID_TaskbarList,
                                    NULL,CLSCTX_INPROC_SERVER,
                                    IID_ITaskbarList3,(LPVOID*) &pITask);
     if (FAILED(hRes))
     {
         pITask = 0;
         CoUninitialize();
         return;
     }

     pITask->HrInit();
     return;
}

void Core::Internal::ProgressManagerPrivate::cleanup()
{
    if (pITask) {
    pITask->Release();
    pITask = NULL;
    CoUninitialize();
    }
}

void Core::Internal::ProgressManagerPrivate::doSetApplicationLabel(const QString &text)
{
    if (!pITask)
        return;

    const HWND winId = hwndOfWidget(qApp->activeWindow());
    if (text.isEmpty()) {
        pITask->SetOverlayIcon(winId, NULL, NULL);
    } else {
        QPixmap pix(QLatin1String(":/core/images/compile_error_taskbar.png"));
#if QT_VERSION >= 0x050000
        pix.setDevicePixelRatio(1); // We want device-pixel sized font depending on the pix.height
#endif
        QPainter p(&pix);
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPixelSize(pix.height() * 0.5);
        p.setFont(font);
        p.drawText(pix.rect(), Qt::AlignCenter, text);
#if QT_VERSION >= 0x050000
        const HICON icon = qt_pixmapToWinHICON(pix);
#else
        const HICON icon = pix.toWinHICON();
#endif
        pITask->SetOverlayIcon(winId, icon, (wchar_t*)text.utf16());
        DestroyIcon(icon);
    }
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressRange(int min, int max)
{
    total = max-min;
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressValue(int value)
{
    if (pITask) {
        const HWND winId = hwndOfWidget(qApp->activeWindow());
        pITask->SetProgressValue(winId, value, total);
    }
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressVisible(bool visible)
{
    if (!pITask)
        return;

    const HWND winId = hwndOfWidget(qApp->activeWindow());
    if (visible)
        pITask->SetProgressState(winId, TBPF_NORMAL);
    else
        pITask->SetProgressState(winId, TBPF_NOPROGRESS);
}

#else

void Core::Internal::ProgressManagerPrivate::initInternal()
{
}

void Core::Internal::ProgressManagerPrivate::cleanup()
{
}

void Core::Internal::ProgressManagerPrivate::doSetApplicationLabel(const QString &text)
{
    Q_UNUSED(text)
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressRange(int min, int max)
{
    Q_UNUSED(min)
    Q_UNUSED(max)
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressValue(int value)
{
    Q_UNUSED(value)
}

void Core::Internal::ProgressManagerPrivate::setApplicationProgressVisible(bool visible)
{
    Q_UNUSED(visible)
}


#endif // __ITaskbarList2_INTERFACE_DEFINED__
