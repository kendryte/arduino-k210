// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#define DBG_TAG "vfs_api"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "vfs_api.h"

#include "rtthread.h"

using namespace fs;

#define DEFAULT_FILE_BUFFER_SIZE 4096

FileImplPtr VFSImpl::open(const char* fpath, const char* mode, const bool create)
{
    if(!_mountpoint) {
        LOG_E("File system is not mounted");
        return FileImplPtr();
    }

    if(!fpath || fpath[0] != '/') {
        LOG_E("%s does not start with /", fpath);
        return FileImplPtr();
    }

    char * temp = (char *)rt_malloc(rt_strlen(fpath)+rt_strlen(_mountpoint)+2);
    if(!temp) {
        LOG_E("rt_malloc failed");
        return FileImplPtr();
    }

    rt_sprintf(temp,"%s%s", _mountpoint, fpath);

    struct stat st;
    //file found
    if(!stat(temp, &st)) {
        rt_free(temp);
        if (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)) {
            return std::make_shared<VFSFileImpl>(this, fpath, mode);
        }
        LOG_E("%s has wrong mode 0x%08X", fpath, st.st_mode);
        return FileImplPtr();
    }

    //try to open this as directory (might be mount point)
    DIR * d = opendir(temp);
    if(d) {
        closedir(d);
        rt_free(temp);
        return std::make_shared<VFSFileImpl>(this, fpath, mode);
    }

    //file not found but mode permits file creation without folder creation
    if((mode && mode[0] != 'r') && (!create)){
        rt_free(temp);
        return std::make_shared<VFSFileImpl>(this, fpath, mode);
    }

    ////file not found but mode permits file creation and folder creation
    if((mode && mode[0] != 'r') && create){

        char *token;
        char *folder = (char *)rt_malloc(rt_strlen(fpath));

        int start_index = 0;
        int end_index = 0;

        token = strchr(fpath+1,'/');
        end_index = (token-fpath);

        while (token != NULL)
        {
            rt_memcpy(folder,fpath + start_index, end_index-start_index);
            folder[end_index-start_index] = '\0';
            
            if(!VFSImpl::mkdir(folder))
            {
                LOG_E("Creating folder: %s failed!",folder);
                return FileImplPtr();
            }

            token = strchr(token+1,'/');
            if(token != NULL)
            {
                end_index = (token-fpath);
                rt_memset(folder, 0, rt_strlen(folder));
            }
            
        }

        rt_free(folder);
        rt_free(temp);

        return std::make_shared<VFSFileImpl>(this, fpath, mode);
    }

    LOG_I("%s does not exist, no permits for creation, mode %s, %s", temp, mode, create ? "C" : "NC");

    rt_free(temp);

    return FileImplPtr();
}

bool VFSImpl::exists(const char* fpath)
{
    if(!_mountpoint) {
        LOG_E("File system is not mounted");
        return false;
    }

    VFSFileImpl f(this, fpath, "r");
    if(f) {
        f.close();
        return true;
    }
    return false;
}

bool VFSImpl::rename(const char* pathFrom, const char* pathTo)
{
    if(!_mountpoint) {
        LOG_E("File system is not mounted");
        return false;
    }

    if(!pathFrom || pathFrom[0] != '/' || !pathTo || pathTo[0] != '/') {
        LOG_E("bad arguments");
        return false;
    }
    if(!exists(pathFrom)) {
        LOG_E("%s does not exists", pathFrom);
        return false;
    }
    char * temp1 = (char *)rt_malloc(rt_strlen(pathFrom)+rt_strlen(_mountpoint)+1);
    if(!temp1) {
        LOG_E("rt_malloc failed");
        return false;
    }
    char * temp2 = (char *)rt_malloc(rt_strlen(pathTo)+rt_strlen(_mountpoint)+1);
    if(!temp2) {
        rt_free(temp1);
        LOG_E("rt_malloc failed");
        return false;
    }
    rt_sprintf(temp1,"%s%s", _mountpoint, pathFrom);
    rt_sprintf(temp2,"%s%s", _mountpoint, pathTo);
    auto rc = ::rename(temp1, temp2);
    rt_free(temp1);
    rt_free(temp2);
    return rc == 0;
}

