//
// Inspired by cppack (https://github.com/mikeloomisgg/cppack)
//

#ifndef NT_JSON_HPP
#define NT_JSON_HPP

#include <string>
#include <charconv>
#include <concepts>
#include <system_error>

namespace json
{
    enum class UnpackerError
    {
        OutOfRange = 1
    };

    struct UnpackerErrCategory : public std::error_category
    {
    public:
        const char *name() const noexcept override
        {
            return "unpacker";
        };

        std::string message(int ev) const override
        {
            switch (static_cast<json::UnpackerError>(ev))
            {
            case json::UnpackerError::OutOfRange:
                return "tried to dereference out of range during deserialization";
            default:
                return "(unrecognized error)";
            }
        };
    };

    inline std::error_code make_error_code(json::UnpackerError e)
    {
        static UnpackerErrCategory theUnpackerErrCategory;
        return {static_cast<int>(e), theUnpackerErrCategory};
    }
};

namespace std
{
    template <>
    struct is_error_code_enum<json::UnpackerError> : public true_type
    {
    };
}

namespace json
{
    using namespace std::literals;

    template <typename T>
    concept CharType = std::is_same_v<T, char>;

    enum DataType : uint8_t
    {
        Array,
        ArrayEnd,
        Object,
        ObjectEnd,
        Str,
        True,
        False,
        Int,
        Null
    };

    class Unpacker
    {
    public:
        Unpacker() : data_pointer(nullptr), data_end(nullptr) {};

        Unpacker(const uint8_t *data_start, std::size_t bytes)
            : data_pointer(data_start), data_end(data_start + bytes) {};

        void set_data(const uint8_t *pointer, std::size_t size)
        {
            data_pointer = pointer;
            data_end = data_pointer + size;
        }

        void unpack_array()
        {
            safe_find_after('[');
        }

        void unpack_array_end()
        {
            safe_find_after(']');
        }

        void unpack_object()
        {
            safe_find_after('{');
        }

        void unpack_object_end()
        {
            safe_find_after('}');
        }

        std::string_view unpack_key()
        {
            safe_find_after('"');
            if (ec == UnpackerError::OutOfRange)
                return ""sv;

            const uint8_t *keyBegin = data_pointer;
            safe_find('"');
            if (ec == UnpackerError::OutOfRange)
                return ""sv;

            std::string_view key((const char *)keyBegin, data_pointer - keyBegin);
            safe_increment();

            return key;
        }

        inline std::string_view unpack_string() { return unpack_key(); }

