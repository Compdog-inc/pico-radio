#include <pico/stdlib.h>
#include <string>
#include <FreeRTOS.h>
#include "textstream.h"
#include "tcpclient.h"

using namespace std::literals;

TextStream::TextStream(TcpClient *source) : TextStream(source, DEFAULT_TEXTSTREAM_BUFFER_SIZE)
{
}

TextStream::TextStream(TcpClient *source, size_t bufferSize) : source(source), bufferSize(bufferSize)
{
    buffer = (uint8_t *)pvPortMalloc(bufferSize);
    assert(buffer != nullptr);
}

TextStream::~TextStream()
{
    vPortFree(buffer);
}

/// @brief Shifts the data in a buffer
/// @param buf The buffer
/// @param len The length of the buffer
/// @param shift A signed offset to shift the data by
void shift_buffer(uint8_t *buf, ssize_t len, ssize_t shift)
{
    if (shift < 0)
    {
        for (ssize_t i = 0; i < len; i++)
        {
            if (i + shift < len && i + shift >= 0)
                buf[i + shift] = buf[i];
        }
    }
    else
    {
        for (ssize_t i = len - 1; i >= 0; i--)
        {
            if (i + shift < len && i + shift >= 0)
                buf[i + shift] = buf[i];
        }
    }
}

std::string TextStream::readLine()
{
    return readLine(TCP_INFINITE_TIMEOUT);
}

std::string TextStream::readLine(uint32_t timeout)
{
    if (end < rend)
    {
    check_line:
        while (end < rend && buffer[end] != '\n' && buffer[end] != '\r')
            end++;
        if (end < rend && (buffer[end] == '\n' || buffer[end] == '\r'))
        {
            std::string str = std::string((char *)(buffer + rstart), end - rstart);

            if (buffer[end] == '\r' && end + 1 < rend && buffer[end + 1] == '\n') // special CR+LF handling
            {
                end++;
            }

            end++;
            rpos = end;
            rstart = rpos;

            return str;
        }
    }
    else
    {
        ssize_t unread = end - rstart;
        rpos = unread;
        shift_buffer(buffer, bufferSize, -rstart); // shift unread data to the start
    }

    int rc;

    while (source->isConnected() && (rc = source->readBytes(
                                         buffer + rpos,     // make sure to write new data after existing
                                         bufferSize - rpos, // make space for existing data
                                         timeout)) > 0)
    {
        if (rc)
        {
            rstart = 0;
            rend = rpos + rc;
            end = rpos;

            if (end < rend)
            {
                goto check_line;
            }
        }
        else
        {
            return "";
        }
    }

    return "";
}

bool TextStream::writeString(std::string str)
{
    return writeString((std::string_view)str);
}

bool TextStream::writeString(std::string_view str)
{
    ssize_t rc = source->writeBytes(str.data(), str.length());

    if (rc < 0)
    {
        return false;
    }
    else
    {
        return (size_t)rc == str.length();
    }
}