bool VFSImpl::remove(const char* fpath)
{
    if(!_mountpoint) {
        LOG_E("File system is not mounted");
        return false;
    }

    if(!fpath || fpath[0] != '/') {
        LOG_E("bad arguments");
        return false;
    }

    VFSFileImpl f(this, fpath, "r");
    if(!f || f.isDirectory()) {
        if(f) {
            f.close();
        }
        LOG_E("%s does not exists or is directory", fpath);
        return false;
    }
    f.close();

    char * temp = (char *)rt_malloc(rt_strlen(fpath)+rt_strlen(_mountpoint)+1);
    if(!temp) {
        LOG_E("rt_malloc failed");
        return false;
    }
    rt_sprintf(temp,"%s%s", _mountpoint, fpath);
    auto rc = unlink(temp);
    rt_free(temp);
    return rc == 0;
}

bool VFSImpl::mkdir(const char *fpath)
{
    if(!_mountpoint) {
        LOG_E("File system is not mounted");
        return false;
    }

    VFSFileImpl f(this, fpath, "r");
    if(f && f.isDirectory()) {
        f.close();
        LOG_I("%s already exists", fpath);
        return true;
    } else if(f) {
        f.close();
        LOG_I("%s is a file", fpath);
        return false;
    }

    char * temp = (char *)rt_malloc(rt_strlen(fpath)+rt_strlen(_mountpoint)+1);
    if(!temp) {
        LOG_E("rt_malloc failed");
        return false;
    }
    rt_sprintf(temp,"%s%s", _mountpoint, fpath);
    // auto rc = ::mkdir(temp, ACCESSPERMS);
    auto rc = ::mkdir(temp, 0);
    rt_free(temp);
    return rc == 0;
}

bool VFSImpl::rmdir(const char *fpath)
{
    if(!_mountpoint) {
        LOG_E("File system is not mounted");
        return false;
    }

    if (strcmp(_mountpoint, "/spiffs") == 0) {
        LOG_E("rmdir is unnecessary in SPIFFS");
        return false;
    }

    VFSFileImpl f(this, fpath, "r");
    if(!f || !f.isDirectory()) {
        if(f) {
            f.close();
        }
        LOG_I("%s does not exists or is a file", fpath);
        return false;
    }
    f.close();

    char * temp = (char *)rt_malloc(rt_strlen(fpath)+rt_strlen(_mountpoint)+1);
    if(!temp) {
        LOG_E("rt_malloc failed");
        return false;
    }
    rt_sprintf(temp,"%s%s", _mountpoint, fpath);
    auto rc = ::rmdir(temp);
    rt_free(temp);
    return rc == 0;
}




VFSFileImpl::VFSFileImpl(VFSImpl* fs, const char* fpath, const char* mode)
    : _fs(fs)
    , _f(NULL)
    , _d(NULL)
    , _path(NULL)
    , _isDirectory(false)
    , _written(false)
{
    char * temp = (char *)rt_malloc(rt_strlen(fpath)+rt_strlen(_fs->_mountpoint)+1);
    if(!temp) {
        return;
    }
    rt_sprintf(temp,"%s%s", _fs->_mountpoint, fpath);

    _path = rt_strdup(fpath);
    if(!_path) {
        LOG_E("strdup(%s) failed", fpath);
        rt_free(temp);
        return;
    }

    if(!stat(temp, &_stat)) {
        //file found
        if (S_ISREG(_stat.st_mode)) {
            _isDirectory = false;
            _f = fopen(temp, mode);
            if(!_f) {
                LOG_E("fopen(%s) failed", temp);
            }
            if(_f && (_stat.st_blksize == 0))
            {
                setvbuf(_f,NULL,_IOFBF,DEFAULT_FILE_BUFFER_SIZE);
            } 
        } else if(S_ISDIR(_stat.st_mode)) {
            _isDirectory = true;
            _d = opendir(temp);
            if(!_d) {
                LOG_E("opendir(%s) failed", temp);
            }
        } else {
            LOG_E("Unknown type 0x%08X for file %s", ((_stat.st_mode)&_IFMT), temp);
        }
    } else {
        //file not found
        if(!mode || mode[0] == 'r') {
            //try to open as directory
            _d = opendir(temp);
            if(_d) {
                _isDirectory = true;
            } else {
                _isDirectory = false;
                //log_w("stat(%s) failed", temp);
            }
        } else {
            //lets create this new file
            _isDirectory = false;
            _f = fopen(temp, mode);
            if(!_f) {
                LOG_E("fopen(%s) failed", temp);
            }
            if(_f && (_stat.st_blksize == 0))
            {
                setvbuf(_f,NULL,_IOFBF,DEFAULT_FILE_BUFFER_SIZE);
            } 
        }
    }
    rt_free(temp);
}

VFSFileImpl::~VFSFileImpl()
{
    close();
}

void VFSFileImpl::close()
{
    if(_path) {
        rt_free(_path);
        _path = NULL;
    }
    if(_isDirectory && _d) {
        closedir(_d);
        _d = NULL;
        _isDirectory = false;
    } else if(_f) {
        fclose(_f);
        _f = NULL;
    }
}

VFSFileImpl::operator bool()
{
    return (_isDirectory && _d != NULL) || _f != NULL;
}

time_t VFSFileImpl::getLastWrite() {
    _getStat() ;
    return _stat.st_mtime;
}

void VFSFileImpl::_getStat() const
{
    if(!_path) {
        return;
    }
    char * temp = (char *)rt_malloc(rt_strlen(_path)+rt_strlen(_fs->_mountpoint)+1);
    if(!temp) {
        return;
    }
    rt_sprintf(temp,"%s%s", _fs->_mountpoint, _path);
    if(!stat(temp, &_stat)) {
        _written = false;
    }
    rt_free(temp);
}

size_t VFSFileImpl::write(const uint8_t *buf, size_t size)
{
    if(_isDirectory || !_f || !buf || !size) {
        LOG_E("_isDirectory %d || !_f %d || !buf %p || !size %d\n", _isDirectory, !_f, !buf, !size);
        return 0;
    }
    _written = true;
    return fwrite(buf, 1, size, _f);
}

size_t VFSFileImpl::read(uint8_t* buf, size_t size)
{
    if(_isDirectory || !_f || !buf || !size) {
        return 0;
    }

    return fread(buf, 1, size, _f);
}

void VFSFileImpl::flush()
{
    if(_isDirectory || !_f) {
        return;
    }
    fflush(_f);
    // workaround for https://github.com/espressif/arduino-esp32/issues/1293
    fsync(fileno(_f));
}

bool VFSFileImpl::seek(uint32_t pos, SeekMode mode)
{
    if(_isDirectory || !_f) {
        return false;
    }
    auto rc = fseek(_f, pos, mode);
    return rc == 0;
}

size_t VFSFileImpl::position() const
{
    if(_isDirectory || !_f) {
        return 0;
    }
    return ftell(_f);
}

size_t VFSFileImpl::size() const
{
    if(_isDirectory || !_f) {
        return 0;
    }
    if (_written) {
        _getStat();
    }
    return _stat.st_size;
}

/*
* Change size of files internal buffer used for read / write operations.
* Need to be called right after opening file before any other operation!
*/
bool VFSFileImpl::setBufferSize(size_t size)
{
    if(_isDirectory || !_f) {
        return 0;
    }
    int res = setvbuf(_f,NULL,_IOFBF,size);
    return res == 0;
}

const char* VFSFileImpl::path() const
{
    return (const char*) _path;
}

const char* VFSFileImpl::name() const
{
    return pathToFileName(path());
}

//to implement
boolean VFSFileImpl::isDirectory(void)
{
    return _isDirectory;
}

FileImplPtr VFSFileImpl::openNextFile(const char* mode)
{
    if(!_isDirectory || !_d) {
        return FileImplPtr();
    }
    struct dirent *file = readdir(_d);
    if(file == NULL) {
        return FileImplPtr();
    }
    if(file->d_type != DT_REG && file->d_type != DT_DIR) {
        return openNextFile(mode);
    }
    String fname = String(file->d_name);
    String name = String(_path);
    if(!fname.startsWith("/") && !name.endsWith("/")) {
        name += "/";
    }
    name += fname;

    return std::make_shared<VFSFileImpl>(_fs, name.c_str(), mode);
}

void VFSFileImpl::rewindDirectory(void)
{
    if(!_isDirectory || !_d) {
        return;
    }
    rewinddir(_d);
}
