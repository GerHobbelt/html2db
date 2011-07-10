#ifndef _MAIN_H
#define _MAIN_H

Bool dbxml;
Bool dbsgml;
typedef enum {
    SgmlTagStart,
    SgmlTagEnd
}SgmlTagType;
void PrintSgml( TidyDocImpl* doc, uint mode, uint indent, Node *node );

/* In tidylib functions */
extern Bool DescendantOf( Node *element, TidyTagId tid );
extern void PPrintString( TidyDocImpl* doc, uint indent, ctmbstr str );
extern void PPrintChar( TidyDocImpl* doc, uint c, uint mode );
extern void PPrintText( TidyDocImpl* doc, uint mode, uint indent, Node* node );
extern void PPrintComment( TidyDocImpl* doc, uint indent, Node* node );
extern void PPrintCDATA( TidyDocImpl* doc, uint indent, Node *node );
extern void PPrintSection( TidyDocImpl* doc, uint indent, Node *node );
extern void PPrintPI( TidyDocImpl* doc, uint indent, Node *node );
extern void PPrintXmlDecl( TidyDocImpl* doc, uint indent, Node *node );
extern Bool ShouldIndent( TidyDocImpl* doc, Node *node );

#endif
