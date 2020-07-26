#include <chrono>
#include <iostream>
#include "xml.hpp"

namespace
{
    class TestError final: public std::logic_error
    {
    public:
        explicit TestError(const std::string& str): std::logic_error(str) {}
        explicit TestError(const char* str): std::logic_error(str) {}
    };

    class TestRunner final
    {
    public:
        TestRunner(int argc, char** argv) noexcept:
            argumentCount(argc), arguments(argv)
        {
        }

        TestRunner(const TestRunner&) = delete;
        TestRunner& operator=(const TestRunner&) = delete;
        ~TestRunner()
        {
            if (result)
                std::cout << "Success, total duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms\n";
        }

        template <class T, class ...Args>
        void run(const std::string& name, T test, Args ...args) noexcept
        {
            for (int i = 1; i < argumentCount; ++i)
                if (name == arguments[i]) break;
                else if (i == argumentCount - 1) return;

            try
            {
                std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
                test(args...);
                std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();

                duration += finish - start;

                std::cerr << name << " succeeded, duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "ms\n";
            }
            catch (const TestError& e)
            {
                std::cerr << name << " failed: " << e.what() << '\n';
                result = false;
            }
        }

        bool getResult() const noexcept { return result; }
        std::chrono::steady_clock::duration getDuration() const noexcept { return duration; }

    private:
        int argumentCount;
        char** arguments;
        bool result = true;
        std::chrono::steady_clock::duration duration = std::chrono::milliseconds(0);
    };

    void testComments()
    {
        xml::Data d = xml::parse("<!--test--><root/>", true, true, true);

        auto first = d.begin();
        if (first == d.end())
            throw TestError("Expected a node");

        auto& node = *first;
        if (node.getType() != xml::Node::Type::comment)
            throw TestError("Expected a comment node");

        if (node.getValue() != "test")
            throw TestError("Wrong value");
    }

    void testProcessingInstruction()
    {
        xml::Data d = xml::parse("<?xml version=\"1.0\"?><root/>", true, true, true);

        auto first = d.begin();
        if (first == d.end())
            throw TestError("Expected a node");

        auto& node = *first;
        if (node.getType() != xml::Node::Type::processingInstruction)
            throw TestError("Expected a processing instruction node");

        if (node.getValue() != "xml")
            throw TestError("Wrong value");

        if (node["version"] != "1.0")
            throw TestError("Wrong attribute");
    }

    void testText()
    {
        xml::Data d = xml::parse("<root>text</root>", true, true, true);

        auto first = d.begin();
        if (first == d.end())
            throw TestError("Expected a node");

        auto& node = *first;
        if (node.getType() != xml::Node::Type::tag)
            throw TestError("Expected a tag node");

        if (node.getValue() != "root")
            throw TestError("Wrong value");

        auto firstChild = node.begin();
        if (firstChild == node.end())
            throw TestError("Expected a child node");

        auto& child = *firstChild;
        if (child.getType() != xml::Node::Type::text)
            throw TestError("Expected a text node");

        if (child.getValue() != "text")
            throw TestError("Wrong value");
    }

    void testEntities()
    {
        xml::Data d = xml::parse("<root test=\"&lt;\">&amp;</root>", true, true, true);

        auto first = d.begin();
        if (first == d.end())
            throw TestError("Expected a node");

        auto& node = *first;
        if (node.getType() != xml::Node::Type::tag)
            throw TestError("Expected a tag node");

        if (node.getValue() != "root")
            throw TestError("Wrong value");

        auto firstAttribute = node.getAttributes().begin();
        if (firstAttribute == node.getAttributes().end())
            throw TestError("Expected an attribute");

        auto& attribute = *firstAttribute;
        if (attribute.first != "test")
            throw TestError("Wrong attribute name");

        if (attribute.second != "<")
            throw TestError("Wrong attribute value");

        auto firstChild = node.begin();
        if (firstChild == node.end())
            throw TestError("Expected a child node");

        auto& child = *firstChild;
        if (child.getType() != xml::Node::Type::text)
            throw TestError("Expected a text node");

        if (child.getValue() != "&")
            throw TestError("Wrong value");
    }

    void testEncoding()
    {
        xml::Data d;
        xml::Node p(xml::Node::Type::processingInstruction);
        p.setValue("xml");
        p.setAttributes({{"version", "1.0"}, {"encoding", "utf-8"}});
        d.pushBack(p);

        xml::Node n(xml::Node::Type::tag);
        n.setValue("n");
        n.setAttributes({{"a", "a"}, {"b", "b"}});

        xml::Node c1(xml::Node::Type::tag);
        c1.setValue("c1");
        c1.setAttributes({{"c", "c"}});

        xml::Node c2(xml::Node::Type::tag);
        c2.setValue("c2");
        c2.setAttributes({{"dd", "dd"}});

        xml::Node t(xml::Node::Type::text);
        t.setValue("text");

        c1.pushBack(t);
        n.pushBack(c1);
        n.pushBack(c2);
        d.pushBack(n);

        if (xml::encode(d) != "<?xml encoding=\"utf-8\" version=\"1.0\"?><n a=\"a\" b=\"b\"><c1 c=\"c\">text</c1><c2 dd=\"dd\"/></n>")
            throw TestError("Wrong encoded result");

        if (xml::encode(d, true) != "<?xml encoding=\"utf-8\" version=\"1.0\"?>\n<n a=\"a\" b=\"b\">\n\t<c1 c=\"c\">\n\t\ttext\n\t</c1>\n\t<c2 dd=\"dd\"/>\n</n>\n")
            throw TestError("Wrong encoded result");
    }

    enum class byte: unsigned char {};

    void testByte()
    {
        std::vector<byte> data = {
            static_cast<byte>('<'),
            static_cast<byte>('r'),
            static_cast<byte>('/'),
            static_cast<byte>('>')
        };

        xml::Data d = xml::parse(data, true, true, true);

        auto first = d.begin();
        if (first == d.end())
            throw TestError("Expected a node");

        auto& node = *first;
        if (node.getType() != xml::Node::Type::tag)
            throw TestError("Expected a tag node");

        if (node.getValue() != "r")
            throw TestError("Wrong value");
    }
}

int main(int argc, char* argv[])
{
    TestRunner testRunner(argc, argv);
    testRunner.run("testComments", testComments);
    testRunner.run("testProcessingInstruction", testProcessingInstruction);
    testRunner.run("testText", testText);
    testRunner.run("testEntities", testEntities);
    testRunner.run("testEncoding", testEncoding);
    testRunner.run("testByte", testByte);

    return testRunner.getResult() ? EXIT_SUCCESS : EXIT_FAILURE;
}
