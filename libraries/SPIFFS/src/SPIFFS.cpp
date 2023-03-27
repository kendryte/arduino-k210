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

#define DBG_TAG "ESP_SPIFFS"
#define DBG_LVL DBG_WARNING
#include "rtdbg.h"

#include "vfs_api.h"

extern "C" {
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "spiffs/esp_spiffs.h"
}

#include "SPIFFS.h"

#include "rtthread.h"

using namespace fs;

class SPIFFSImpl : public VFSImpl
{
public:
    SPIFFSImpl();
    virtual ~SPIFFSImpl() { }
    virtual bool exists(const char* path);
};

SPIFFSImpl::SPIFFSImpl()
{
}

bool SPIFFSImpl::exists(const char* path)
{
    File f = open(path, "r",false);
    return (f == true) && !f.isDirectory();
}

SPIFFSFS::SPIFFSFS() : FS(FSImplPtr(new SPIFFSImpl())), partitionLabel_(NULL)
{

}

SPIFFSFS::~SPIFFSFS()
{
    if (partitionLabel_){
        rt_free(partitionLabel_);
        partitionLabel_ = NULL;
    }
}

bool SPIFFSFS::begin(bool formatOnFail, const char * basePath, uint8_t maxOpenFiles, const char * partitionLabel)
{
    if (partitionLabel_){
        rt_free(partitionLabel_);
        partitionLabel_ = NULL;
    }

    if (partitionLabel){
        partitionLabel_ = rt_strdup(partitionLabel);
    }

    if(esp_spiffs_mounted(partitionLabel_)){
        LOG_W("SPIFFS Already Mounted!");
        return true;
    }

    esp_vfs_spiffs_conf_t conf = {
      .base_path = basePath,
      .partition_label = partitionLabel_,
      .max_files = maxOpenFiles,
      .format_if_mount_failed = false
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if(err == ESP_FAIL && formatOnFail){
        if(format()){
            err = esp_vfs_spiffs_register(&conf);
        }
    }
    if(err != ESP_OK){
        LOG_E("Mounting SPIFFS failed! Error: %d", err);
        return false;
    }
    _impl->mountpoint(basePath);
    return true;
}

void SPIFFSFS::end()
{
    if(esp_spiffs_mounted(partitionLabel_)){
        esp_err_t err = esp_vfs_spiffs_unregister(partitionLabel_);
        if(err){
            LOG_E("Unmounting SPIFFS failed! Error: %d", err);
            return;
        }
        _impl->mountpoint(NULL);
    }
}

bool SPIFFSFS::format()
{
    // disableCore0WDT();
    esp_err_t err = esp_spiffs_format(partitionLabel_);
    // enableCore0WDT();
    if(err){
        LOG_E("Formatting SPIFFS failed! Error: %d", err);
        return false;
    }
    return true;
}

size_t SPIFFSFS::totalBytes()
{
    size_t total,used;
    if(esp_spiffs_info(partitionLabel_, &total, &used)){
        return 0;
    }
    return total;
}

size_t SPIFFSFS::usedBytes()
{
    size_t total,used;
    if(esp_spiffs_info(partitionLabel_, &total, &used)){
        return 0;
    }
    return used;
}

SPIFFSFS SPIFFS;