        int32_t unpack_int()
        {
            safe_find('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '.');
            if (ec == UnpackerError::OutOfRange)
                return 0;

            const uint8_t *begin = data_pointer;
            safe_find(',', '}');
            if (ec == UnpackerError::OutOfRange)
                return 0;

            std::string_view asStr((const char *)begin, data_pointer - begin);

            int32_t i;
            if (std::from_chars(asStr.data(), asStr.data() + asStr.size(), i).ec == std::errc::invalid_argument)
            {
                ec = UnpackerError::OutOfRange;
                return 0;
            }

            return i;
        }

        bool unpack_bool()
        {
            char c;
            do
            {
                c = safe_data();
                if (ec == UnpackerError::OutOfRange)
                    break;

                switch (c)
                {
                case 't':
                    if (data_pointer + 3 < data_end && data_pointer[1] == 'r' && data_pointer[2] == 'u' && data_pointer[3] == 'e')
                    {
                        data_pointer += 4;
                        return true;
                    }
                    break;
                case 'f':
                    if (data_pointer + 4 < data_end && data_pointer[1] == 'a' && data_pointer[2] == 'l' && data_pointer[3] == 's' && data_pointer[4] == 'e')
                    {
                        data_pointer += 5;
                        return false;
                    }
                    break;
                default:
                    break;
                }

                ec = safe_peek_increment(data_pointer);
            } while (ec != UnpackerError::OutOfRange);

            return false;
        }

        DataType peek_type() const
        {
            DataType type;
            if (peek_type(&type) == UnpackerError::OutOfRange)
                return Null;
            else
                return type;
        }

        inline bool is_bool(DataType type) const
        {
            return type == True || type == False;
        }

        std::error_code peek_type(DataType *type) const
        {
            const uint8_t *ptr = data_pointer;
            std::error_code ec{};

            char c;
            do
            {
                ec = safe_peek(ptr, (uint8_t &)c);
                if (ec == UnpackerError::OutOfRange)
                    break;

                switch (c)
                {
                case '[':
                    *type = Array;
                    return ec;
                case ']':
                    *type = ArrayEnd;
                    return ec;
                case '{':
                    *type = Object;
                    return ec;
                case '}':
                    *type = ObjectEnd;
                    return ec;
                case '"':
                    *type = Str;
                    return ec;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '-':
                case '.':
                    *type = Int;
                    return ec;
                case 't':
                    if (ptr + 3 < data_end && ptr[1] == 'r' && ptr[2] == 'u' && ptr[3] == 'e')
                    {
                        *type = True;
                        return ec;
                    }
                    break;
                case 'f':
                    if (ptr + 4 < data_end && ptr[1] == 'a' && ptr[2] == 'l' && ptr[3] == 's' && ptr[4] == 'e')
                    {
                        *type = False;
                        return ec;
                    }
                    break;
                case 'n':
                    if (ptr + 3 < data_end && ptr[1] == 'u' && ptr[2] == 'l' && ptr[3] == 'l')
                    {
                        *type = Null;
                        return ec;
                    }
                    break;
                default:
                    break;
                }

                ec = safe_peek_increment(ptr);
            } while (ec != UnpackerError::OutOfRange);

            return ec;
        }

        std::error_code ec{};

    private:
        const uint8_t *data_pointer;
        const uint8_t *data_end;

        uint8_t safe_data()
        {
            if (data_pointer < data_end)
                return *data_pointer;
            ec = UnpackerError::OutOfRange;
            return 0;
        }

        std::error_code safe_peek(const uint8_t *ptr, uint8_t &data) const
        {
            if (ptr < data_end)
            {
                data = *ptr;
                return {};
            }
            else
            {
                return UnpackerError::OutOfRange;
            }
        }

        void safe_increment(int64_t bytes = 1)
        {
            if (data_end - data_pointer >= 0)
            {
                data_pointer += bytes;
            }
            else
            {
                ec = UnpackerError::OutOfRange;
            }
        }

        std::error_code safe_peek_increment(const uint8_t *&ptr, int64_t bytes = 1) const
        {
            if (data_end - ptr >= 0)
            {
                ptr += bytes;
                return {};
            }
            else
            {
                return UnpackerError::OutOfRange;
            }
        }

        inline void safe_find_after(char ch)
        {
            char c;
            do
            {
                c = safe_data();
                safe_increment();
            } while (ec != UnpackerError::OutOfRange && c != ch);
        }

        inline void safe_find(char ch)
        {
            char c;
            do
            {
                c = safe_data();
                if (c == ch)
                    break;
                safe_increment();
            } while (ec != UnpackerError::OutOfRange);
        }

        template <CharType... Matches>
        inline void safe_find(Matches... match)
        {
            ec = safe_peek_find(data_pointer, match...);
        }

        inline std::error_code safe_peek_find_after(char ch, const uint8_t **data_pointer) const
        {
            std::error_code ec{};
            *data_pointer = this->data_pointer;
            char c;
            do
            {
                ec = safe_peek(*data_pointer, (uint8_t &)c);
                if (ec == UnpackerError::OutOfRange)
                    break;
                ec = safe_peek_increment(*data_pointer);
            } while (ec != UnpackerError::OutOfRange && c != ch);
            return ec;
        }

        inline std::error_code safe_peek_find(char ch, const uint8_t *&data_pointer) const
        {
            std::error_code ec{};
            char c;
            do
            {
                ec = safe_peek(data_pointer, (uint8_t &)c);
                if (ec == UnpackerError::OutOfRange || c == ch)
                    break;
                ec = safe_peek_increment(data_pointer);
            } while (ec != UnpackerError::OutOfRange);
            return ec;
        }

        template <CharType... Matches>
        inline std::error_code safe_peek_find(const uint8_t *&data_pointer, Matches... match) const
        {
            std::error_code ec{};
            char c;
            do
            {
                ec = safe_peek(data_pointer, (uint8_t &)c);
                if (ec == UnpackerError::OutOfRange)
                    break;

                // test each match
                for (const auto p : {match...})
                {
                    if (c == p)
                        return ec;
                }

                ec = safe_peek_increment(data_pointer);
            } while (ec != UnpackerError::OutOfRange);
            return ec;
        }
    };
};

#endif