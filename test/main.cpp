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
        template <class T, class ...Args>
        void run(T test, Args ...args) noexcept
        {
            try
            {
                test(args...);
            }
            catch (const TestError& e)
            {
                std::cerr << e.what() << '\n';
                result = false;
            }
        }

        bool getResult() const noexcept { return result; }

    private:
        bool result = true;
    };

    void testComments()
    {
        xml::Data d = xml::parse("<!--test--><root/>", true, true, true);

        auto first = d.begin();
        if (first == d.end())
            throw TestError("Expected a node");

        xml::Node& node = *first;
        if (node.getType() != xml::Node::Type::Comment)
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

        xml::Node& node = *first;
        if (node.getType() != xml::Node::Type::ProcessingInstruction)
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

        xml::Node& node = *first;
        if (node.getType() != xml::Node::Type::Tag)
            throw TestError("Expected a tag node");

        if (node.getValue() != "root")
            throw TestError("Wrong value");

        auto firstChild = node.begin();
        if (firstChild == node.end())
            throw TestError("Expected a child node");

        xml::Node& child = *firstChild;
        if (child.getType() != xml::Node::Type::Text)
            throw TestError("Expected a text node");

        if (child.getValue() != "text")
            throw TestError("Wrong value");
    }

    void testEncoding()
    {
        xml::Data d;
        xml::Node p(xml::Node::Type::ProcessingInstruction);
        p.setValue("xml");
        p.setAttributes({{"version", "1.0"}, {"encoding", "utf-8"}});
        d.pushBack(p);

        xml::Node n(xml::Node::Type::Tag);
        n.setValue("n");
        n.setAttributes({{"a", "a"}, {"b", "b"}});

        xml::Node c1(xml::Node::Type::Tag);
        c1.setValue("c1");
        c1.setAttributes({{"c", "c"}});

        xml::Node c2(xml::Node::Type::Tag);
        c2.setValue("c2");
        c2.setAttributes({{"dd", "dd"}});

        xml::Node t(xml::Node::Type::Text);
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
}

int main()
{
    TestRunner testRunner;
    testRunner.run(testComments);
    testRunner.run(testProcessingInstruction);
    testRunner.run(testText);
    testRunner.run(testEncoding);

    if (testRunner.getResult())
        std::cout << "Success\n";

    return testRunner.getResult() ? EXIT_SUCCESS : EXIT_FAILURE;
}
