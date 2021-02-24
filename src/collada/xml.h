struct xml_attribute
{
    char* Namespace;
    char* Name;
    _sbuf_ char* Value; //Sbuf because parsed as a string by the lexer
};

struct xml_element
{
    char* Name;
    char* Namespace;
    _sbuf_ xml_attribute* Attributes;
    //@Cleanup We currently alow text only if there are no children,
    //or if they come after it
    char* Text;
    _sbuf_ xml_element** Children;
    _sbuf_ xml_element* Parent;
    u64 ChildrenCount;
};

struct xml_file
{
    xml_element* Header;
    xml_element* Root;
};

