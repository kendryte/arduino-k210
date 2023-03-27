#include "compatible.h"

#include "k210-hal.h"
#include "rtthread.h"

#include "pins_arduino.h"

static const esp_partition_t _partition_table[] = {
    {.address = FLASH_FS_ADDR, .size = FLASH_FS_SIZE, .label = "builtin_flash"},
};

const esp_partition_t *esp_partition_find_first(const char *label)
{
    for(int i = 0; i < sizeof(_partition_table) / sizeof(_partition_table[0]); i++) {
        if(0x00 == rt_strncmp(label, _partition_table[i].label, 17)) {
            return &_partition_table[i];
        }
    }

    return NULL;
}

esp_err_t esp_partition_erase_range(const esp_partition_t* partition, size_t offset, size_t size)
{
    return hal_flash_erase(partition->address + offset, size);
}

size_t strlcat(char * dest, const char * src, size_t n)
{
	size_t dsize = rt_strlen(dest);
	size_t len = rt_strlen(src);
	size_t res = dsize + len;

	dest += dsize;
	n -= dsize;

	if (len >= n)
		len = n-1;

	rt_memcpy(dest, src, len);
	dest[len] = 0;

	return res;
}

size_t strlcpy(char * dest, const char * src, size_t n)
{
	size_t len;
	size_t ret = rt_strlen(src);

	if (n) {
		len = (ret >= n) ? n - 1 : ret;
		rt_memcpy(dest, src, len);
		dest[len] = '\0';
	}

	return ret;
}
