#include "xml.h"

internal xml_element*
AllocXmlElement(char* Name, char* Namespace = 0, xml_attribute* Attributes = 0,
                char* Text = 0, xml_element** Children = 0)
{
    xml_element* Result = (xml_element*)ZeroAlloc(sizeof(xml_element));
    Result->Name = Name;
    Result->Namespace = Namespace;
    Result->Attributes = Attributes;
    Result->Text = Text;
    Result->Children = Children;
    Result->ChildrenCount = SbufLen(Children);
    
    return Result;
}

internal xml_attribute*
ParseXmlAttributes(lexer* Lexer)
{
    _sbuf_ xml_attribute* Attributes = 0;
    while(IsToken(Lexer, TOKEN_NAME))
    {
        xml_attribute Item = {};
        Item.Name = ParseName(Lexer);
        if(MatchToken(Lexer, TOKEN_COLON))
        {
            Item.Namespace = Item.Name;
            Item.Name = ParseName(Lexer);
        }
        ExpectToken(Lexer, TOKEN_ASSIGN);
        Item.Value = Lexer->Token.StringVal;
        ExpectToken(Lexer, TOKEN_STRING);
        
        SbufPush(Attributes, Item);
    }
    
    return Attributes;
}

internal xml_element*
ParseXmlElement(lexer* Lexer)
{
    char* Name = ParseName(Lexer);
    char* Namespace = 0;
    if(MatchToken(Lexer, TOKEN_COLON))
    {
        Namespace = Name;
        Name = ParseName(Lexer);
    }
    
    xml_attribute* Attributes = ParseXmlAttributes(Lexer);
    if(MatchToken(Lexer, TOKEN_DIV))
    {
        ExpectToken(Lexer, TOKEN_GT);
        xml_element* Element = AllocXmlElement(Name, Namespace, Attributes);
        return Element;
    }
    ExpectToken(Lexer, TOKEN_GT);
    
    //while no element beginning
    char* TextBegin = 0;
    char* TextEnd = 0;
    if(!IsToken(Lexer, TOKEN_LT))
    {
        TextBegin = Lexer->Token.Start;
        JumpToChar(Lexer, '<');
        TextEnd = Lexer->Token.Start;
    }
    char* Text = 0;
    if(TextBegin)
    {
        Text = (char*)ZeroAlloc(TextEnd - TextBegin + 1);
        strncpy(Text, TextBegin, TextEnd - TextBegin);
    }
    
    _sbuf_ xml_element** Children = 0;
    while(MatchToken(Lexer, TOKEN_LT))
    {
        if(MatchToken(Lexer, TOKEN_DIV))
        {
            char* ClosingName = ParseName(Lexer);
            ExpectToken(Lexer, TOKEN_GT);
            Assert(ClosingName == Name);
            break;
        }
        xml_element* Child = ParseXmlElement(Lexer);
        SbufPush(Children, Child);
    }
    
    xml_element* Element = AllocXmlElement(Name, Namespace, Attributes, Text, Children);
    return Element;
}

internal void
SetParentToXmlTree(xml_element* Element, xml_element* Parent)
{
    Element->Parent = Parent;
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        SetParentToXmlTree(Element->Children[Index], Element);
    }
}

//Expects an initialized lexer, flags should be LEXER_IGNORE_NEWLINES | LEXER_XML_COMMENTS
internal xml_file
ParseXmlFile(lexer* Lexer)
{
    ExpectToken(Lexer, TOKEN_LT);
    ExpectToken(Lexer, TOKEN_QUESTION);
    char* HeaderName = ParseName(Lexer);
    xml_attribute* HeaderAttributes = ParseXmlAttributes(Lexer);
    ExpectToken(Lexer, TOKEN_QUESTION);
    ExpectToken(Lexer, TOKEN_GT);
    
    xml_element* Header = AllocXmlElement(HeaderName, 0, HeaderAttributes);
    
    //We parse the elements starting from the token after the opening angled bracket
    ExpectToken(Lexer, TOKEN_LT);
    xml_element* Root = ParseXmlElement(Lexer);
    SetParentToXmlTree(Root, 0);
    
    xml_file Result = {};
    Result.Header = Header;
    Result.Root = Root;
    
    return Result;
}

internal void
FreeXmlElement(xml_element* Element)
{
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        FreeXmlElement(Element->Children[Index]);
    }
    
    for(u32 Index = 0; Index < SbufLen(Element->Attributes); Index++)
    {
        SbufFree(Element->Attributes[Index].Value);
    }
    SbufFree(Element->Attributes);
    SbufFree(Element->Children);
    Free(Element->Text);
}

internal void
FreeXmlFile(xml_file* XmlFile)
{
    FreeXmlElement(XmlFile->Header);
    FreeXmlElement(XmlFile->Root);
    
    *XmlFile = {};
}

internal void
PrintIndentation(u32 Indentation)
{
    for(u32 i = 0; i < Indentation; i++)
    {
        printf("  ");
    }
}

internal void
PrintXmlElement(xml_element* Element, u32 Indentation)
{
    PrintIndentation(Indentation);
    
    printf("<");
    if(Element->Namespace)
    {
        printf("%s:", Element->Namespace);
    }
    
    printf("%s", Element->Name);
    
    for(u32 Index = 0; Index < SbufLen(Element->Attributes); Index++)
    {
        printf(" %s=\"%s\"", Element->Attributes[Index].Name, Element->Attributes[Index].Value);
    }
    
    printf(">");
    Indentation++;
    
    if(Element->Text)
    {
        printf("%s", Element->Text);
    }
    
    if(Element->Children)
    {
        printf("\n");
    }
    
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        PrintXmlElement(Element->Children[Index], Indentation);
    }
    
    Indentation--;
    if(Element->Children)
    {
        PrintIndentation(Indentation);
    }
    
    printf("</%s>\n", Element->Name);
}

//Depth first search, recursive, easier to implement, good enough?
//Name must be interned
internal xml_element*
FindXmlFirstChildByNameRec(xml_element* Element, char* Name)
{
    if(Element->Name == Name)
        return Element;
    
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        xml_element* Result = FindXmlFirstChildByNameRec(Element->Children[Index], Name);
        if(Result) return Result;
    }
    
    return 0;
}

internal xml_element*
FindXmlFirstChildByName(xml_element* Element, char* Name)
{
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        xml_element* Result = FindXmlFirstChildByNameRec(Element->Children[Index], Name);
        if(Result) return Result;
    }
    
    return 0;
}


//Name must be interned, Value is not interned
internal xml_element*
FindXmlFirstChildByAttributeRec(xml_element* Element, char* Name, char* Value)
{
    for(u32 Index = 0; Index < SbufLen(Element->Attributes); Index++)
    {
        if(Element->Attributes[Index].Name == Name &&
           strcmp(Element->Attributes[Index].Value, Value) == 0)
        {
            return Element;
        }
    }
    
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        xml_element* Result = FindXmlFirstChildByAttributeRec(Element->Children[Index], Name, Value);
        if(Result) return Result;
    }
    
    return 0;
}

internal xml_element*
FindXmlFirstChildByAttribute(xml_element* Element, char* Name, char* Value)
{
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        xml_element* Result = FindXmlFirstChildByAttributeRec(Element->Children[Index], Name, Value);
        if(Result) return Result;
    }
    
    return 0;
}

//Name must be interned
internal _sbuf_ xml_element**
FindAllDirectXmlChildrenByName(xml_element* Element, char* Name)
{
    _sbuf_ xml_element** Result = 0;
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        xml_element* Child = Element->Children[Index];
        if(Child->Name == Name)
        {
            SbufPush(Result, Child);
        }
    }
    
    return Result;
}

//ElementName and AttributeName must be interned, Value is not
internal xml_element*
FindXmlFirstChildByNameAndAttributeRec(xml_element* Element, char* ElementName, char* AttributeName,
                                       char* Value)
{
    if(Element->Name == ElementName)
    {
        for(u32 Index = 0; Index < SbufLen(Element->Attributes); Index++)
        {
            if(Element->Attributes[Index].Name == AttributeName &&
               strcmp(Element->Attributes[Index].Value, Value) == 0)
            {
                return Element;
            }
        }
    }
    
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        xml_element* Result = FindXmlFirstChildByNameAndAttributeRec(Element->Children[Index], ElementName,
                                                                     AttributeName, Value);
        if(Result) return Result;
    }
    
    return 0;
}

//ElementName and AttributeName must be interned, Value is not
internal xml_element*
FindXmlFirstChildByNameAndAttribute(xml_element* Element, char* ElementName, char* AttributeName,
                                    char* Value)
{
    for(u32 Index = 0; Index < SbufLen(Element->Children); Index++)
    {
        xml_element* Result = FindXmlFirstChildByNameAndAttributeRec(Element->Children[Index], ElementName,
                                                                     AttributeName, Value);
        if(Result) return Result;
    }
    
    return 0;
}

//Name must be interned
internal char*
GetXmlAttributeValue(xml_element* Element, char* Name)
{
    for(u32 Index = 0; Index < SbufLen(Element->Attributes); Index++)
    {
        if(Element->Attributes[Index].Name == Name)
        {
            return Element->Attributes[Index].Value;
        }
    }
    
    return 0;
}

internal u32
GetXmlAttributeValueAsU32(xml_element* Element, char* Name)
{
    return strtoul(GetXmlAttributeValue(Element, Name), 0, 10);
}

