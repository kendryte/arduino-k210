// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#pragma once
#include "FS.h"
namespace fs {
// Backend contracts consumed by the Apache-2.0 VFS adapter.
class FileImpl {
public:
    virtual ~FileImpl()                                          = default;
    virtual             operator bool()                          = 0;
    virtual size_t      read(uint8_t* output, size_t count)      = 0;
    virtual size_t      write(const uint8_t* data, size_t count) = 0;
    virtual bool        seek(uint32_t position, SeekMode mode)   = 0;
    virtual size_t      position() const                         = 0;
    virtual size_t      size() const                             = 0;
    virtual bool        setBufferSize(size_t count)              = 0;
    virtual void        flush()                                  = 0;
    virtual void        close()                                  = 0;
    virtual time_t      getLastWrite()                           = 0;
    virtual const char* path() const                             = 0;
    virtual const char* name() const                             = 0;
    virtual bool        isDirectory()                            = 0;
    virtual FileImplPtr openNextFile(const char* mode)           = 0;
    virtual void        rewindDirectory()                        = 0;
};
class FSImpl {
public:
    virtual ~FSImpl()                                                         = default;
    virtual FileImplPtr open(const char* path, const char* mode, bool create) = 0;
    virtual bool        exists(const char* path)                              = 0;
    virtual bool        rename(const char* from, const char* to)              = 0;
    virtual bool        remove(const char* path)                              = 0;
    virtual bool        mkdir(const char* path)                               = 0;
    virtual bool        rmdir(const char* path)                               = 0;
    void                mountpoint(const char* path);
    const char*         mountpoint();

protected:
    const char* _mountpoint = nullptr;
};
}
