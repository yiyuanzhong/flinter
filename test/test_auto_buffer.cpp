#include <gtest/gtest.h>

#include <stdio.h>

#include <vector>

#include <flinter/types/auto_buffer.h>
#include <flinter/logger.h>

TEST(AutoBufferTest, TestCompileWithType)
{
    LOG(INFO) << "BEGIN";
    flinter::AutoBuffer<char> buffer(128 * 1024 * 1024);
    char *p = buffer.get();
    printf("%p\n", p);
    buffer[0] = 'a';
    buffer[1] = '\0';
    printf("%s\n", &buffer[0]);
    LOG(INFO) << "END";
}

TEST(AutoBufferTest, TestCompileWithoutType)
{
    LOG(INFO) << "BEGIN";
    flinter::AutoBuffer<> buffer(128 * 1024 * 1024);
    unsigned char *p = buffer.get();
    printf("%p\n", p);
    buffer[0] = 'a';
    buffer[1] = '\0';
    printf("%s\n", &buffer[0]);
    LOG(INFO) << "END";
}

TEST(AutoBufferTest, TestComparedToVector)
{
    LOG(INFO) << "BEGIN";
    std::vector<char> buffer(128 * 1024 * 1024);
    char *p = &buffer[0];
    printf("%p\n", p);
    buffer[0] = 'a';
    buffer[1] = '\0';
    printf("%s\n", &buffer[0]);
    LOG(INFO) << "END";
}
