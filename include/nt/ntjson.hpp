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
            safe_increment();

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
            const uint8_t *true_ptr;
            auto true_ec = safe_peek_find("true"sv, &true_ptr);
            if (true_ec == UnpackerError::OutOfRange)
                true_ptr = data_end;

            const uint8_t *false_ptr;
            auto false_ec = safe_peek_find("false"sv, &false_ptr);
            if (false_ec == UnpackerError::OutOfRange)
                false_ptr = data_end;

            if (true_ptr < false_ptr)
                return true;
            else
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
            const uint8_t *array_ptr;
            auto array_ec = safe_peek_find('[', &array_ptr);
            if (array_ec == UnpackerError::OutOfRange)
                array_ptr = data_end;
            std::pair<const uint8_t *, DataType> array = {array_ptr, Array};

            const uint8_t *array_end_ptr;
            auto array_end_ec = safe_peek_find(']', &array_end_ptr);
            if (array_end_ec == UnpackerError::OutOfRange)
                array_end_ptr = data_end;
            std::pair<const uint8_t *, DataType> array_end = {array_end_ptr, ArrayEnd};

            const uint8_t *object_ptr;
            auto object_ec = safe_peek_find('{', &object_ptr);
            if (object_ec == UnpackerError::OutOfRange)
                object_ptr = data_end;
            std::pair<const uint8_t *, DataType> object = {object_ptr, Object};

            const uint8_t *object_end_ptr;
            auto object_end_ec = safe_peek_find('}', &object_end_ptr);
            if (object_end_ec == UnpackerError::OutOfRange)
                object_end_ptr = data_end;
            std::pair<const uint8_t *, DataType> object_end = {object_end_ptr, ObjectEnd};

            const uint8_t *str_ptr;
            auto str_ec = safe_peek_find('"', &str_ptr);
            if (str_ec == UnpackerError::OutOfRange)
                str_ptr = data_end;
            std::pair<const uint8_t *, DataType> str = {str_ptr, Str};

            const uint8_t *true_ptr;
            auto true_ec = safe_peek_find("true"sv, &true_ptr);
            if (true_ec == UnpackerError::OutOfRange)
                true_ptr = data_end;
            std::pair<const uint8_t *, DataType> trueType = {true_ptr, True};

            const uint8_t *false_ptr;
            auto false_ec = safe_peek_find("false"sv, &false_ptr);
            if (false_ec == UnpackerError::OutOfRange)
                false_ptr = data_end;
            std::pair<const uint8_t *, DataType> falseType = {false_ptr, False};

            const uint8_t *int_ptr;
            auto int_ec = safe_peek_find(&int_ptr, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '.');
            if (int_ec == UnpackerError::OutOfRange)
                int_ptr = data_end;
            std::pair<const uint8_t *, DataType> intType = {int_ptr, Int};

            const uint8_t *null_ptr;
            auto null_ec = safe_peek_find("null"sv, &null_ptr);
            if (null_ec == UnpackerError::OutOfRange)
                null_ptr = data_end;
            std::pair<const uint8_t *, DataType> nullType = {null_ptr, Null};

            auto closest = std::min({array, array_end, object, object_end, str, trueType, falseType, intType, nullType}, [](auto &a, auto &b)
                                    { return a.first < b.first; });
            *type = closest.second;
            return closest.first == data_end ? UnpackerError::OutOfRange : std::error_code{};
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

        std::string_view safe_view(std::size_t len)
        {
            if (data_pointer + len <= data_end)
                return std::string_view((const char *)data_pointer, len);
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

        std::error_code safe_peek_view(const uint8_t *ptr, std::string_view &view, std::size_t len) const
        {
            if (ptr + len <= data_end)
            {
                view = std::string_view((const char *)ptr, len);
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
            } while (ec != UnpackerError::OutOfRange && c != ch);
        }

        inline void safe_find(std::string_view match)
        {
            std::string_view view;
            do
            {
                view = safe_view(match.length());
                if (view == match)
                    break;
                safe_increment();
            } while (ec != UnpackerError::OutOfRange && view != match);
        }

        template <CharType... Matches>
        inline void safe_find(Matches... match)
        {
            ec = safe_peek_find(&data_pointer, match...);
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

        inline std::error_code safe_peek_find(char ch, const uint8_t **data_pointer) const
        {
            std::error_code ec{};
            *data_pointer = this->data_pointer;
            char c;
            do
            {
                ec = safe_peek(*data_pointer, (uint8_t &)c);
                if (ec == UnpackerError::OutOfRange || c == ch)
                    break;
                ec = safe_peek_increment(*data_pointer);
            } while (ec != UnpackerError::OutOfRange && c != ch);
            return ec;
        }

        inline std::error_code safe_peek_find(std::string_view match, const uint8_t **data_pointer) const
        {
            std::error_code ec{};
            *data_pointer = this->data_pointer;
            std::string_view view;
            do
            {
                ec = safe_peek_view(*data_pointer, view, match.length());
                if (ec == UnpackerError::OutOfRange || view == match)
                    break;
                ec = safe_peek_increment(*data_pointer);
            } while (ec != UnpackerError::OutOfRange && view != match);
            return ec;
        }

        template <CharType... Matches>
        inline std::error_code safe_peek_find(const uint8_t **data_pointer, Matches... match) const
        {
            *data_pointer = this->data_end;
            ([&]
             {
                const uint8_t *ptr = *data_pointer;
                std::error_code ec = safe_peek_find(match, &ptr);
                if (ec != UnpackerError::OutOfRange)
                {
                    *data_pointer = std::min(*data_pointer, ptr);
                } }(),
             ...);
            return (*data_pointer == this->data_end) ? (UnpackerError::OutOfRange) : std::error_code{};
        }
    };
};

#endif