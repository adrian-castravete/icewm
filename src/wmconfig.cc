/*
 * IceWM
 *
 * Copyright (C) 1997-2002 Marko Macek
 */
#include "config.h"
#include "ykey.h"
#include "yprefs.h"

#define CFGDEF
#include "wmconfig.h"
#include "bindkey.h"
#include "default.h"

#include "wmoption.h"
#include "ymenu.h"
#include "wmmgr.h"
#include "yaction.h"
#include "yapp.h"
#include "sysdep.h"

#include "intl.h"

YStringArray configWorkspaces;

void WMConfig::loadConfiguration(IApp *app, const char *fileName) {
    YConfig::findLoadConfigFile(app, icewm_preferences, fileName);
    YConfig::findLoadConfigFile(app, icewm_themable_preferences, fileName);
}

void WMConfig::loadThemeConfiguration(IApp *app, const char *themeName) {
    bool ok = YConfig::findLoadThemeFile(app,
                icewm_themable_preferences,
                *themeName == '/' ? themeName :
                upath("themes").child(themeName));
    if (ok == false)
        fail(_("Failed to load theme %s"), themeName);
}

void WMConfig::freeConfiguration() {
    YConfig::freeConfig(icewm_preferences);
    YConfig::freeConfig(icewm_themable_preferences);
}

void addWorkspace(const char *, const char *value, bool append) {
    if (!append) {
        configWorkspaces.clear();
    }
    configWorkspaces += value;
}

static const struct {
    const char name[8];
    enum WMLook value;
} wmLookNames[] = {
    { "win95",  lookWin95  },
    { "motif",  lookMotif  },
    { "warp3",  lookWarp3  },
    { "warp4",  lookWarp4  },
    { "nice",   lookNice   },
    { "pixmap", lookPixmap },
    { "metal",  lookMetal  },
    { "gtk",    lookGtk    },
    { "flat",   lookFlat   },
};

const char* getLookName(enum WMLook look) {
    for (size_t k = 0; k < ACOUNT(wmLookNames); ++k) {
        if (look == wmLookNames[k].value)
            return wmLookNames[k].name;
    }
    return "";
}

bool getLook(const char *name, const char *arg, enum WMLook *lookPtr) {
    for (size_t k = 0; k < ACOUNT(wmLookNames); ++k) {
        if (0 == strcmp(arg, wmLookNames[k].name)) {
            if (lookPtr) {
                *lookPtr = wmLookNames[k].value;
            }
            return true;
        }
    }
    return false;
}

void setLook(const char *name, const char *arg, bool) {
    enum WMLook look = wmLook;
    if (getLook(name, arg, &look)) {
        wmLook = look;
    } else {
        msg(_("Unknown value '%s' for option '%s'."), arg, name);
    }
}

static bool ensureDirectory(const upath& path) {
    if (path.dirExists())
        return true;
    if (path.mkdir() != 0) {
        fail(_("Unable to create directory %s"), path.string().c_str());
    }
    return path.dirExists();
}

static upath getDefaultsFilePath(const pstring& basename) {
    upath prv(YApplication::getPrivConfDir());
    if (ensureDirectory(prv)) {
        return prv + basename;
    }
    return null;
}

void WMConfig::setDefault(const char *basename, cstring content) {
    upath confOld(getDefaultsFilePath(basename));
    if (confOld == null) {
        return; // no directory
    }
    upath confNew(confOld.path() + ".new.tmp");

    FILE *fpNew = confNew.fopen("w");
    if (fpNew) {
        fputs(content, fpNew);
        if (content[0] && content[strlen(content)-1] != '\n')
            fputc('\n', fpNew);
    }
    if (fpNew == nullptr || fflush(fpNew) || ferror(fpNew)) {
        fail(_("Unable to write to %s"), confNew.string().c_str());
        if (fpNew)
            fclose(fpNew);
        confNew.remove();
        return;
    }

    FILE *fpOld = confOld.fopen("r");
    if (fpOld) {
        for (int i = 0; i < 10; ++i) {
            char buf[600] = "#", *line = buf;
            if (fgets(1 + buf, sizeof buf - 1, fpOld)) {
                while (line[1] == '#')
                    ++line;
                fputs(line, fpNew);
                if ('\n' != line[strlen(line)-1])
                    fputc('\n', fpNew);
            }
        }
        fclose(fpOld);
    }
    fclose(fpNew);

    if (fpOld != nullptr || confOld.access() == 0) {
        confOld.remove();
    }
    if (confNew.renameAs(confOld)) {
        fail(_("Unable to rename %s to %s"),
                confNew.string().c_str(), confOld.string().c_str());
        confNew.remove();
    }
}

void WMConfig::setDefaultFocus(long focusMode) {
    setDefault("focus_mode", "FocusMode=\"" + mstring(focusMode) + "\"");
}

void WMConfig::setDefaultTheme(mstring themeName) {
    setDefault("theme", "Theme=\"" + themeName + "\"");
}

static void print_options(cfoption *options) {
    for (int i = 0; options[i].type != cfoption::CF_NONE; ++i) {
        switch (options[i].type) {
        case cfoption::CF_BOOL:
            printf("%s=%d\n", options[i].name, options[i].boolval());
            break;
        case cfoption::CF_INT:
            printf("%s=%d\n", options[i].name, *options[i].v.i.int_value);
            break;
        case cfoption::CF_UINT:
            printf("%s=%u\n", options[i].name, *options[i].v.u.uint_value);
            break;
        case cfoption::CF_STR:
            printf("%s=\"%s\"\n", options[i].name, Elvis(options[i].str(), ""));
            break;
        case cfoption::CF_KEY:
            printf("%s=\"%s\"\n", options[i].name, options[i].key()->name);
            break;
        case cfoption::CF_FUNC:
            if (0 == strcmp("Look", options[i].name)) {
                printf("%s=%s\n", "Look", getLookName(wmLook));
            }
            break;
        case cfoption::CF_NONE:
            break;
        }
    }
}

void WMConfig::printPrefs(long focus, bool log, bool sync, const char* spl) {
    printf("FocusMode=%ld\n", focus);
    printf("LoggingEvents=%d\n", log);
    printf("SynchronizeX11=%d\n", sync);
    printf("SplashFile=\"%s\"\n", spl);
    print_options(icewm_preferences);
    print_options(icewm_themable_preferences);
}


// vim: set sw=4 ts=4 et:
