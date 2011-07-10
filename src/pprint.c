/*
  pprint.c -- pretty print in sgml
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tidy.h"
#include "pprint.h"
#include "tidy-int.h"
#include "tmbstr.h"
#include "parser.h"
#include "entities.h"
#include "tmbstr.h"
#include "utf8.h"
#include "lexer.h"
#include "main.h"


#if 0 /* [i_a] clashes with functions in lexer.obj(tidylib.lib) - when linking to static lib 'tidylib.lib */

Bool TY_(IsDigit)(uint c)
{
	if (c < 0x100)
		return (isdigit(c) ? yes : no);
	return no;
}

uint TY_(ToLower)(uint c)
{
	if (c < 0x100)
		c = toupper(c);

    return c;
}

uint TY_(ToUpper)(uint c)
{
	if (c < 0x100)
		c = tolower(c);

    return c;
}

#endif


uint tidyNodeGetModel(TidyNode node)
{
	Node* nimp = tidyNodeToImpl( node );
	if (nimp && nimp->tag)
		return nimp->tag->model;
	return CM_UNKNOWN;
}

void tidyNodeSetType(TidyNode node, NodeType nt)
{
	Node* nimp = tidyNodeToImpl( node );
	if (nimp)
		nimp->type = nt;
}


void PPrintDocType( TidyDoc doc, uint indent, TidyNode node )
{
    if(dbsgml)
        tidyPPrintString(doc, indent, "<!DOCTYPE article PUBLIC \"-//OASIS//DTD DocBook V4.1//EN\">\n");
    else if(dbxml)
        tidyPPrintString(doc, indent, "<?xml version='1.0'?>\n<!DOCTYPE book PUBLIC \"-//OASIS//DTD DocBook XML V4.2//EN\">\n");
}

void PrintSgmlDefault()
{
    char *str = "SGML cannot contain these elements";

    fprintf(stderr, str);
}

void PrintSgmlBodyStart(TidyDoc doc, uint indent)
{
    char *str = "<article>";
    tidyPPrintString(doc, indent, str);
}

#define DIGIT(c) (c - 48)
#define TOTAL_H 6
static Bool seen_h[TOTAL_H] = {no, no, no, no, no, no};

/* Yuck ugly. FIXME */
#define SECT(i) (i - startsect)
static int startsect = 0; /* We are at level 0(H1) initially */

void PrintSgmlBodyEnd(TidyDoc doc, uint indent)
{   
	int i = TOTAL_H - 1;
    char str[30];

    while(i >= 0) {
        if(seen_h[i] == yes) {
            if(i == 5)
                sprintf(str, "</simplesect>");
            else
                sprintf(str, "</sect%d>", SECT(i) + 1);
            tidyPPrintString(doc, indent, str);
            seen_h[i] = no;
        }
        --i;
    }
    
    sprintf(str, "</article>");
    tidyPPrintString(doc, indent, str);
}

TidyBuffer *GetContent(TidyDoc doc, TidyNode node)
{   
	TidyNode content;
	TidyNode temp_node;
	char *temp;
    Bool flag = no;
    int i;
	TidyBuffer *buf = tidyBufCreate(NULL);
	tidyBufCheckAlloc(buf, 256, 0);

    content = tidyGetChild(node);

    /* Find the <a> tag */
    for (temp_node = content;
         temp_node && !tidyNodeIsA(temp_node); 
         temp_node = tidyGetNext(temp_node))
         ;
    
    if(temp_node == NULL) { /* There is no <a> .. </a> tag */
        /* Discard all elements which are not text nodes */
        temp_node = content;
        for (temp_node = content;
             temp_node && tidyNodeGetType(temp_node) != TidyNode_Text; 
             temp_node = tidyGetNext(temp_node))
           ;
        if(temp_node == NULL) { /* There's no TextNode either */ 
			buf->bp[0] = 0;
            return buf;
        }
    }
    content = temp_node;

    if(tidyNodeGetType(content) == TextNode)  {
		tidyNodeGetText(doc, content, buf);
    }
    else if(tidyNodeIsA(content)){
        TidyAttr name;
        int size;

        name = tidyGetAttrByName(content, "name");
        if(name == NULL)
            name = tidyGetAttrByName(content, "href");

        if(name == NULL) {  /* No href or name, let's take empty id */
			buf->bp[0] = 0;
			return buf;
        }
        else {
			ctmbstr v = tidyAttrValue(name);
            size = TY_(tmbstrlen)(v);
			tidyBufAppend(buf, v, size);
       }
    }

	/* enforce NUL termination in buf: */
	assert(buf->size < buf->allocated);
	buf->bp[buf->size] = 0;

    temp = buf->bp;
    if(temp[0] == '#')
        flag = yes;

#define SGML_NAMELEN 44  /* Maximum id namelength */
    for(i = 0; *temp && i < SGML_NAMELEN; ++temp, ++i) {
        if(flag)
            *temp = *(temp + 1);
        /* FIXME: Add more characters which are not allowed later */
        /* Another way is to skip over these characters */
        if(*temp == ' ' || *temp == '+' || 
           *temp == ':' || *temp == '*' ||
           *temp == ')' || *temp == '(' ||
           *temp == '/')
            *temp = '_';
    }
    *temp = '\0';
	buf->size = temp - buf->bp;

    /* This might create problem with linkends */
#if 0
    /* ID cannot start with '_' */
    if(str[0] = '_')
        str[0] = 'a';
#endif
    return buf;
}

void PrintSectTag( TidyDoc doc, uint indent, TidyNode node, uint startsect)
{   
	char sectnum = tidyNodeGetName(node)[1];
    char str[500];

    TidyBuffer *id = GetContent(doc, node);
	assert(TY_(tmbstrlen)(id->bp) < id->allocated);

    if(sectnum == '6')  /* there's no sect6. We can do variety of
                           things here. may be <section> .. */
        sprintf(str, "<simplesect id=\"%s\"><title>", id->bp); 
    else 
        sprintf(str, "<sect%c id=\"%s\"><title>", SECT(sectnum), id->bp); 
    tidyPPrintString(doc, indent, str);
	tidyBufFree(id);
}

Bool ImmediateDescendantOfHTags(TidyNode element)
{   
	TidyNode parent = tidyGetParent(element);

    if (strlen(tidyNodeGetName(parent)) == 2 && 
            tidyNodeGetName(parent)[0] == 'h' && 
            TY_(IsDigit)(tidyNodeGetName(parent)[1]))
        return yes;
    return no;
}
void PrintSgmlLink(TidyDoc doc, uint indent, TidyNode node)
{   TidyAttr addr;
    char str[500];  /* FIXME: allocate dynamically later */

    addr = tidyGetAttrByName(node, "name");
    if(addr == NULL) {
        addr = tidyGetAttrByName(node, "href");
        if(!ImmediateDescendantOfHTags(node)) {
            if(tidyAttrValue(addr)[0] == '#') 
                sprintf(str, "<link linkend=\"%s\">", tidyAttrValue(addr) + 1); 
            else
                sprintf(str, "<ulink url=\"%s\">", tidyAttrValue(addr)); 
            if( !tidyDescendantOf(node, TidyTag_P) && 
                /* <programlisting> doesn't allow <para> */
                !tidyDescendantOf(node, TidyTag_PRE) &&
                 tidyGetPrev(node) && tidyNodeGetType(tidyGetPrev(node)) == TextNode)
                tidyPPrintString(doc, indent, "<para>");
            tidyPPrintString(doc, indent, str);
        }
    }
    else {
        if(!ImmediateDescendantOfHTags(node)) {
            if(!tidyDescendantOf(node, TidyTag_P)) 
                sprintf(str, "<para id=\"%s\">", tidyAttrValue(addr)); 
            else /* We cannnot have a <para> inside another <para> */
                sprintf(str, "<anchor id=\"%s\"/>", tidyAttrValue(addr)); 
            tidyPPrintString(doc, indent, str);
        }
    }
}

void PrintSgmlLinkEnd(TidyDoc doc, uint indent, TidyNode node)
{   
	TidyAttr addr;

    addr = tidyGetAttrByName(node, "name");
    if(addr == NULL) {
        addr = tidyGetAttrByName(node, "href");
        if(!ImmediateDescendantOfHTags(node)) {
            if(tidyAttrValue(addr)[0] == '#') 
                tidyPPrintString(doc, indent, "</link>");
            else
                tidyPPrintString(doc, indent, "</ulink>");
            if( !tidyDescendantOf(node, TidyTag_P) && 
                 /* <programlisting> doesn't allow <para> */
                !tidyDescendantOf(node, TidyTag_PRE) &&
                 tidyGetPrev(node) && tidyNodeGetType(tidyGetPrev(node)) == TextNode)
                tidyPPrintString(doc, indent, "</para>");
        }
    }
    else {
        if(!ImmediateDescendantOfHTags(node)) {
            if(!tidyDescendantOf(node, TidyTag_P))
                tidyPPrintString(doc, indent, "</para>");
            /* else
               <anchor ..  /> has already been placed. no need to
               do any thing */
        }
    }
}


void PrintSgmlTagString(TidyDoc doc, uint mode, uint indent, 
                        SgmlTagType sgmltag_type, char *str)
{   tidyPPrintChar(doc, str[0], mode | TidyTextFormat_CDATA);
    if(sgmltag_type == SgmlTagEnd)
        tidyPPrintChar(doc, '/', mode);
    tidyPPrintString(doc, indent, str + 1);
}

void PrintSgmlList(TidyDoc doc, uint indent, uint mode, TidyNode node)
{   if(tidyNodeIsUL(node))
        tidyPPrintString(doc, indent, "<itemizedlist>");
    else if(tidyNodeIsOL(node))
        tidyPPrintString(doc, indent, "<orderedlist>");
    else if(tidyNodeIsDL(node))
        tidyPPrintString(doc, indent, "<variablelist>");
}

void PrintSgmlListEnd(TidyDoc doc, uint indent, uint mode, TidyNode node)
{   if(tidyNodeIsUL(node))
        tidyPPrintString(doc, indent, "</itemizedlist>");
    else if(tidyNodeIsUL(node))
        tidyPPrintString(doc, indent, "</orderedlist>");
    else if(tidyNodeIsDL(node))
        tidyPPrintString(doc, indent, "</variablelist>");
}

void PrintSgmlListItem(TidyDoc doc, uint indent, TidyNode node)
{   if(tidyNodeIsLI(node))
        tidyPPrintString(doc, indent, "<listitem>");
    else if(tidyNodeIsDD(node))
        tidyPPrintString(doc, indent, "<listitem>");
}

void PrintSgmlListItemEnd(TidyDoc doc, uint indent, TidyNode node)
{   if(tidyNodeIsLI(node))
        tidyPPrintString(doc, indent, "</listitem>");
    else if(tidyNodeIsDD(node))
        tidyPPrintString(doc, indent, "</listitem></varlistentry>");
}

void PrintSgmlImage(TidyDoc doc, uint indent, TidyNode node)
{   TidyAttr addr;
    char str[500];

    addr = tidyGetAttrByName(node, "src");
    /* We can get other attributes like width, height etc.. */
    if(addr != NULL) {
        tidyPPrintString(doc, indent, "<inlinemediaobject><imageobject>");
        tidyPCondFlushLine(doc, indent);
        sprintf(str, "<imagedata fileref=\"%s\">", tidyAttrValue(addr)); 
        tidyPPrintString(doc, indent, str);
        tidyPCondFlushLine(doc, indent);
        tidyPPrintString(doc, indent, "</imageobject></inlinemediaobject>");
        tidyPCondFlushLine(doc, indent);
    }
}

int CountColumns(TidyNode node)
{
	TidyNode temp;
	TidyNode row_content;
    int ncols = 0;

    temp = tidyGetChild(node);

    /* FIXME: */
    /* Perhaps this is not needed, check with HTML standard later */
    /*
    while(tidyNodeIsTR(temp))
        temp = temp->next;
    */
    /* This can contain th or td's */
    row_content = tidyGetChild(temp);
    while(row_content) {
        if(tidyNodeIsTH(row_content) || tidyNodeIsTD(row_content)) {
            TidyAttr colspan;
            
            colspan = tidyGetAttrByName(row_content, "colspan");
            if(colspan)
                ncols += atoi(tidyAttrValue(colspan));
            else
                ++ncols;
        }
        else
            fprintf(stderr, "PrintSgml: error in table processing\n");
        row_content = tidyGetNext(row_content);
   } 
   return ncols;
}

void PrintSgmlTable(TidyDoc doc, uint indent, TidyNode node)
{   int ncols;
    char str[500];

    tidyPPrintString(doc, indent, "<informaltable>");
    ncols = CountColumns(node);
    sprintf(str, "<tgroup cols=\"%d\"><tbody>", ncols);
    tidyPPrintString(doc, indent, str);
}

void PrintSgmlTableEnd(TidyDoc doc, uint indent, TidyNode node)
{   
    tidyPPrintString(doc, indent, "</tbody></tgroup></informaltable>");
}

