/*
 * IceWM
 *
 * Copyright (C) 1999-2001 Marko Macek
 *
 * MessageBox
 */
#include "config.h"

#include "ylib.h"
#include "ymsgbox.h"

#include "WinMgr.h"
#include "yapp.h"
#include "yxapp.h"
#include "wmframe.h"
#include "sysdep.h"
#include "yprefs.h"
#include "prefs.h"

#include "intl.h"

YMsgBox::YMsgBox(int buttons, YWindow *owner): YDialog(owner) {
    fListener = 0;
    fButtonOK = 0;
    fButtonCancel = 0;
    fLabel = new YLabel(null, this);
    fLabel->show();

    setToplevel(true);

    if (buttons & mbOK) {
        fButtonOK = new YActionButton(this);
        if (fButtonOK) {

            fButtonOK->setText(_("_OK"), -2);
            fButtonOK->setActionListener(this);
            fButtonOK->show();
        }
    }
    if (buttons & mbCancel) {
        fButtonCancel = new YActionButton(this);
        if (fButtonCancel) {
            fButtonCancel->setText(_("_Cancel"), -2);
            fButtonCancel->setActionListener(this);
            fButtonCancel->show();
        }
    }
    autoSize();
    setWinLayerHint(WinLayerAboveDock);
    setWinWorkspaceHint(-1);
    setWinHintsHint(WinHintsSkipWindowMenu);
    {

        Atom protocols[2];
        protocols[0] = _XA_WM_DELETE_WINDOW;
        protocols[1] = _XA_WM_TAKE_FOCUS;
        XSetWMProtocols(xapp->display(), handle(), protocols, 2);
        getProtocols(true);
    }
    setMwmHints(MwmHints(
       MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS,
       MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
       MWM_DECOR_BORDER | MWM_DECOR_TITLE | MWM_DECOR_MENU,
       0,
       0));
}

YMsgBox::~YMsgBox() {
    delete fLabel; fLabel = 0;
    delete fButtonOK; fButtonOK = 0;
    delete fButtonCancel; fButtonCancel = 0;
}

void YMsgBox::autoSize() {
    unsigned lw = fLabel ? fLabel->width() : 0;
    unsigned w = lw + 24, h;

    w = clamp(w, 240U, desktop->width());

    h = 12;
    if (fLabel) {
        fLabel->setPosition((w - lw) / 2, h);
        h += fLabel->height();
    }
    h += 18;

    unsigned const hh(max(fButtonOK ? fButtonOK->height() : 0,
                          fButtonCancel ? fButtonCancel->height() : 0));
    unsigned const ww(max(fButtonOK ? fButtonOK->width() : 0,
                          fButtonCancel ? fButtonCancel->width() : 3));

    if (fButtonOK) {
        fButtonOK->setSize(ww, hh);
        fButtonOK->setPosition((w - hh)/2 - fButtonOK->width(), h);
    }
    if (fButtonCancel) {
        fButtonCancel->setSize(ww, hh);
        fButtonCancel->setPosition((w + hh)/2, h);
    }

    h += fButtonOK ? fButtonOK->height() :
        fButtonCancel ? fButtonCancel->height() : 0;
    h += 12;

    setSize(w, h);
}

void YMsgBox::setTitle(const ustring &title) {
    cstring cs(title);
    setWindowTitle(cs.c_str());
    autoSize();
}

void YMsgBox::setText(const ustring &text) {
    if (fLabel)
        fLabel->setText(text);
    autoSize();
}

void YMsgBox::setPixmap(ref<YPixmap>/*pixmap*/) {
}

void YMsgBox::actionPerformed(YAction action, unsigned int /*modifiers*/) {
    if (fListener) {
        if (fButtonOK && action == *fButtonOK) {
            fListener->handleMsgBox(this, mbOK);
        }
        else if (fButtonCancel && action == *fButtonCancel) {
            fListener->handleMsgBox(this, mbCancel);
        }
        else {
            TLOG(("unknown action %d for msgbox", action.ident()));
        }
    }
}

void YMsgBox::handleClose() {
    if (fListener)
        fListener->handleMsgBox(this, 0);
    else {
        manager->unmanageClient(this);
        manager->focusTopWindow();
    }
}

void YMsgBox::handleFocus(const XFocusChangeEvent &/*focus*/) {
}

void YMsgBox::showFocused() {
    switch (msgBoxDefaultAction) {
    case 0:
        if (fButtonCancel) fButtonCancel->requestFocus(false);
        break;
    case 1:
        if (fButtonOK) fButtonOK->requestFocus(false);
        break;
    }
    if (getFrame() == 0)
        manager->manageClient(handle(), false);
    if (getFrame()) {
        int dx, dy;
        unsigned dw, dh;
        desktop->getScreenGeometry(&dx, &dy, &dw, &dh);
        getFrame()->setNormalPositionOuter(
            dx + dw / 2 - getFrame()->width() / 2,
            dy + dh / 2 - getFrame()->height() / 2);
        getFrame()->activateWindow(true);
    }
}

// vim: set sw=4 ts=4 et:
