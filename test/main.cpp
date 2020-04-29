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
        xml::Node n(xml::Node::Type::Tag);
        n.setValue("n");
        n.setAttributes({{"a", "a"}, {"b", "b"}});
        d.pushBack(n);

        if (xml::encode(d) != "<n a=\"a\" b=\"b\"/>")
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
