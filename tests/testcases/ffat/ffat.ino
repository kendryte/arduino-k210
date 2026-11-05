#include "FS.h"
#include "FFat.h"
#include <errno.h>
#include <string.h>

#include "unity/unity.h"

int listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return -1;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return -1;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.path(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    return 0;
}

int readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return -1;
    }

    Serial.println("- read from file:");
    while (file.available())
    {
        Serial.write(file.read());
    }
    file.close();

    return 0;
}

int writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return -1;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
        goto fail;
    }
    file.close();
    return 0;
fail:
    file.close();
    return -1;
}

int appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("- failed to open file for appending");
        return -1;
    }
    if (file.print(message))
    {
        Serial.println("- message appended");
    }
    else
    {
        Serial.println("- append failed");
        goto fail;
    }
    file.close();
    return 0;
fail:
    file.close();
    return -1;
}

int renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("- file renamed");
    }
    else
    {
        Serial.println("- rename failed");
        goto fail;
    }
    return 0;
fail:

    return -1;
}

int deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\r\n", path);
    errno = 0;
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        int error = errno;
        Serial.printf("- delete failed: errno=%d (%s)\r\n", error, strerror(error));
        goto fail;
    }
    return 0;
fail:

    return -1;
}

int testFileIO(fs::FS &fs, const char *path)
{
    Serial.printf("Testing file I/O with %s\r\n", path);

    uint8_t buf[512];
    const size_t blocks = 64;
    const size_t expectedSize = blocks * sizeof(buf);
    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        int error = errno;
        Serial.printf("- open for writing failed: errno=%d (%s)\r\n", error, strerror(error));
        return -1;
    }

    uint32_t start = millis();
    for (size_t i = 0; i < blocks; i++)
    {
        for (size_t j = 0; j < sizeof(buf); j++)
        {
            buf[j] = static_cast<uint8_t>(i + j);
        }
        errno = 0;
        size_t written = file.write(buf, sizeof(buf));
        if (written != sizeof(buf))
        {
            int error = errno;
            Serial.printf("- short write at block %u: %u/%u bytes, errno=%d (%s)\r\n",
                          (unsigned)i, (unsigned)written, (unsigned)sizeof(buf), error, strerror(error));
            file.close();
            return -1;
        }
    }
    file.close();
    Serial.printf("- %u bytes written in %u ms\r\n", (unsigned)expectedSize, (unsigned)(millis() - start));

    file = fs.open(path, FILE_READ);
    if (!file || file.isDirectory())
    {
        int error = errno;
        Serial.printf("- open for reading failed: errno=%d (%s)\r\n", error, strerror(error));
        file.close();
        return -1;
    }
    if (file.size() != expectedSize)
    {
        Serial.printf("- wrong file size: %u, expected %u\r\n", (unsigned)file.size(), (unsigned)expectedSize);
        file.close();
        return -1;
    }
    start = millis();
    for (size_t i = 0; i < blocks; i++)
    {
        errno = 0;
        size_t count = file.read(buf, sizeof(buf));
        if (count != sizeof(buf))
        {
            int error = errno;
            Serial.printf("- short read at block %u: %u/%u bytes, errno=%d (%s)\r\n",
                          (unsigned)i, (unsigned)count, (unsigned)sizeof(buf), error, strerror(error));
            file.close();
            return -1;
        }
        for (size_t j = 0; j < sizeof(buf); j++)
        {
            if (buf[j] != static_cast<uint8_t>(i + j))
            {
                Serial.printf("- data mismatch at block %u, byte %u\r\n", (unsigned)i, (unsigned)j);
                file.close();
                return -1;
            }
        }
    }
    file.close();
    Serial.printf("- %u bytes verified in %u ms\r\n", (unsigned)expectedSize, (unsigned)(millis() - start));
    return 0;
}

int fsInit(void)
{
    if (!FFat.begin())
    {
        Serial.println("FFat Mount Failed");
        return -1;
    }

    return 0;
}
/*****************************************************************************/
/* These functions are intended to be called before and after each test. */
void setUp(void)
{
}

void tearDown(void)
{
}

void testListDir(void)
{
    TEST_ASSERT_EQUAL(0, listDir(FFat, "/", 0));
}

void testwriteFile(void)
{
    TEST_ASSERT_EQUAL(0, writeFile(FFat, "/hello.txt", "Hello "));
}

void testappendFile(void)
{
    TEST_ASSERT_EQUAL(0, appendFile(FFat, "/hello.txt", "World!\r\n"));
}

void testreadFile(void)
{
    TEST_ASSERT_EQUAL(0, readFile(FFat, "/hello.txt"));
}

void testrenameFile(void)
{
    TEST_ASSERT_EQUAL(0, renameFile(FFat, "/hello.txt", "/foo.txt"));
    TEST_ASSERT_EQUAL(0, readFile(FFat, "/foo.txt"));
    TEST_ASSERT_EQUAL(0, deleteFile(FFat, "/foo.txt"));
}

void testtestFileIO(void)
{
    // Exercise consecutive read/delete/recreate operations without a delay.
    for (size_t i = 0; i < 3; i++)
    {
        TEST_ASSERT_EQUAL(0, testFileIO(FFat, "/test.txt"));
        TEST_ASSERT_EQUAL(0, deleteFile(FFat, "/test.txt"));
        TEST_ASSERT_FALSE(FFat.exists("/test.txt"));
    }
}

void testInit(void)
{
    TEST_ASSERT_EQUAL(0, fsInit());
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        ;
    }

    UNITY_BEGIN();
    RUN_TEST(testInit);
    RUN_TEST(testListDir);
    RUN_TEST(testwriteFile);
    RUN_TEST(testappendFile);
    RUN_TEST(testreadFile);
    RUN_TEST(testrenameFile);
    RUN_TEST(testtestFileIO);

    UNITY_END();
}

void loop()
{
}