Bool DescendantOfAddress(TidyNode element)
{
    TidyNode parent;
    
    for (parent = tidyGetParent(element);
            parent != NULL; parent = tidyGetParent(parent))
    {   if (tidyNodeGetName(parent) && TY_(tmbstrcasecmp)(tidyNodeGetName(parent), "address") == 0)
            return yes;
    }

    return no;
}

void PrintSgmlTag( TidyDoc doc, uint mode, uint indent, TidyNode node,
                    SgmlTagType sgmltag_type)
{   static int level = -1;

    if(tidyNodeIsHTML(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlBodyStart(doc, indent);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlBodyEnd(doc, indent);
    }
    else if(tidyNodeIsHEAD(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<articleinfo>");
    else if(tidyNodeIsTITLE(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<title>");
    /* May be we can replace with node->model & CM_LIST */
    else if(tidyNodeIsUL(node) || tidyNodeIsOL(node)|| tidyNodeIsDL(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlList(doc, indent, mode, node);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlListEnd(doc, indent, mode, node);
    }
    else if(tidyNodeIsDT(node)) {
        if(sgmltag_type == SgmlTagStart)
            tidyPPrintString(doc, indent, "<varlistentry><term>");
        else if(sgmltag_type == SgmlTagEnd)
            tidyPPrintString(doc, indent, "</term>");
    }
    else if(tidyNodeIsLI(node) || tidyNodeIsDD(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlListItem(doc, indent, node);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlListItemEnd(doc, indent, node);
    }
    /* Later we should clean this before coming to PrintSgml */
    else if(tidyNodeIsP(node) && 
            /* Table <entry> processing */
            !tidyDescendantOf(node, TidyTag_TH) && !tidyDescendantOf(node, TidyTag_TD) && 
            !DescendantOfAddress(node) && 
            /* <programlisting> doesn't allow <para> */
            !tidyDescendantOf(node, TidyTag_PRE)) 
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<para>");
    else if(tidyNodeIsBLOCKQUOTE(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<blockquote>");
    else if(tidyNodeIsPRE(node) && 
            /* Table <entry> processing */
            !tidyDescendantOf(node, TidyTag_TH) && !tidyDescendantOf(node, TidyTag_TD))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, 
                               "<programlisting>");
    else if(tidyNodeIsA(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlLink(doc, indent, node);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlLinkEnd(doc, indent, node);
    }
    /* Table would require more processing */
    else if(tidyNodeIsTABLE(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlTable(doc, indent, node);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlTableEnd(doc, indent, node);
    }
    else if(tidyNodeIsTR(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<row>");
    else if(tidyNodeIsTD(node) || tidyNodeIsTH(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<entry>");
    else if(tidyNodeIsIMG(node)) { /* This is a StartEndTag */
        if(sgmltag_type == SgmlTagStart)    
            PrintSgmlImage(doc, indent, node);
    }

    else if(TY_(tmbstrcasecmp)(tidyNodeGetName(node), "cite") == 0)
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, 
                                "<citation>");
    /* We should distinguish tag_strong and tag_em later 
       haven't found proper docbook tag for <strong> */
    else if(tidyNodeIsEM(node) || tidyNodeIsSTRONG(node) ||
            TY_(tmbstrcasecmp)(tidyNodeGetName(node), "address") == 0) {
        if(sgmltag_type == SgmlTagStart) {
            if(tidyDescendantOf(node, TidyTag_P) || tidyDescendantOf(node, TidyTag_PRE))
                tidyPPrintString(doc, indent, "<emphasis>");
            else
                tidyPPrintString(doc, indent, "<para><emphasis>");
        }
        else if(sgmltag_type == SgmlTagEnd) {
            if(tidyDescendantOf(node, TidyTag_P) || tidyDescendantOf(node, TidyTag_PRE))
                tidyPPrintString(doc, indent, "</emphasis>");
            else 
                tidyPPrintString(doc, indent, "</emphasis></para>");
        }
    }
    else {
        if(TY_(tmbstrcasecmp)(tidyNodeGetName(node), "code") == 0 &&
            !(tidyNodeIsDD(tidyGetParent(node)) || tidyNodeIsLI(tidyGetParent(node))))
                PrintSgmlTagString(doc, mode, indent, 
                                sgmltag_type, "<literal>");
        else if(strlen(tidyNodeGetName(node)) == 2 && 
                tidyNodeGetName(node)[0] == 'h' && 
                TY_(IsDigit)(tidyNodeGetName(node)[1])) {
            if(sgmltag_type == SgmlTagStart) {        
                int sectnum = DIGIT(tidyNodeGetName(node)[1]) - 1;
                char str[30];

                if(seen_h[sectnum] == no)
                    seen_h[sectnum] = yes;
                else {
                    int i = level;
                    while(i > sectnum && seen_h[i] == yes) {
                        if(i == 5)
                            sprintf(str, "</simplesect>");
                        else
                            sprintf(str, "</sect%d>", SECT(i) + 1);
                        tidyPPrintString(doc, indent, str);
                        seen_h[i] = no;
                        --i;
                    }
                    if(sectnum == 5)
                        sprintf(str, "</simplesect>");
                    else
                        sprintf(str, "</sect%d>", SECT(sectnum) + 1);
                    tidyPPrintString(doc, indent, str);
                }
                /* H1 is not the first level 
                   like the curses man2html pages */
                if(level == -1 && sectnum > 0) 
                    startsect = sectnum;
                
                PrintSectTag(doc, indent, node, startsect);
                level = sectnum;
            }
            else
                tidyPPrintString(doc, indent, "</title>");
        }
    }
}

void PrintSgml( TidyDoc doc, uint mode, uint indent, TidyNode node )
{   
	TidyNode content;
    Bool xhtml = tidyOptGetBool( doc, TidyXhtmlOut );
    uint spaces = tidyOptGetInt( doc, TidyIndentSpaces );

    if (node == NULL)
        return;
    
    if (tidyNodeGetType(node) == TextNode) {
        if(tidyDescendantOf(node, TidyTag_DD) && !tidyDescendantOf(node, TidyTag_A) &&
            !tidyDescendantOf(node, TidyTag_P) && 
            /* We have to decide on this table stuff later
             * <entry> processing is complex */
            !tidyDescendantOf(node, TidyTag_TD) && !tidyDescendantOf(node, TidyTag_TH) &&
            /* <programlisting> doesn't allow <para> */
            !tidyDescendantOf(node, TidyTag_PRE)    
            ) 
        /*  && wstrcasecmp(tidyGetParent(node)->element, "code") != 0)
         above line may be needed later to properly convert <code> stuff */
        {
            tidyPPrintString(doc, indent, "<para>");
            tidyPPrintText(doc, mode, indent, node);
            tidyPPrintString(doc, indent, "</para>");
        }
        else {
            if(tidyDescendantOf(node, TidyTag_STYLE))
                fprintf(stderr, "PrintSgml: skipping style elements\n\n");
            else
                tidyPPrintText(doc, mode, indent, node);
        }
    }
    else if(tidyNodeGetType(node) == CDATATag && TidyEscapeCdata)
        tidyPPrintText(doc, mode, indent, node);
    else if (tidyNodeGetType(node) == CommentTag)
        tidyPPrintComment(doc, indent, node);
    else if (tidyNodeGetType(node) == RootNode)
    {
        for (content = tidyGetChild(node);
                content != NULL;
                content = tidyGetNext(content))
		{
           PrintSgml(doc, mode, indent, content);
		}
    }
    else if (tidyNodeGetType(node) == DocTypeTag)
        PPrintDocType(doc, indent, node);
    else if (tidyNodeGetType(node) == CDATATag)
        tidyPPrintCDATA(doc, indent, node);
    else if (tidyNodeGetType(node) == SectionTag)
        tidyPPrintSection(doc, indent, node);
    else if (tidyNodeGetType(node) == AspTag || 
             tidyNodeGetType(node) == JsteTag ||
             tidyNodeGetType(node) == PhpTag )
        PrintSgmlDefault();
    else if (tidyNodeGetType(node) == ProcInsTag)
        tidyPPrintPI(doc, indent, node);
    else if (tidyNodeGetType(node) == XmlDecl)// && DbXml May be this is needed
        tidyPPrintXmlDecl(doc, indent, node);
    else if (tidyNodeGetModel(node) & CM_EMPTY || 
            (tidyNodeGetType(node) == StartEndTag && !xhtml))
    {
        if (!(tidyNodeGetModel(node) & CM_INLINE))
            tidyPCondFlushLine(doc, indent);

        if ( tidyOptGetBool(doc, TidyMakeClean) && tidyNodeIsWBR(node) )
            tidyPPrintString(doc, indent, " ");
        else
            PrintSgmlTag(doc, mode, indent, node, SgmlTagStart);
    }
    else {
        if (tidyNodeGetType(node) == StartEndTag)
            tidyNodeSetType(node, StartTag);
        
		if (tidyNodeGetId(node) == TidyTag_PRE /* tag->parser == TY_(ParsePre) */ 
			|| tidyNodeGetId(node) == TidyTag_LISTING 
			|| tidyNodeGetId(node) == TidyTag_PLAINTEXT 
			|| tidyNodeGetId(node) == TidyTag_XMP
			)
        {
            tidyPCondFlushLine(doc, indent);

            indent = 0;
            tidyPCondFlushLine(doc, indent);

            PrintSgmlTag(doc, mode, indent, node, SgmlTagStart);
            tidyPFlushLine(doc, indent);

            for (content = tidyGetChild(node);
                    content != NULL;
                    content = tidyGetNext(content))
			{
                PrintSgml(doc, (mode | TidyTextFormat_Preformatted | TidyTextFormat_NoWrap), 
                            indent, content);
			}

            tidyPCondFlushLine(doc, indent);
            PrintSgmlTag(doc, mode, indent, node, SgmlTagEnd);
            tidyPFlushLine(doc, indent);

            if ( !tidyOptGetBool(doc, TidyIndentContent) && tidyGetNext(node) != NULL )
                tidyPFlushLine(doc, indent);
        }
        else if (tidyNodeGetModel(node) & CM_INLINE)
        {   PrintSgmlTag(doc, mode, indent, node, SgmlTagStart);

            if (tidyShouldIndent(doc, node))
            {
                tidyPCondFlushLine(doc, indent);
                indent += spaces;

                for (content = tidyGetChild(node);
                        content != NULL;
                        content = tidyGetNext(content))
				{
                    PrintSgml(doc, mode, indent, content);
				}

                tidyPCondFlushLine(doc, indent);
                indent -= spaces;
                tidyPCondFlushLine(doc, indent);
            }
            else
            {
	            for (content = tidyGetChild(node);
                        content != NULL;
                        content = tidyGetNext(content))
				{
                    PrintSgml(doc, mode, indent, content);
				}
            }

            PrintSgmlTag(doc, mode, indent, node, SgmlTagEnd);
        }
        else 
        {   tidyPCondFlushLine(doc, indent);
/*            if (TidySmartIndent && tidyGetPrev(node) != NULL)
                tidyPFlushLine(doc, indent);
*/
            PrintSgmlTag(doc, mode ,indent, node, SgmlTagStart);
            if (tidyShouldIndent(doc, node))
			{
                tidyPCondFlushLine(doc, indent);
			}
            else if (tidyNodeGetModel(node) & CM_HTML || 
                     tidyNodeIsNOFRAMES(node) ||
                    (tidyNodeGetModel(node) & CM_HEAD && !(tidyNodeIsTITLE(node))))
			{
                tidyPFlushLine(doc, indent);
			}

            if (tidyShouldIndent(doc, node))
            {   tidyPCondFlushLine(doc, indent);
                indent += spaces;

                for (content = tidyGetChild(node);
                        content != NULL;
                        content = tidyGetNext(content))
				{
                    PrintSgml(doc, mode, indent, content);
				}
                tidyPCondFlushLine(doc, indent);
                indent -= spaces;
                tidyPCondFlushLine(doc, indent);
            }
            else
            {   TidyNode last;
                last = NULL;
                for (content = tidyGetChild(node);
                        content != NULL;
                        content = tidyGetNext(content)) {
                    /* kludge for naked text before block level tag */
                    if (last && !tidyOptGetBool(doc, TidyIndentContent) && 
                        tidyNodeGetType(last) == TextNode &&
                        !(tidyNodeGetModel(content) & CM_INLINE) )
                    {
                        /* tidyPFlushLine(doc, indent); */
                        tidyPFlushLine(doc, indent);
                    }

                    PrintSgml(doc, mode, 
                        (tidyShouldIndent(doc, node) ? indent+spaces : indent), 
                        content);
                    last = content;
                }
            }
            PrintSgmlTag(doc, mode, indent, node, SgmlTagEnd);
            tidyPFlushLine(doc, indent);
            if (!tidyOptGetBool(doc, TidyIndentContent) &&
                tidyGetNext(node) != NULL &&
                !tidyOptGetBool(doc, TidyHideEndTags) &&
                (tidyNodeGetModel(node) & (CM_BLOCK|CM_LIST|CM_DEFLIST|CM_TABLE)))
			{
                tidyPFlushLine(doc, indent);
			}
        }
    }
}

