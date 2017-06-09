/*
 * IceWM
 *
 * Copyright (C) 1998-2001 Marko Macek
 */

#include "config.h"

#ifndef NO_CONFIGURE_MENUS

#include "obj.h"
#include "objmenu.h"
#include "browse.h"
#include "wmmgr.h"
#include "wmprog.h"
#include "yicon.h"
#include "sysdep.h"
#include "base.h"
#include "udir.h"

BrowseMenu::BrowseMenu(
    IApp *app,
    YSMListener *smActionListener,
    YActionListener *wmActionListener,
    upath path,
    YWindow *parent): ObjectMenu(wmActionListener, parent)
{
    this->app = app;
    this->smActionListener = smActionListener;
    fPath = path;
    fModTime = 0;
}

BrowseMenu::~BrowseMenu() {
}

static int compare_ustring_pointers(const void *p1, const void *p2)
{
    ustring *u1 = * (ustring * const *) p1;
    ustring *u2 = * (ustring * const *) p2;
    return u1->compareTo(*u2);
}

void BrowseMenu::updatePopup() {
    struct stat sb;

    if (fPath.stat(&sb) != 0)
        removeAll();
    else if (sb.st_mtime > fModTime) {
        fModTime = sb.st_mtime;

        removeAll();

        YObjectArray<ustring> dirList;
        for (udir dir(fPath); dir.next(); ) {
            dirList.append(new ustring(dir.entry()));
        }
        ustring **begin = dirList.getItemPtr(0);
        const int count = dirList.getCount();
        qsort(begin, count, sizeof(*begin), compare_ustring_pointers);
        for (int index = 0; index < count; ++index) {
            const ustring& entry(*begin[index]);
            upath npath(fPath + entry);

            YMenu *sub = 0;
            if (npath.dirExists())
                sub = new BrowseMenu(app, smActionListener, wmActionListener, npath);

            DFile *pfile = new DFile(app, entry, null, npath);
            YMenuItem *item = add(new DObjectMenuItem(pfile));
            if (item) {
#ifndef LITE
                static ref<YIcon> file, folder;
                if (file == null)
                    file = YIcon::getIcon("file");
                if (folder == null)
                    folder = YIcon::getIcon("folder");
#endif
                item->setSubmenu(sub);
#ifndef LITE
                if (sub) {
                    if (folder != null)
                        item->setIcon(folder);
                } else {
                    if (file != null)
                        item->setIcon(file);
                }
#endif
            }
        }
    }
}
#endif
