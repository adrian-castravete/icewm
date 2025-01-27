/*
 * IceWM
 *
 * Copyright (C) 2004,2005 Marko Macek
 */
#include "config.h"

#include "mstring.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <regex.h>
#include "base.h"
#include "ascii.h"

MStringData *MStringData::alloc(size_t length) {
    size_t size = sizeof(MStringData) + length + 1;
    MStringData *ud = static_cast<MStringData *>(malloc(size));
    ud->fRefCount = 0;
    return ud;
}

MStringData *MStringData::create(const char *str, size_t length) {
    MStringData *ud = MStringData::alloc(length);
    strncpy(ud->fStr, str, length);
    ud->fStr[length] = 0;
    return ud;
}

MStringData *MStringData::create(const char *str) {
    return create(str, strlen(str));
}

mstring::mstring(MStringData *fStr, size_t fOffset, size_t fCount):
    fStr(fStr),
    fOffset(fOffset),
    fCount(fCount)
{
    if (fStr) acquire();
}

void mstring::init(const char *str, size_t len) {
    if (str) {
        fStr = MStringData::create(str, len);
        fOffset = 0;
        fCount = len;
        acquire();
    } else {
        fStr = nullptr;
        fOffset = 0;
        fCount = 0;
    }
}

mstring::mstring(const char *str) {
    init(str, str ? strlen(str) : 0);
}

mstring::mstring(const char *str, size_t len) {
    init(str, len);
}

mstring::mstring(const char *str1, const char *str2) {
    size_t len1 = str1 ? strlen(str1) : 0;
    size_t len2 = str2 ? strlen(str2) : 0;
    if (len1) {
        init(str1, len1 + len2);
        if (len2)
            strlcpy(fStr->fStr + len1, str2, len2 + 1);
    } else {
        init(str2, len2);
    }
}

mstring::mstring(const char *str1, const char *str2, const char *str3) {
    size_t len1 = str1 ? strlen(str1) : 0;
    size_t len2 = str2 ? strlen(str2) : 0;
    size_t len3 = str3 ? strlen(str3) : 0;
    fOffset = 0;
    fCount = len1 + len2 + len3;
    fStr = MStringData::alloc(fCount);
    memcpy(fStr->fStr, str1, len1);
    memcpy(fStr->fStr + len1, str2, len2);
    memcpy(fStr->fStr + len1 + len2, str3, len3);
    fStr->fStr[fCount] = 0;
    acquire();
}

mstring::mstring(long n) {
    char num[24];
    snprintf(num, sizeof num, "%ld", n);
    init(num, strlen(num));
}

void mstring::destroy() {
    free(fStr); fStr = nullptr;
}

mstring::~mstring() {
    if (fStr)
        release();
}

mstring mstring::operator+(const mstring& rv) const {
    if (!length()) return rv;
    if (!rv.length()) return *this;
    size_t newCount = length() + rv.length();
    MStringData *ud = MStringData::alloc(newCount);
    memcpy(ud->fStr, data(), length());
    memcpy(ud->fStr + length(), rv.data(), rv.length());
    ud->fStr[newCount] = 0;
    return mstring(ud, 0, newCount);
}

mstring& mstring::operator+=(const mstring& rv) {
    return *this = *this + rv;
}

mstring& mstring::operator=(const mstring& rv) {
    if (fStr != rv.fStr) {
        if (fStr) release();
        fStr = rv.fStr;
        if (fStr) acquire();
    }
    fOffset = rv.fOffset;
    fCount = rv.fCount;
    return *this;
}

mstring& mstring::operator=(null_ref &) {
    if (fStr) {
        release();
        fStr = nullptr;
        fCount = 0;
        fOffset = 0;
    }
    return *this;
}

mstring mstring::substring(size_t pos) const {
    PRECONDITION(pos <= length());
    return mstring(fStr, fOffset + pos, fCount - pos);
}

mstring mstring::substring(size_t pos, size_t len) const {
    PRECONDITION(pos <= length());

    return mstring(fStr, fOffset + pos, min(len, fCount - pos));
}

bool mstring::split(unsigned char token, mstring *left, mstring *remain) const {
    PRECONDITION(token < 128);
    int splitAt = indexOf(char(token));
    if (splitAt >= 0) {
        size_t i = size_t(splitAt);
        mstring l = substring(0, i);
        mstring r = substring(i + 1, length() - i - 1);
        *left = l;
        *remain = r;
        return true;
    }
    return false;
}

bool mstring::splitall(unsigned char token, mstring *left, mstring *remain) const {
    if (split(token, left, remain))
        return true;

    if (nonempty()) {
        *left = *this;
        *remain = null;
        return true;
    }
    return false;
}

int mstring::charAt(int pos) const {
    if (pos >= 0 && pos < int(length()))
        return data()[pos];
    else
        return -1;
}

bool mstring::startsWith(const mstring &s) const {
    if (length() < s.length())
        return false;
    if (s.isEmpty())
        return true;
    if (memcmp(data(), s.data(), s.length()) == 0)
        return true;
    return false;
}

bool mstring::endsWith(const mstring &s) const {
    if (length() < s.length())
        return false;
    if (s.isEmpty())
        return true;
    if (memcmp(data() + length() - s.length(), s.data(), s.length()) == 0)
        return true;
    return false;
}

int mstring::find(const mstring &s) const {
    const int slen = int(s.length());
    const int stop = int(length()) - slen;
    for (int start = 0; start <= stop; ++start) {
        for (int i = 0; ; ++i) {
            if (i == slen)
                return start;
            if (data()[i + start] != s.data()[i])
                break;
        }
    }
    return -1;
}

