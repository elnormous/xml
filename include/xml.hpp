// Copyright 2015-2020 Elviss Strazdins. All rights reserved.

#ifndef XML_HPP
#define XML_HPP

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace xml
{
    class ParseError final: public std::logic_error
    {
    public:
        explicit ParseError(const std::string& str): std::logic_error(str) {}
        explicit ParseError(const char* str): std::logic_error(str) {}
    };

    class RangeError final: public std::runtime_error
    {
    public:
        explicit RangeError(const std::string& str): std::runtime_error(str) {}
        explicit RangeError(const char* str): std::runtime_error(str) {}
    };

    namespace utf8
    {
        template <class Iterator>
        std::u32string toUtf32(Iterator begin, Iterator end)
        {
            std::u32string result;

            for (auto i = begin; i != end; ++i)
            {
                char32_t cp = *i & 0xFF;

                if (cp <= 0x7F) // length = 1
                {
                    // do nothing
                }
                else if ((cp >> 5) == 0x6) // length = 2
                {
                    if (++i == end)
                        throw ParseError("Invalid UTF-8 string");
                    cp = ((cp << 6) & 0x7FF) + (*i & 0x3F);
                }
                else if ((cp >> 4) == 0xE) // length = 3
                {
                    if (++i == end)
                        throw ParseError("Invalid UTF-8 string");
                    cp = ((cp << 12) & 0xFFFF) + (((*i & 0xFF) << 6) & 0x0FFF);
                    if (++i == end)
                        throw ParseError("Invalid UTF-8 string");
                    cp += *i & 0x3F;
                }
                else if ((cp >> 3) == 0x1E) // length = 4
                {
                    if (++i == end)
                        throw ParseError("Invalid UTF-8 string");
                    cp = ((cp << 18) & 0x1FFFFF) + (((*i & 0xFF) << 12) & 0x3FFFF);
                    if (++i == end)
                        throw ParseError("Invalid UTF-8 string");
                    cp += ((*i & 0xFF) << 6) & 0x0FFF;
                    if (++i == end)
                        throw ParseError("Invalid UTF-8 string");
                    cp += *i & 0x3F;
                }

                result.push_back(cp);
            }

            return result;
        }

        template <class T>
        std::u32string toUtf32(const T& text)
        {
            return toUtf32(std::begin(text), std::end(text));
        }

        inline std::string fromUtf32(char32_t c)
        {
            std::string result;

            if (c <= 0x7F)
                result.push_back(static_cast<char>(c));
            else if (c <= 0x7FF)
            {
                result.push_back(static_cast<char>(0xC0 | ((c >> 6) & 0x1F)));
                result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
            }
            else if (c <= 0xFFFF)
            {
                result.push_back(static_cast<char>(0xE0 | ((c >> 12) & 0x0F)));
                result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
            }
            else
            {
                result.push_back(static_cast<char>(0xF0 | ((c >> 18) & 0x07)));
                result.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
            }

            return result;
        }

        template <class Iterator>
        std::string fromUtf32(Iterator begin, Iterator end)
        {
            std::string result;

            for (auto i = begin; i != end; ++i)
            {
                if (*i <= 0x7F)
                    result.push_back(static_cast<char>(*i));
                else if (*i <= 0x7FF)
                {
                    result.push_back(static_cast<char>(0xC0 | ((*i >> 6) & 0x1F)));
                    result.push_back(static_cast<char>(0x80 | (*i & 0x3F)));
                }
                else if (*i <= 0xFFFF)
                {
                    result.push_back(static_cast<char>(0xE0 | ((*i >> 12) & 0x0F)));
                    result.push_back(static_cast<char>(0x80 | ((*i >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (*i & 0x3F)));
                }
                else
                {
                    result.push_back(static_cast<char>(0xF0 | ((*i >> 18) & 0x07)));
                    result.push_back(static_cast<char>(0x80 | ((*i >> 12) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | ((*i >> 6) & 0x3F)));
                    result.push_back(static_cast<char>(0x80 | (*i & 0x3F)));
                }
            }

            return result;
        }

        template <class T>
        std::string fromUtf32(const T& text)
        {
            return fromUtf32(std::begin(text), std::end(text));
        }
    } // namespace utf8

    class Node final
    {
    public:
        enum class Type
        {
            comment,
            characterData,
            typeDeclaration,
            processingInstruction,
            tag,
            text
        };

        Node() = default;
        Node(Type initType): type(initType) {}
        Node(const std::string& val): type(Type::text), value(val) {}

        Node& operator=(Type newType) noexcept
        {
            type = newType;
            return *this;
        }

        Node& operator=(const std::string& val)
        {
            type = Type::text;
            value = val;
            return *this;
        }

        Type getType() const noexcept { return type; }
        void setType(Type newType) noexcept { type = newType; }

        std::vector<Node>::iterator begin() noexcept
        {
            return children.begin();
        }

        std::vector<Node>::iterator end() noexcept
        {
            return children.end();
        }

        std::vector<Node>::const_iterator begin() const noexcept
        {
            return children.begin();
        }
        std::vector<Node>::const_iterator end() const noexcept
        {
            return children.end();
        }

        const std::string& operator[](const std::string& attribute) const
        {
            auto i = attributes.find(attribute);
            if (i == attributes.end())
                throw RangeError("Invalid attribute");
            return i->second;
        }
        std::string& operator[](const std::string& attribute) noexcept { return attributes[attribute]; }

        const std::vector<Node>& getChildren() const noexcept { return children; }
        void pushBack(const Node& node) { children.push_back(node); }

        const std::string& getValue() const noexcept { return value; }
        void setValue(const std::string& newValue) { value = newValue; }

        const std::map<std::string, std::string>& getAttributes() const noexcept { return attributes; }
        void setAttributes(const std::map<std::string, std::string>& newAttributes) { attributes = newAttributes; }

    private:
        Type type;
        std::string value;
        std::map<std::string, std::string> attributes;
        std::vector<Node> children;
    };

    class Data final
    {
    public:
        Data() = default;

        std::vector<Node>::iterator begin() noexcept
        {
            return children.begin();
        }

        std::vector<Node>::iterator end() noexcept
        {
            return children.end();
        }

        std::vector<Node>::const_iterator begin() const noexcept
        {
            return children.begin();
        }

        std::vector<Node>::const_iterator end() const noexcept
        {
            return children.end();
        }

        const std::vector<Node>& getChildren() const noexcept { return children; }
        void pushBack(const Node& node) { children.push_back(node); }

    private:
        std::vector<Node> children;
    };

    inline namespace detail
    {
        constexpr std::uint8_t utf8ByteOrderMark[] = {0xEF, 0xBB, 0xBF};
    }

    template <class Iterator>
    Data parse(Iterator begin, Iterator end,
               bool preserveWhitespaces = false,
               bool preserveComments = false,
               bool preserveProcessingInstructions = false)
    {
        class Parser final
        {
        public:
            static Data parse(Iterator begin, Iterator end,
                              bool preserveWhitespaces,
                              bool preserveComments,
                              bool preserveProcessingInstructions)
            {
                bool byteOrderMark = hasByteOrderMark(begin, end);

                const std::u32string str = utf8::toUtf32(byteOrderMark ? begin + 3 : begin, end);
                auto iterator = str.begin();
                bool rootTagFound = false;

                Data result;

                for (;;)
                {
                    if (!preserveWhitespaces) skipWhitespaces(iterator, str.end());

                    if (iterator == str.end()) break;

                    Node node = parse(iterator, str.end(),
                                      preserveWhitespaces,
                                      preserveComments,
                                      preserveProcessingInstructions);

                    if ((preserveComments || node.getType() != Node::Type::comment) &&
                        (preserveProcessingInstructions || node.getType() != Node::Type::processingInstruction))
                    {
                        result.pushBack(node);

                        if (node.getType() == Node::Type::tag)
                        {
                            if (rootTagFound)
                                throw ParseError("Multiple root tags found");
                            else
                                rootTagFound = true;
                        }
                    }
                }

                if (!rootTagFound)
                    throw ParseError("No root tag found");

                return result;
            }

        private:
            static bool hasByteOrderMark(Iterator begin, Iterator end) noexcept
            {
                for (auto i = std::begin(utf8ByteOrderMark); i != std::end(utf8ByteOrderMark); ++i, ++begin)
                    if (begin == end || static_cast<std::uint8_t>(*begin) != *i)
                        return false;
                return true;
            }

            static constexpr bool isWhitespace(const char32_t c) noexcept
            {
                return c == ' ' || c == '\t' || c == '\r' || c == '\n';
            }

            static constexpr bool isNameStartChar(const char32_t c) noexcept
            {
                return (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    c == ':' || c == '_' ||
                    (c >= 0xC0 && c <= 0xD6) ||
                    (c >= 0xD8 && c <= 0xF6) ||
                    (c >= 0xF8 && c <= 0x2FF) ||
                    (c >= 0x370 && c <= 0x37D) ||
                    (c >= 0x37F && c <= 0x1FFF) ||
                    (c >= 0x200C && c <= 0x200D) ||
                    (c >= 0x2070 && c <= 0x218F);
            }

            static constexpr bool isNameChar(const char32_t c) noexcept
            {
                return isNameStartChar(c) ||
                    c == '-' || c == '.' ||
                    (c >= '0' && c <= '9') ||
                    c == 0xB7 ||
                    (c >= 0x0300 && c <= 0x036F) ||
                    (c >= 0x203F && c <= 0x2040);
            }

            static void skipWhitespaces(std::u32string::const_iterator& iterator,
                                        std::u32string::const_iterator end)
            {
                while (iterator != end)
                    if (isWhitespace(*iterator))
                        ++iterator;
                    else
                        break;
            }

            static std::string parseName(std::u32string::const_iterator& iterator,
                                         std::u32string::const_iterator end)
            {
                std::string result;

                if (iterator == end)
                    throw ParseError("Unexpected end of data");

                if (!isNameStartChar(*iterator))
                    throw ParseError("Invalid name start");

                for (;;)
                {
                    if (iterator == end)
                        throw ParseError("Unexpected end of data");

                    if (!isNameChar(*iterator))
                        break;
                    else
                    {
                        result += utf8::fromUtf32(*iterator);
                        ++iterator;
                    }
                }

                return result;
            }

            static std::string parseEntity(std::u32string::const_iterator& iterator,
                                           std::u32string::const_iterator end)
            {
                std::string result;

                if (iterator == end)
                    throw ParseError("Unexpected end of data");

                if (*iterator != '&')
                    throw ParseError("Expected an ampersand");

                std::string value;

                for (;;)
                {
                    if (++iterator == end)
                        throw ParseError("Unexpected end of data");

                    if (*iterator == ';')
                    {
                        ++iterator;
                        break;
                    }
                    else
                        value.push_back(static_cast<char>(*iterator));
                }

                if (value.empty())
                    throw ParseError("Invalid entity");

                if (value == "quot")
                    result = "\"";
                else if (value == "amp")
                    result = "&";
                else if (value == "apos")
                    result = "'";
                else if (value == "lt")
                    result = "<";
                else if (value == "gt")
                    result = ">";
                else if (value[0] == '#')
                {
                    if (value.length() < 2)
                        throw ParseError("Invalid entity");

                    char32_t c = 0;

                    if (value[1] == 'x') // hex value
                    {
                        if (value.length() != 2 + 4)
                            throw ParseError("Invalid entity");

                        for (std::size_t i = 0; i < 4; ++i)
                        {
                            std::uint8_t code = 0;

                            if (value[i + 2] >= '0' && value[i + 2] <= '9')
                                code = static_cast<std::uint8_t>(value[i + 2]) - '0';
                            else if (value[i + 2] >= 'a' && value[i + 2] <='f')
                                code = static_cast<std::uint8_t>(value[i + 2]) - 'a' + 10;
                            else if (value[i + 2] >= 'A' && value[i + 2] <='F')
                                code = static_cast<std::uint8_t>(value[i + 2]) - 'A' + 10;
                            else
                                throw ParseError("Invalid character code");

                            c = (c << 4) | code;
                        }
                    }
                    else
                    {
                        if (value.length() != 1 + 4)
                            throw ParseError("Invalid entity");

                        for (std::size_t i = 0; i < 4; ++i)
                        {
                            std::uint8_t code = 0;

                            if (value[i + 1] >= '0' && value[i + 1] <= '9')
                                code = static_cast<std::uint8_t>(value[i + 1]) - '0';
                            else
                                throw ParseError("Invalid character code");

                            c = c * 10 + code;
                        }
                    }

                    result = utf8::fromUtf32(c);
                }
                else
                    throw ParseError("Invalid entity");

                return result;
            }

            static std::string parseString(std::u32string::const_iterator& iterator,
                                           std::u32string::const_iterator end)
            {
                std::string result;

                if (iterator == end)
                    throw ParseError("Unexpected end of data");

                if (*iterator != '"' && *iterator != '\'')
                    throw ParseError("Expected quotes");

                auto quotes = *iterator;

                ++iterator;

                for (;;)
                {
                    if (iterator == end)
                        throw ParseError("Unexpected end of data");

                    if (*iterator == quotes)
                    {
                        ++iterator;
                        break;
                    }
                    else if (*iterator == '&')
                    {
                        std::string entity = parseEntity(iterator, end);
                        result += entity;
                    }
                    else
                    {
                        result += utf8::fromUtf32(*iterator);
                        ++iterator;
                    }
                }

                return result;
            }

            static Node parse(std::u32string::const_iterator& iterator,
                              std::u32string::const_iterator end,
                              bool preserveWhitespaces,
                              bool preserveComments,
                              bool preserveProcessingInstructions)
            {
                Node result;

                if (iterator == end)
                    throw ParseError("Unexpected end of data");

                if (*iterator == '<')
                {
                    if (++iterator == end)
                        throw ParseError("Unexpected end of data");

                    if (*iterator == '!') // <!
                    {
                        if (++iterator == end)
                            throw ParseError("Unexpected end of data");

                        if (*iterator == '-') // <!-
                        {
                            if (++iterator == end)
                                throw ParseError("Unexpected end of data");

                            if (*iterator != '-') // <!--
                                throw ParseError("Expected a comment");

                            result = Node::Type::comment;

                            std::string value;
                            for (;;)
                            {
                                if (std::distance(++iterator, end) < 3)
                                    throw ParseError("Unexpected end of data");

                                if (*iterator == '-')
                                {
                                    if (*(iterator + 1) == '-') // --
                                    {
                                        iterator += 2;

                                        if (*iterator == '>') // -->
                                        {
                                            ++iterator;
                                            break;
                                        }
                                        else
                                            throw ParseError("Unexpected double-hyphen inside comment");
                                    }
                                }

                                value += utf8::fromUtf32(*iterator);
                            }

                            result.setValue(value);
                        }
                        else if (*iterator == '[') // <![
                        {
                            ++iterator;
                            std::string name;
                            name = parseName(iterator, end);

                            if (name != "CDATA")
                                throw ParseError("Expected CDATA");

                            if (iterator == end)
                                throw ParseError("Unexpected end of data");

                            if (*iterator != '[')
                                throw ParseError("Expected a left bracket");

                            result = Node::Type::characterData;

                            std::string value;
                            for (;;)
                            {
                                if (std::distance(++iterator, end) < 3)
                                    throw ParseError("Unexpected end of data");

                                if (*iterator == ']' &&
                                    *(iterator + 1) == ']' &&
                                    *(iterator + 2) == '>')
                                {
                                    iterator += 3;
                                    break;
                                }

                                value += utf8::fromUtf32(*iterator);
                            }
                            result.setValue(value);
                        }
                        else
                            throw ParseError("Type declarations are not supported");
                    }
                    else if (*iterator == '?') // <?
                    {
                        ++iterator;
                        result = Node::Type::processingInstruction;
                        result.setValue(parseName(iterator, end));

                        for (;;)
                        {
                            skipWhitespaces(iterator, end);

                            if (iterator == end)
                                throw ParseError("Unexpected end of data");

                            if (*iterator == '?')
                            {
                                if (++iterator == end)
                                    throw ParseError("Unexpected end of data");

                                if (*iterator != '>') // ?>
                                    throw ParseError("Expected a right angle bracket");

                                ++iterator;
                                break;
                            }

                            std::string attribute = parseName(iterator, end);

                            skipWhitespaces(iterator, end);

                            if (iterator == end)
                                throw ParseError("Unexpected end of data");

                            if (*iterator != '=')
                                throw ParseError("Expected an equal sign");

                            ++iterator;

                            skipWhitespaces(iterator, end);
                            result[attribute] = parseString(iterator, end);
                        }
                    }
                    else // <
                    {
                        result = Node::Type::tag;
                        result.setValue(parseName(iterator, end));

                        bool tagClosed = false;

                        for (;;)
                        {
                            skipWhitespaces(iterator, end);

                            if (iterator == end)
                                throw ParseError("Unexpected end of data");

                            if (*iterator == '>')
                            {
                                ++iterator;
                                break;
                            }
                            else if (*iterator == '/')
                            {
                                if (++iterator == end)
                                    throw ParseError("Unexpected end of data");

                                if (*iterator != '>') // />
                                    throw ParseError("Expected a right angle bracket");

                                tagClosed = true;
                                ++iterator;
                                break;
                            }

                            std::string attribute;
                            attribute = parseName(iterator, end);

                            skipWhitespaces(iterator, end);

                            if (iterator == end)
                                throw ParseError("Unexpected end of data");

                            if (*iterator != '=')
                                throw ParseError("Expected an equal sign");

                            ++iterator;

                            skipWhitespaces(iterator, end);

                            result[attribute] = parseString(iterator, end);
                        }

                        if (!tagClosed)
                        {
                            for (;;)
                            {
                                if (!preserveWhitespaces) skipWhitespaces(iterator, end);

                                if (iterator == end)
                                    throw ParseError("Unexpected end of data");

                                if (*iterator == '<' &&
                                    iterator + 1 != end &&
                                    *(iterator + 1) == '/')
                                {
                                    ++iterator; // skip the left angle bracket
                                    ++iterator; // skip the slash

                                    std::string tag = parseName(iterator, end);

                                    if (tag != result.getValue())
                                        throw ParseError("Tag not closed properly");

                                    if (iterator == end)
                                        throw ParseError("Unexpected end of data");

                                    if (*iterator != '>')
                                        throw ParseError("Expected a right angle bracket");

                                    ++iterator;

                                    break;
                                }
                                else
                                {
                                    Node node = parse(iterator, end, preserveWhitespaces, preserveComments, preserveProcessingInstructions);

                                    if ((preserveComments || node.getType() != Node::Type::comment) &&
                                        (preserveProcessingInstructions || node.getType() != Node::Type::processingInstruction))
                                        result.pushBack(node);
                                }
                            }
                        }
                    }
                }
                else
                {
                    result = Node::Type::text;

                    std::string value;
                    for (;;)
                    {
                        if (iterator == end || // end of a file
                            *iterator == '<') // start of a tag
                            break;
                        else if (*iterator == '&')
                        {
                            std::string entity = parseEntity(iterator, end);
                            value += entity;
                        }
                        else
                        {
                            value += utf8::fromUtf32(*iterator);
                            ++iterator;
                        }
                    }
                    result.setValue(value);
                }

                return result;
            }
        };

        return Parser::parse(begin, end,
                             preserveWhitespaces,
                             preserveComments,
                             preserveProcessingInstructions);
    }

    inline Data parse(const char* data,
                      bool preserveWhitespaces = false,
                      bool preserveComments = false,
                      bool preserveProcessingInstructions = false)
    {
        const char* end = data;
        while (*end) ++end;
        return parse(data, end,
                     preserveWhitespaces,
                     preserveComments,
                     preserveProcessingInstructions);
    }

    template <class T>
    Data parse(const T& data)
    {
        return parse(std::begin(data), std::end(data));
    }

    inline std::string encode(const Data& data, bool whitespaces = false, bool byteOrderMark = false)
    {
        class Encoder final
        {
        public:
            static std::string encode(const Data& data, bool whitespaces, bool byteOrderMark)
            {
                std::string result;
                if (byteOrderMark) result.assign(std::begin(utf8ByteOrderMark),
                                                 std::end(utf8ByteOrderMark));

                for (const Node& node : data)
                {
                    encode(node, result, whitespaces);
                    if (whitespaces) result.push_back('\n');
                }

                return result;
            }

        private:
            static void encode(const std::string& str,
                               std::string& result)
            {
                for (const char c : str)
                {
                    switch (c)
                    {
                        case '"':
                            result.insert(result.end(), {'&', 'q', 'u', 'o', 't', ';'});
                            break;
                        case '&':
                            result.insert(result.end(), {'&', 'a', 'm', 'p', ';'});
                            break;
                        case '\'':
                            result.insert(result.end(), {'&', 'a', 'p', 'o', 's', ';'});
                            break;
                        case '<':
                            result.insert(result.end(), {'&', 'l', 't', ';'});
                            break;
                        case '>':
                            result.insert(result.end(), {'&', 'g', 't', ';'});
                            break;
                        default:
                            result.insert(result.end(), c);
                            break;
                    }
                }
            }

            static void encode(const Node& node, std::string& result, bool whitespaces, size_t level = 0)
            {
                switch (node.getType())
                {
                    case Node::Type::comment:
                    {
                        const auto& value = node.getValue();
                        result.insert(result.end(), {'<', '!', '-', '-'});
                        result.insert(result.end(), value.begin(), value.end());
                        result.insert(result.end(), {'-', '-', '>'});
                        break;
                    }
                    case Node::Type::characterData:
                    {
                        const auto& value = node.getValue();
                        result.insert(result.end(), {'<', '!', '[', 'C', 'D', 'A', 'T', 'A', '['});
                        result.insert(result.end(), value.begin(), value.end());
                        result.insert(result.end(), {']', ']', '>'});
                        break;
                    }
                    case Node::Type::typeDeclaration:
                        throw ParseError("Type declarations are not supported");
                    case Node::Type::processingInstruction:
                    {
                        const auto& value = node.getValue();
                        result.insert(result.end(), {'<', '?'});
                        result.insert(result.end(), value.begin(), value.end());

                        const auto& attributes = node.getAttributes();
                        for (const auto& attribute : attributes)
                        {
                            result.insert(result.end(), ' ');
                            result.insert(result.end(), attribute.first.begin(), attribute.first.end());
                            result.insert(result.end(), {'=', '"'});
                            encode(attribute.second, result);
                            result.insert(result.end(), '"');
                        }

                        result.insert(result.end(), {'?', '>'});
                        break;
                    }
                    case Node::Type::tag:
                    {
                        const auto& value = node.getValue();
                        result.insert(result.end(), '<');
                        result.insert(result.end(), value.begin(), value.end());

                        const auto& attributes = node.getAttributes();
                        for (const auto& attribute : attributes)
                        {
                            result.insert(result.end(), ' ');
                            result.insert(result.end(), attribute.first.begin(), attribute.first.end());
                            result.insert(result.end(), {'=', '"'});
                            result.insert(result.end(), attribute.second.begin(), attribute.second.end());
                            result.insert(result.end(), '"');
                        }

                        const auto& children = node.getChildren();
                        if (children.empty())
                            result.insert(result.end(), {'/', '>'});
                        else
                        {
                            result.insert(result.end(), '>');
                            if (whitespaces) result.push_back('\n');

                            for (const Node& child : children)
                            {
                                if (whitespaces) result.insert(result.end(), level + 1, '\t');
                                encode(child, result, whitespaces, level + 1);
                                if (whitespaces) result.push_back('\n');
                            }

                            if (whitespaces) result.insert(result.end(), level, '\t');
                            result.insert(result.end(), {'<', '/'});
                            result.insert(result.end(), value.begin(), value.end());
                            result.insert(result.end(), '>');
                        }
                        break;
                    }
                    case Node::Type::text:
                    {
                        const auto& value = node.getValue();
                        encode(value, result);
                        break;
                    }
                    default:
                        throw ParseError("Unknown node type");
                }
            }
        };

        return Encoder::encode(data, whitespaces, byteOrderMark);
    }
} // namespace xml

#endif // XML_HPP
