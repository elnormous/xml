#include <cstddef>
#include <vector>
#include "catch2/catch.hpp"
#include "xml.hpp"

TEST_CASE("Constuctor")
{
    SECTION("String literal")
    {
        const xml::Node node = "test";
        REQUIRE(node.getValue() == "test");
    }

    SECTION("String view")
    {
        const xml::Node node = std::string_view{"test"};
        REQUIRE(node.getValue() == "test");
    }

    SECTION("String")
    {
        const xml::Node node = std::string{"test"};
        REQUIRE(node.getValue() == "test");
    }
}

TEST_CASE("Assignment")
{
    SECTION("String literal")
    {
        xml::Node node;
        node = "test";
        REQUIRE(node.getValue() == "test");
    }

    SECTION("String view")
    {
        xml::Node node;
        node = std::string_view{"test"};
        REQUIRE(node.getValue() == "test");
    }

    SECTION("String")
    {
        xml::Node node;
        node = std::string{"test"};
        REQUIRE(node.getValue() == "test");
    }
}

TEST_CASE("Comments", "[parsing]")
{
    const xml::Data d = xml::parse("<!--test--><root/>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::comment);
    REQUIRE(node.getValue() == "test");
}

TEST_CASE("Empty element", "[parsing]")
{
    const xml::Data d = xml::parse("<root/>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::tag);
    REQUIRE(node.getName() == "root");
}

TEST_CASE("End tag", "[parsing]")
{
    const xml::Data d = xml::parse("<root></root>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::tag);
    REQUIRE(node.getName() == "root");
}

TEST_CASE("XSD", "[parsing]")
{
    const xml::Data d = xml::parse("<xs:schema></xs:schema>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::tag);
    REQUIRE(node.getName() == "xs:schema");
}

TEST_CASE("Processing instruction", "[parsing]")
{
    const xml::Data d = xml::parse("<root><?pi bb?></root>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::tag);
    REQUIRE(node.getName() == "root");

    const auto firstChild = node.begin();
    REQUIRE(firstChild != node.end());

    const auto& child = *firstChild;
    REQUIRE(child.getType() == xml::Node::Type::processingInstruction);
    REQUIRE(child.getName() == "pi");
    REQUIRE(child.getValue() == "bb");
}

TEST_CASE("Prolog", "[parsing]")
{
    const xml::Data d = xml::parse("<?xml version=\"1.0\"?><root/>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::processingInstruction);
    REQUIRE(node.getName() == "xml");
    REQUIRE(node.getValue() == "version=\"1.0\"");
}

TEST_CASE("Text", "[parsing]")
{
    const xml::Data d = xml::parse("<root>text</root>", true, true, true);

    const auto first = d.begin();
    const auto& node = *first;

    const auto firstChild = node.begin();
    REQUIRE(firstChild != node.end());

    const auto& child = *firstChild;
    REQUIRE(child.getType() == xml::Node::Type::text);
    REQUIRE(child.getValue() == "text");
}

TEST_CASE("Attributes", "[parsing]")
{
    const xml::Data d = xml::parse("<root test=\"t\" test2=\"1\"></root>", true, true, true);

    const auto first = d.begin();
    const auto& node = *first;

    const auto firstAttribute = node.getAttributes().begin();
    REQUIRE(firstAttribute != node.getAttributes().end());

    const auto& attribute = *firstAttribute;
    REQUIRE(attribute.first == "test");

    REQUIRE(attribute.second == "t");
}

TEST_CASE("EntityReferences", "[parsing]")
{
    const xml::Data d = xml::parse("<root test=\"&lt;\">&gt;&amp;&apos;&quot;</root>", true, true, true);

    const auto first = d.begin();
    const auto& node = *first;

    const auto firstAttribute = node.getAttributes().begin();
    REQUIRE(firstAttribute != node.getAttributes().end());

    const auto& attribute = *firstAttribute;
    REQUIRE(attribute.first == "test");

    REQUIRE(attribute.second == "<");

    const auto firstChild = node.begin();
    REQUIRE(firstChild != node.end());

    const auto& child = *firstChild;
    REQUIRE(child.getType() == xml::Node::Type::text);
    REQUIRE(child.getValue() == ">&'\"");
}

TEST_CASE("CharacterReferences", "[parsing]")
{
    const xml::Data d = xml::parse("<root test=\"&#65;\">&#x42;</root>", true, true, true);

    const auto first = d.begin();
    const auto& node = *first;

    const auto firstAttribute = node.getAttributes().begin();
    REQUIRE(firstAttribute != node.getAttributes().end());

    const auto& attribute = *firstAttribute;
    REQUIRE(attribute.first == "test");

    REQUIRE(attribute.second == "A");

    const auto firstChild = node.begin();
    REQUIRE(firstChild != node.end());

    const auto& child = *firstChild;
    REQUIRE(child.getType() == xml::Node::Type::text);
    REQUIRE(child.getValue() == "B");
}

TEST_CASE("Character data", "[parsing]")
{
    const xml::Data d = xml::parse("<root><![CDATA[test]]></root>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::tag);
    REQUIRE(node.getName() == "root");

    const auto& firstChild = node.begin();
    REQUIRE(firstChild != node.end());

    const auto& childNode = *firstChild;
    REQUIRE(childNode.getType() == xml::Node::Type::characterData);
    REQUIRE(childNode.getValue() == "test");
}

TEST_CASE("Document type definition", "[parsing]")
{
    const xml::Data d = xml::parse("<!DOCTYPE test><root></root>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::documentTypeDefinition);
}

TEST_CASE("Encoding", "[encoding]")
{
    xml::Data d;
    xml::Node p(xml::Node::Type::processingInstruction);
    p.setName("xml");
    p.setValue("version=\"1.0\" encoding=\"utf-8\"");
    d.pushBack(p);

    xml::Node n(xml::Node::Type::tag);
    n.setName("n");
    n.setAttributes({{"a", "a"}, {"b", "b"}});

    xml::Node c1(xml::Node::Type::tag);
    c1.setName("c1");
    c1.setAttributes({{"c", "c"}});

    xml::Node c2(xml::Node::Type::tag);
    c2.setName("c2");
    c2.setAttributes({{"dd", "dd"}});

    xml::Node t(xml::Node::Type::text);
    t.setValue("text");

    c1.pushBack(t);
    n.pushBack(c1);
    n.pushBack(c2);
    d.pushBack(n);

    REQUIRE(xml::encode(d) == "<?xml version=\"1.0\" encoding=\"utf-8\"?><n a=\"a\" b=\"b\"><c1 c=\"c\">text</c1><c2 dd=\"dd\"/></n>");
    REQUIRE(xml::encode(d, true) == "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<n a=\"a\" b=\"b\">\n\t<c1 c=\"c\">\n\t\ttext\n\t</c1>\n\t<c2 dd=\"dd\"/>\n</n>\n");
}

TEST_CASE("IllegalCharacters", "[errors]")
{
    REQUIRE_THROWS_AS(xml::parse("<root>&</root>", true, true, true), xml::ParseError);
    REQUIRE_THROWS_AS(xml::parse("<root a=\"<\"></root>", true, true, true), xml::ParseError);
}

TEST_CASE("Illegal processing instruction", "[errors]")
{
    REQUIRE_THROWS_AS(xml::parse("<root><?xml version=\"1.0\"?></root>", true, true, true), xml::ParseError);
}

TEST_CASE("Invalid comment", "[errors]")
{
    REQUIRE_THROWS_AS(xml::parse("<!-- comment -- a -->", true, true, true), xml::ParseError);
}

TEST_CASE("Byte", "[byte]")
{
    const std::vector<std::byte> data = {
        static_cast<std::byte>('<'),
        static_cast<std::byte>('r'),
        static_cast<std::byte>('/'),
        static_cast<std::byte>('>')
    };

    const xml::Data d = xml::parse(data, true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::tag);
    REQUIRE(node.getName() == "r");
}

TEST_CASE("Range-based for loop for data")
{
    SECTION("Mutable")
    {
        xml::Data data;
        data.pushBack(xml::Node{"0"});
        data.pushBack(xml::Node{"1"});

        int counter = 0;

        for (xml::Node& i : data)
            REQUIRE(i.getValue() == std::to_string(counter++));

        REQUIRE(counter == 2);
    }

    SECTION("Const")
    {
        xml::Data data;
        data.pushBack(xml::Node{"0"});
        data.pushBack(xml::Node{"1"});

        const auto& constData = data;

        int counter = 0;

        for (const xml::Node& i : constData)
            REQUIRE(i.getValue() == std::to_string(counter++));

        REQUIRE(counter == 2);
    }
}

TEST_CASE("Range-based for loop for node")
{
    SECTION("Mutable")
    {
        xml::Node node;
        node.pushBack(xml::Node{"0"});
        node.pushBack(xml::Node{"1"});

        int counter = 0;

        for (xml::Node& i : node)
            REQUIRE(i.getValue() == std::to_string(counter++));

        REQUIRE(counter == 2);
    }

    SECTION("Const")
    {
        xml::Node node;
        node.pushBack(xml::Node{"0"});
        node.pushBack(xml::Node{"1"});

        const auto& constNode = node;

        int counter = 0;

        for (const xml::Node& i : constNode)
            REQUIRE(i.getValue() == std::to_string(counter++));

        REQUIRE(counter == 2);
    }
}
