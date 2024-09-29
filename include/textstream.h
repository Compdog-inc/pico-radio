#ifndef _TEXT_STREAM_H_
#define _TEXT_STREAM_H_

#include <stdlib.h>
#include <string>
#include "tcpclient.h"

// The default size of the TextStream buffer
constexpr size_t DEFAULT_TEXTSTREAM_BUFFER_SIZE = 128;

/// @brief Provides a way to use std::string with TcpClient
class TextStream
{
public:
    /// @brief Create a new TextStream based on a TcpClient
    /// @param source The client to read/write from
    TextStream(TcpClient *source);
    /// @brief Create a new TextStream based on a TcpClient
    /// @param source The client to read/write from
    /// @param bufferSize A custom buffer size to use
    TextStream(TcpClient *source, size_t bufferSize);
    /// @brief Free the internal buffer
    ~TextStream();

    /// @brief Reads a line from the TcpClient
    /// @return A std::string containing the line
    std::string readLine();
    /// @brief Reads a line from the TcpClient
    /// @param timeout The timeout passed to the TcpClient
    /// @return A std::string containing the line (empty on timeout)
    std::string readLine(uint32_t timeout);

    /// @brief Writes a std::string to the TcpClient
    /// @param str The string to write
    /// @return True on success
    bool writeString(std::string str);
    /// @brief Writes a std::string_view to the TcpClient
    /// @param str The string to write
    /// @return True on success
    bool writeString(std::string_view str);

private:
    TcpClient *source;
    size_t bufferSize;
    uint8_t *buffer;

    ssize_t rpos = 0;
    ssize_t rend = 0;
};

#endif