int mstring::indexOf(char ch) const {
    if (isEmpty())
        return -1;
    char *s = static_cast<char *>(
                const_cast<void *>(memchr(data(), ch, fCount)));
    if (s == nullptr)
        return -1;
    return int(s - data());
}

int mstring::lastIndexOf(char ch) const {
    for (int k = int(length()) - 1; k >= 0; --k) {
        if (ch == data()[k]) return k;
    }
    return -1;
}

int mstring::count(char ch) const {
    unsigned n = 0;
    for (size_t k = 0, len = length(); k < len; ++k) {
        n += ch == data()[k];
    }
    return int(n);
}

bool mstring::equals(const char *s) const {
    return equals(s, s ? strlen(s) : 0);
}

bool mstring::equals(const char *s, size_t len) const {
    return len == length() && 0 == memcmp(s, data(), len);
}

bool mstring::equals(const mstring &s) const {
    return compareTo(s) == 0;
}

int mstring::collate(const mstring &s, bool ignoreCase) const {
    if (!ignoreCase)
        return strcoll(cstring(*this), cstring(s));
    else
        return strcoll(cstring(this->lower()), cstring(s.lower()));
}

int mstring::compareTo(const mstring &s) const {
#ifdef upstream_comp
    if (s.length() > length()) {
        return -1;
    } else if (s.length() == length()) {
        if (isEmpty())
            return 0;
        return memcmp(s.data(), data(), fCount);
    } else {
        return 1;
    }
#else
    size_t minlen = min(s.length(), length());
    if (minlen > 0) {
        int res = memcmp(data(), s.data(), minlen);
        if (res)
           return res;
    }
    return int(length()) - int(s.length());
#endif
}

bool mstring::copyTo(char *dst, size_t len) const {
    if (len > 0 && fCount > 0) {
        size_t minlen = min(len - 1, size_t(fCount));
        memcpy(dst, data(), minlen);
        dst[minlen] = 0;
    }
    return size_t(fCount) < len;
}

mstring mstring::replace(int position, int len, const mstring &insert) const {
    PRECONDITION(position >= 0);
    PRECONDITION(len >= 0);
    PRECONDITION(position + len <= int(length()));
    return substring(0, size_t(position)) + insert
        + substring(size_t(position + len), length() - size_t(position + len));
}

mstring mstring::remove(int position, int len) const {
    return replace(position, len, mstring(null));
}

mstring mstring::insert(int position, const mstring &s) const {
    return replace(position, 0, s);
}

mstring mstring::append(const mstring &s) const {
    return *this + s;
}

mstring mstring::searchAndReplaceAll(const mstring& s, const mstring& r) const {
    mstring modified(*this);
    const int step = int(1 + r.length() - s.length());
    for (int offset = 0; size_t(offset) + s.length() <= modified.length(); ) {
        int found = offset + modified.substring(size_t(offset)).find(s);
        if (found < offset) {
            break;
        }
        modified = modified.replace(found, int(s.length()), r);
        offset = max(0, offset + step);
    }
    return modified;
}

mstring mstring::lower() const {
    if (fStr) {
        MStringData *ud = MStringData::create(data(), fCount);
        for (unsigned i = 0; i < fCount; ++i) {
            ud->fStr[i] = ASCII::toLower(ud->fStr[i]);
        }
        return mstring(ud, 0, fCount);
    }
    return null;
}

mstring mstring::upper() const {
    if (*this == null) {
        return null;
    } else {
        MStringData *ud = MStringData::create(data(), fCount);
        for (unsigned i = 0; i < fCount; ++i) {
            ud->fStr[i] = ASCII::toUpper(ud->fStr[i]);
        }
        return mstring(ud, 0, fCount);
    }
}

mstring mstring::trim() const {
    int k = 0, n = int(length());
    while (k < n && isspace(static_cast<unsigned char>(charAt(k)))) {
        ++k;
    }
    while (k < n && isspace(static_cast<unsigned char>(charAt(n - 1)))) {
        --n;
    }
    return substring(size_t(k), size_t(n - k));
}

void mstring::normalize()
{
    if (fStr) {
        if (data()[fCount]) {
            MStringData *ud = MStringData::create(data(), fCount);
            release();
            fStr = ud;
            fOffset = 0;
            acquire();
        }
    }
}

mstring mstring::match(const char* regex, const char* flags) const {
    int compFlags = REG_EXTENDED;
    int execFlags = 0;
    for (int i = 0; flags && flags[i]; ++i) {
        switch (flags[i]) {
            case 'i': compFlags |= REG_ICASE; break;
            case 'n': compFlags |= REG_NEWLINE; break;
            case 'B': execFlags |= REG_NOTBOL; break;
            case 'E': execFlags |= REG_NOTEOL; break;
        }
    }

    regex_t preg;
    int comp = regcomp(&preg, regex, compFlags);
    if (comp) {
        if (testOnce(regex, __LINE__)) {
            char rbuf[123] = "";
            regerror(comp, &preg, rbuf, sizeof rbuf);
            warn("match regcomp: %s", rbuf);
        }
        return null;
    }

    regmatch_t pos;
    int exec = regexec(&preg, data(), 1, &pos, execFlags);

    regfree(&preg);

    if (exec)
        return null;

    return mstring(data() + pos.rm_so, size_t(pos.rm_eo - pos.rm_so));
}

cstring::cstring(const mstring &s): str(s) {
    str.normalize();
}

cstring& cstring::operator=(const cstring& cs) {
    str = cs.str;
    return *this;
}


// vim: set sw=4 ts=4 et:
