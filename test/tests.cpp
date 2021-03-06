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

TEST_CASE("Comments", "[comments]")
{
    const xml::Data d = xml::parse("<!--test--><root/>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::comment);
    REQUIRE(node.getValue() == "test");
}

TEST_CASE("Empty element", "[empty_element]")
{
    const xml::Data d = xml::parse("<root/>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::tag);
    REQUIRE(node.getValue() == "root");
}

TEST_CASE("End tag", "[end_tag]")
{
    const xml::Data d = xml::parse("<root></root>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::tag);
    REQUIRE(node.getValue() == "root");
}

TEST_CASE("ProcessingInstruction", "[processing_instructions]")
{
    const xml::Data d = xml::parse("<?xml version=\"1.0\"?><root/>", true, true, true);

    const auto first = d.begin();
    REQUIRE(first != d.end());

    const auto& node = *first;
    REQUIRE(node.getType() == xml::Node::Type::processingInstruction);
    REQUIRE(node.getValue() == "xml");
    REQUIRE(node["version"] == "1.0");
}

TEST_CASE("Text", "[text]")
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

TEST_CASE("Attributes", "[attributes]")
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

TEST_CASE("EntityReferences", "[entity_references]")
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

TEST_CASE("CharacterReferences", "[character_references]")
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

TEST_CASE("Encoding", "[encoding]")
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

    REQUIRE(xml::encode(d) == "<?xml encoding=\"utf-8\" version=\"1.0\"?><n a=\"a\" b=\"b\"><c1 c=\"c\">text</c1><c2 dd=\"dd\"/></n>");
    REQUIRE(xml::encode(d, true) == "<?xml encoding=\"utf-8\" version=\"1.0\"?>\n<n a=\"a\" b=\"b\">\n\t<c1 c=\"c\">\n\t\ttext\n\t</c1>\n\t<c2 dd=\"dd\"/>\n</n>\n");
}

TEST_CASE("IllegalCharacters", "[illegal_characters]")
{
    REQUIRE_THROWS_AS(xml::parse("<root>&</root>", true, true, true), xml::ParseError);
    REQUIRE_THROWS_AS(xml::parse("<root a=\"<\"></root>", true, true, true), xml::ParseError);
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
    REQUIRE(node.getValue() == "r");
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
