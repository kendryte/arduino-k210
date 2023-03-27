#include "FS.h"
#include "FFat.h"

#include "unity/unity.h"

// You only need to format FFat the first time you run a test
#define FORMAT_FFAT true

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
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        Serial.println("- delete failed");
        goto fail;
    }
    return 0;
fail:

    return -1;
}

int testFileIO(fs::FS &fs, const char *path)
{
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return -1;
    }

    size_t i;
    Serial.print("- writing");
    uint32_t start = millis();
    for (i = 0; i < 64; i++)
    {
        if ((i & 0x001F) == 0x001F)
        {
            Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 64 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if (file && !file.isDirectory())
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading");
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F)
            {
                Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %ld bytes read in %u ms\r\n", flen, end);
        file.close();
    }
    else
    {
        Serial.println("- failed to open file for reading");
        goto fail;
    }
    return 0;
fail:
    file.close();
    return -1;
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
    TEST_ASSERT_EQUAL(0, testFileIO(FFat, "/test.txt"));
    TEST_ASSERT_EQUAL(0, deleteFile(FFat, "/test.txt"));
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
