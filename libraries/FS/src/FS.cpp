// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2026, arduino-k210 contributors
// License: LICENSE

#include "FS.h"
#include "FSImpl.h"
#include <limits.h>
#include <utility>
namespace fs {
File::File(FileImplPtr implementation)
    : _p(std::move(implementation))
{
    setTimeout(0);
}
File::operator bool() const
{
    return _p && static_cast<bool>(*_p);
}
size_t File::write(uint8_t value)
{
    return write(&value, 1);
}
size_t File::write(const uint8_t* data, size_t count)
{
    return *this && data ? _p->write(data, count) : 0;
}
size_t File::read(uint8_t* output, size_t count)
{
    return *this && output ? _p->read(output, count) : 0;
}
int File::read()
{
    uint8_t value;
    return read(&value, 1) == 1 ? value : -1;
}
size_t File::readBytes(char* output, size_t count)
{
    return read(reinterpret_cast<uint8_t*>(output), count);
}
int File::available()
{
    if (!*this || isDirectory())
        return 0;
    const size_t length = size(), offset = position();
    if (offset >= length)
        return 0;
    return length - offset > INT_MAX ? INT_MAX : static_cast<int>(length - offset);
}
int File::peek()
{
    if (!*this)
        return -1;
    const size_t offset = position();
    if (offset > UINT32_MAX)
        return -1;
    const int value = read();
    return seek(static_cast<uint32_t>(offset)) ? value : -1;
}
void File::flush()
{
    if (*this)
        _p->flush();
}
bool File::seek(uint32_t offset, SeekMode mode)
{
    return *this && mode >= SeekSet && mode <= SeekEnd && _p->seek(offset, mode);
}
size_t File::position() const
{
    return *this ? _p->position() : 0;
}
size_t File::size() const
{
    return *this ? _p->size() : 0;
}
bool File::setBufferSize(size_t count)
{
    return *this && _p->setBufferSize(count);
}
void File::close()
{
    if (_p) {
        _p->close();
        _p.reset();
    }
}
time_t File::getLastWrite()
{
    return *this ? _p->getLastWrite() : 0;
}
const char* File::path() const
{
    return *this ? _p->path() : nullptr;
}
const char* File::name() const
{
    return *this ? _p->name() : nullptr;
}
bool File::isDirectory()
{
    return *this && _p->isDirectory();
}
File File::openNextFile(const char* mode)
{
    return File(*this && mode ? _p->openNextFile(mode) : FileImplPtr());
}
void File::rewindDirectory()
{
    if (*this)
        _p->rewindDirectory();
}
File FS::open(const char* path, const char* mode, bool create)
{
    return File(_impl && path && mode ? _impl->open(path, mode, create) : FileImplPtr());
}
bool FS::exists(const char* path)
{
    return _impl && path && _impl->exists(path);
}
bool FS::remove(const char* path)
{
    return _impl && path && _impl->remove(path);
}
bool FS::rename(const char* from, const char* to)
{
    return _impl && from && to && _impl->rename(from, to);
}
bool FS::mkdir(const char* path)
{
    return _impl && path && _impl->mkdir(path);
}
bool FS::rmdir(const char* path)
{
    return _impl && path && _impl->rmdir(path);
}
void FSImpl::mountpoint(const char* path)
{
    _mountpoint = path;
}
const char* FSImpl::mountpoint()
{
    return _mountpoint;
}
}
