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
    testRunner.run(testEncoding);

    if (testRunner.getResult())
        std::cout << "Success\n";

    return testRunner.getResult() ? EXIT_SUCCESS : EXIT_FAILURE;
}
