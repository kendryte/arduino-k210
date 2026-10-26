// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "Stream.h"
#include <memory>
#include <stdint.h>
#include <time.h>
#define FILE_READ   "r"
#define FILE_WRITE  "w"
#define FILE_APPEND "a"

namespace fs {
using boolean = bool;
enum SeekMode { SeekSet = 0, SeekCur = 1, SeekEnd = 2 };
class FileImpl;
class FSImpl;
using FileImplPtr = std::shared_ptr<FileImpl>;
using FSImplPtr   = std::shared_ptr<FSImpl>;
class File : public Stream {
public:
    File(FileImplPtr implementation = FileImplPtr());
    using Print::write;
    size_t      write(uint8_t value) override;
    size_t      write(const uint8_t* data, size_t count) override;
    int         read() override;
    size_t      read(uint8_t* output, size_t count);
    size_t      readBytes(char* output, size_t count);
    int         available() override;
    int         peek() override;
    void        flush() override;
    bool        seek(uint32_t position, SeekMode mode = SeekSet);
    size_t      position() const;
    size_t      size() const;
    bool        setBufferSize(size_t count);
    void        close();
                operator bool() const;
    time_t      getLastWrite();
    const char* path() const;
    const char* name() const;
    bool        isDirectory();
    File        openNextFile(const char* mode = FILE_READ);
    void        rewindDirectory();

protected:
    FileImplPtr _p;
};
class FS {
public:
    explicit FS(FSImplPtr implementation)
        : _impl(implementation)
    {
    }
    File open(const char* path, const char* mode = FILE_READ, bool create = false);
    File open(const String& path, const char* mode = FILE_READ, bool create = false)
    {
        return open(path.c_str(), mode, create);
    }
    bool exists(const char* path);
    bool exists(const String& path)
    {
        return exists(path.c_str());
    }
    bool remove(const char* path);
    bool remove(const String& path)
    {
        return remove(path.c_str());
    }
    bool rename(const char* from, const char* to);
    bool rename(const String& from, const String& to)
    {
        return rename(from.c_str(), to.c_str());
    }
    bool mkdir(const char* path);
    bool mkdir(const String& path)
    {
        return mkdir(path.c_str());
    }
    bool rmdir(const char* path);
    bool rmdir(const String& path)
    {
        return rmdir(path.c_str());
    }

protected:
    FSImplPtr _impl;
};
}
#ifndef FS_NO_GLOBALS
using fs::File;
using fs::FS;
using fs::SeekCur;
using fs::SeekEnd;
using fs::SeekMode;
using fs::SeekSet;
#endif
