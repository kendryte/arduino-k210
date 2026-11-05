#include "Arduino.h"

#include "FS.h"
#include "SPIFFS.h"

#include "Image.h"
#include "KPU.h"
#include "k210-hal.h"
#include "kpu_helper.h"
#include "rtthread.h"

#include "unity/unity.h"

using namespace K210;

namespace
{
constexpr uint32_t MODEL_4M_FLASH_OFFSET = 4 * 1024 * 1024;
constexpr uint32_t MODEL_5M_FLASH_OFFSET = 5 * 1024 * 1024;
constexpr size_t MODEL_PROBE_SIZE = 64 * 1024;
constexpr size_t COPY_BUFFER_SIZE = 4096;
constexpr size_t MAX_MODEL_SIZE = 2 * 1024 * 1024;
constexpr uint32_t MODEL_INPUT_WIDTH = 64;
constexpr uint32_t MODEL_INPUT_HEIGHT = 64;

const char *MODEL_4M_PATH = "/model_4m.kmodel";
const char *MODEL_5M_PATH = "/model_5m.kmodel";

bool spiffs_mounted = false;
bool model_4m_available = false;
bool model_5m_available = false;

int probe_model_size(uint32_t flash_offset)
{
    uint8_t *header = static_cast<uint8_t *>(rt_malloc_align(MODEL_PROBE_SIZE, 8));
    if (header == nullptr)
    {
        return -1;
    }

    int model_size = -1;
    if (hal_flash_read_data(flash_offset, header, MODEL_PROBE_SIZE) == W25QXX_OK)
    {
        model_size = maix_kpu_helper_probe_model_size(header, MODEL_PROBE_SIZE);
    }
    rt_free_align(header);

    if (model_size <= 0 || static_cast<size_t>(model_size) > MAX_MODEL_SIZE)
    {
        return -1;
    }
    return model_size;
}

int provision_model(const char *path, uint32_t flash_offset)
{
    const int model_size = probe_model_size(flash_offset);
    if (model_size <= 0)
    {
        Serial.printf("Cannot find a valid KPU model at flash offset 0x%08x\n", flash_offset);
        return -1;
    }

    size_t replaceable_size = 0;
    File existing = SPIFFS.open(path, FILE_READ);
    if (existing)
    {
        replaceable_size = existing.size();
        const bool size_matches = replaceable_size == static_cast<size_t>(model_size);
        existing.close();
        if (size_matches)
        {
            return 0;
        }
    }

    const size_t available = SPIFFS.totalBytes() - SPIFFS.usedBytes() + replaceable_size;
    if (available < static_cast<size_t>(model_size))
    {
        Serial.printf("Not enough SPIFFS space for %s (%d bytes required)\n", path, model_size);
        return -1;
    }

    File destination = SPIFFS.open(path, FILE_WRITE);
    if (!destination)
    {
        return -1;
    }

    uint8_t *buffer = static_cast<uint8_t *>(rt_malloc_align(COPY_BUFFER_SIZE, 8));
    if (buffer == nullptr)
    {
        destination.close();
        return -1;
    }

    size_t copied = 0;
    while (copied < static_cast<size_t>(model_size))
    {
        const size_t remaining = static_cast<size_t>(model_size) - copied;
        const size_t chunk_size = remaining < COPY_BUFFER_SIZE ? remaining : COPY_BUFFER_SIZE;
        if (hal_flash_read_data(flash_offset + copied, buffer, chunk_size) != W25QXX_OK ||
            destination.write(buffer, chunk_size) != chunk_size)
        {
            break;
        }
        copied += chunk_size;
    }

    destination.flush();
    destination.close();
    rt_free_align(buffer);

    if (copied != static_cast<size_t>(model_size))
    {
        return -1;
    }

    Serial.printf("Provisioned %s in SPIFFS (%d bytes)\n", path, model_size);
    return 0;
}

size_t file_size(const char *path)
{
    File file = SPIFFS.open(path, FILE_READ);
    if (!file)
    {
        return 0;
    }
    const size_t size = file.size();
    file.close();
    return size;
}

int run_model_from_spiffs(const char *path)
{
    KPU_Base model;
    if (model.load_kmodel(SPIFFS, path) != 0)
    {
        return -1;
    }

    Image input(MODEL_INPUT_WIDTH, MODEL_INPUT_HEIGHT, IMAGE_FORMAT_RGBP888, true);
    if (input.data() == nullptr)
    {
        return -1;
    }
    for (size_t i = 0; i < input.size(); ++i)
    {
        input.data()[i] = static_cast<uint8_t>(i & 0xff);
    }

    if (model.run_kmodel(&input) != 0)
    {
        return -1;
    }

    uint8_t *result = nullptr;
    size_t result_size = 0;
    if (model.get_result(&result, &result_size) != 0 || result == nullptr || result_size == 0)
    {
        return -1;
    }
    return 0;
}
} // namespace

void setUp(void)
{
}

void tearDown(void)
{
}

void test_models_are_available_in_spiffs(void)
{
    const int model_4m_status = spiffs_mounted ? provision_model(MODEL_4M_PATH, MODEL_4M_FLASH_OFFSET) : -1;
    const int model_5m_status = spiffs_mounted ? provision_model(MODEL_5M_PATH, MODEL_5M_FLASH_OFFSET) : -1;
    const size_t model_4m_size = spiffs_mounted ? file_size(MODEL_4M_PATH) : 0;
    const size_t model_5m_size = spiffs_mounted ? file_size(MODEL_5M_PATH) : 0;
    model_4m_available = model_4m_status == 0 && model_4m_size > 0;
    model_5m_available = model_5m_status == 0 && model_5m_size > 0;

    TEST_ASSERT_TRUE(spiffs_mounted);
    TEST_ASSERT_EQUAL_INT(0, model_4m_status);
    TEST_ASSERT_EQUAL_INT(0, model_5m_status);
    TEST_ASSERT_GREATER_THAN(0, model_4m_size);
    TEST_ASSERT_GREATER_THAN(0, model_5m_size);
}

void test_model_at_4m_loads_and_runs_from_spiffs(void)
{
    if (!model_4m_available)
    {
        TEST_IGNORE_MESSAGE("The 4 MiB KPU model is unavailable in SPIFFS");
    }
    TEST_ASSERT_EQUAL_INT(0, run_model_from_spiffs(MODEL_4M_PATH));
}

void test_model_at_5m_loads_and_runs_from_spiffs(void)
{
    if (!model_5m_available)
    {
        TEST_IGNORE_MESSAGE("The 5 MiB KPU model is unavailable in SPIFFS");
    }
    TEST_ASSERT_EQUAL_INT(0, run_model_from_spiffs(MODEL_5M_PATH));
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    spiffs_mounted = SPIFFS.begin(true);
    UNITY_BEGIN();
    RUN_TEST(test_models_are_available_in_spiffs);
    RUN_TEST(test_model_at_4m_loads_and_runs_from_spiffs);
    RUN_TEST(test_model_at_5m_loads_and_runs_from_spiffs);
    UNITY_END();

    if (spiffs_mounted)
    {
        SPIFFS.end();
    }
}

void loop()
{
}
