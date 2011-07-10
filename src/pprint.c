/*
  pprint.c -- pretty print in sgml
  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pprint.h"
#include "tidy-int.h"
#include "parser.h"
#include "entities.h"
#include "tmbstr.h"
#include "utf8.h"
#include "main.h"

void PPrintDocType( TidyDocImpl* doc, uint indent, Node *node )
{
    if(dbsgml)
        PPrintString(doc, indent, "<!DOCTYPE article PUBLIC \"-//OASIS//DTD DocBook V4.1//EN\">\n");
    else if(dbxml)
        PPrintString(doc, indent, "<?xml version='1.0'?>\n<!DOCTYPE book PUBLIC \"-//OASIS//DTD DocBook XML V4.2//EN\">\n");
}

void PrintSgmlDefault()
{
    char *str = "SGML cannot contain these elements";

    fprintf(stderr, str);
}

void PrintSgmlBodyStart(TidyDocImpl* doc, uint indent)
{
    char *str = "<article>";
    PPrintString(doc, indent, str);
}

#define DIGIT(c) (c - 48)
#define TOTAL_H 6
static Bool seen_h[TOTAL_H] = {no, no, no, no, no, no};

/* Yuck ugly. FIXME */
#define SECT(i) (i - startsect)
static int startsect = 0; /* We are at level 0(H1) initially */

void PrintSgmlBodyEnd(TidyDocImpl *doc, uint indent)
{   int i = TOTAL_H - 1;
    char str[10];

    while(i >= 0) {
        if(seen_h[i] == yes) {
            if(i == 5)
                sprintf(str, "</simplesect>");
            else
                sprintf(str, "</sect%d>", SECT(i) + 1);
            PPrintString(doc, indent, str);
            seen_h[i] = no;
        }
        --i;
    }
    
    sprintf(str, "</article>");
    PPrintString(doc, indent, str);
}

char *GetContent(Lexer *lexer, Node *node)
{   Node *content, *temp_node;
    char *str, *temp;
    Bool flag = no;
    int i;

    content = node->content;

    /* Find the <a> tag */
    for (temp_node = content;
         temp_node && !nodeIsA(temp_node); 
         temp_node = temp_node->next)
         ;
    
    if(temp_node == NULL) { /* There is no <a> .. </a> tag */
        /* Discard all elements which are not text nodes */
        temp_node = content;
        for (temp_node = content;
             temp_node && temp_node->type != TextNode; 
             temp_node = temp_node->next)
           ;
        if(temp_node == NULL) { /* There's no TextNode either */ 
            str = MemAlloc(1);
            str[0] = '\0';
            return str;
        }
    }
    content = temp_node;

    if(content->type == TextNode)  {
        int size = content->end - content->start;
        
        str = MemAlloc(size + 1);
        str[size] = '\0';
        tmbstrncpy(str, lexer->lexbuf + content->start, size);
    }
    else if(nodeIsA(content)){
        AttVal *name;
        int size;

        name = GetAttrByName(content, "name");
        if(name == NULL)
            name = GetAttrByName(content, "href");

        if(name == NULL) {  /* No href or name, let's take empty id */
            size = 0;
            str = MemAlloc(size + 1);
            str[size] = '\0';
        }
        else {
            size = tmbstrlen(name->value);
            str = MemAlloc(size + 1);
            str[size] = '\0';
            tmbstrncpy(str, name->value, size);
       }
    }

    temp = str;
    if(str[0] == '#')
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

    /* This might create problem with linkends */
#if 0
    /* ID cannot start with '_' */
    if(str[0] = '_')
        str[0] = 'a';
#endif
    return str;
}

void PrintSectTag( TidyDocImpl *doc, uint indent, Node *node, uint startsect)
{   char sectnum = node->element[1];
    char str[100];

    char *id = GetContent(doc->lexer, node);

    if(sectnum == '6')  /* there's no sect6. We can do variety of
                           things here. may be <section> .. */
        sprintf(str, "<simplesect id=\"%s\"><title>", id); 
    else 
        sprintf(str, "<sect%c id=\"%s\"><title>", SECT(sectnum), id); 
    PPrintString(doc, indent, str);
    MemFree(id);
}

Bool ImmediateDescendantOfHTags(Node *element)
{   Node *parent = element->parent;

    if (strlen(parent->element) == 2 && 
            parent->element[0] == 'h' && 
            IsDigit(parent->element[1]))
        return yes;
    return no;
}
void PrintSgmlLink(TidyDocImpl *doc, uint indent, Node *node)
{   AttVal *addr;
    char str[500];  /* FIXME allocate dynamically later */

    addr = GetAttrByName(node, "name");
    if(addr == NULL) {
        addr = GetAttrByName(node, "href");
        if(!ImmediateDescendantOfHTags(node)) {
            if(addr->value[0] == '#') 
                sprintf(str, "<link linkend=\"%s\">", addr->value + 1); 
            else
                sprintf(str, "<ulink url=\"%s\">", addr->value); 
            if( !DescendantOf(node, TidyTag_P) && 
                /* <programlisting> doesn't allow <para> */
                !DescendantOf(node, TidyTag_PRE) &&
                 node->prev && node->prev->type == TextNode)
                PPrintString(doc, indent, "<para>");
            PPrintString(doc, indent, str);
        }
    }
    else {
        if(!ImmediateDescendantOfHTags(node)) {
            if(!DescendantOf(node, TidyTag_P)) 
                sprintf(str, "<para id=\"%s\">", addr->value); 
            else /* We cannnot have a <para> inside another <para> */
                sprintf(str, "<anchor id=\"%s\"/>", addr->value); 
            PPrintString(doc, indent, str);
        }
    }
}

void PrintSgmlLinkEnd(TidyDocImpl *doc, uint indent, Node *node)
{   AttVal *addr;

    addr = GetAttrByName(node, "name");
    if(addr == NULL) {
        addr = GetAttrByName(node, "href");
        if(!ImmediateDescendantOfHTags(node)) {
            if(addr->value[0] == '#') 
                PPrintString(doc, indent, "</link>");
            else
                PPrintString(doc, indent, "</ulink>");
            if( !DescendantOf(node, TidyTag_P) && 
                 /* <programlisting> doesn't allow <para> */
                !DescendantOf(node, TidyTag_PRE) &&
                 node->prev && node->prev->type == TextNode)
                PPrintString(doc, indent, "</para>");
        }
    }
    else {
        if(!ImmediateDescendantOfHTags(node)) {
            if(!DescendantOf(node, TidyTag_P))
                PPrintString(doc, indent, "</para>");
            /* else
               <anchor ..  /> has already been placed. no need to
               do any thing */
        }
    }
}


void PrintSgmlTagString(TidyDocImpl *doc, uint mode, uint indent, 
                        SgmlTagType sgmltag_type, char *str)
{   PPrintChar(doc, str[0], mode | CDATA);
    if(sgmltag_type == SgmlTagEnd)
        PPrintChar(doc, '/', mode);
    PPrintString(doc, indent, str + 1);
}

void PrintSgmlList(TidyDocImpl *doc, uint indent, uint mode, Node *node)
{   if(nodeIsUL(node))
        PPrintString(doc, indent, "<itemizedlist>");
    else if(nodeIsOL(node))
        PPrintString(doc, indent, "<orderedlist>");
    else if(nodeIsDL(node))
        PPrintString(doc, indent, "<variablelist>");
}

void PrintSgmlListEnd(TidyDocImpl *doc, uint indent, uint mode, Node *node)
{   if(nodeIsUL(node))
        PPrintString(doc, indent, "</itemizedlist>");
    else if(nodeIsUL(node))
        PPrintString(doc, indent, "</orderedlist>");
    else if(nodeIsDL(node))
        PPrintString(doc, indent, "</variablelist>");
}

void PrintSgmlListItem(TidyDocImpl *doc, uint indent, Node *node)
{   if(nodeIsLI(node))
        PPrintString(doc, indent, "<listitem>");
    else if(nodeIsDD(node))
        PPrintString(doc, indent, "<listitem>");
}

void PrintSgmlListItemEnd(TidyDocImpl *doc, uint indent, Node *node)
{   if(nodeIsLI(node))
        PPrintString(doc, indent, "</listitem>");
    else if(nodeIsDD(node))
        PPrintString(doc, indent, "</listitem></varlistentry>");
}

void PrintSgmlImage(TidyDocImpl *doc, uint indent, Node *node)
{   AttVal *addr;
    char str[100];

    addr = GetAttrByName(node, "src");
    /* We can get other attributes like width, height etc.. */
    if(addr != NULL) {
        PPrintString(doc, indent, "<inlinemediaobject><imageobject>");
        PCondFlushLine(doc, indent);
        sprintf(str, "<imagedata fileref=\"%s\">", addr->value); 
        PPrintString(doc, indent, str);
        PCondFlushLine(doc, indent);
        PPrintString(doc, indent, "</imageobject></inlinemediaobject>");
        PCondFlushLine(doc, indent);
    }
}

int CountColumns(Node *node)
{   Node *temp, *row_content;
    int ncols = 0;

    temp = node->content;

    /* FIXME */
    /* Perhaps this is not needed, check with HTML standard later */
    /*
    while(nodeIsTR(temp))
        temp = temp->next;
    */
    /* This can contain th or td's */
    row_content = temp->content;
    while(row_content) {
        if(nodeIsTH(row_content) || nodeIsTD(row_content)) {
            AttVal *colspan;
            
            colspan = GetAttrByName(row_content, "colspan");
            if(colspan)
                ncols += atoi(colspan->value);
            else
                ++ncols;
        }
        else
            fprintf(stderr, "PrintSgml: error in table processing\n");
        row_content = row_content->next;
   } 
   return ncols;
}

void PrintSgmlTable(TidyDocImpl *doc, uint indent, Node *node)
{   int ncols;
    char str[100];

    PPrintString(doc, indent, "<informaltable>");
    ncols = CountColumns(node);
    sprintf(str, "<tgroup cols=\"%d\"><tbody>", ncols);
    PPrintString(doc, indent, str);
}

void PrintSgmlTableEnd(TidyDocImpl *doc, uint indent, Node *node)
{   
    PPrintString(doc, indent, "</tbody></tgroup></informaltable>");
}

Bool DescendantOfAddress(Node *element)
{
    Node *parent;
    
    for (parent = element->parent;
            parent != null; parent = parent->parent)
    {   if (parent->element && tmbstrcasecmp(parent->element, "address") == 0)
            return yes;
    }

    return no;
}

void PrintSgmlTag( TidyDocImpl *doc, uint mode, uint indent, Node *node,
                    SgmlTagType sgmltag_type)
{   static int level = -1;

    if(nodeIsHTML(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlBodyStart(doc, indent);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlBodyEnd(doc, indent);
    }
    else if(nodeIsHEAD(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<articleinfo>");
    else if(nodeIsTITLE(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<title>");
    /* May be we can replace with node->model & CM_LIST */
    else if(nodeIsUL(node) || nodeIsOL(node)|| nodeIsDL(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlList(doc, indent, mode, node);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlListEnd(doc, indent, mode, node);
    }
    else if(nodeIsDT(node)) {
        if(sgmltag_type == SgmlTagStart)
            PPrintString(doc, indent, "<varlistentry><term>");
        else if(sgmltag_type == SgmlTagEnd)
            PPrintString(doc, indent, "</term>");
    }
    else if(nodeIsLI(node) || nodeIsDD(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlListItem(doc, indent, node);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlListItemEnd(doc, indent, node);
    }
    /* Later we should clean this before coming to PrintSgml */
    else if(nodeIsP(node) && 
            /* Table <entry> processing */
            !DescendantOf(node, TidyTag_TH) && !DescendantOf(node, TidyTag_TD) && 
            !DescendantOfAddress(node) && 
            /* <programlisting> doesn't allow <para> */
            !DescendantOf(node, TidyTag_PRE)) 
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<para>");
    else if(nodeIsBLOCKQUOTE(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<blockquote>");
    else if(nodeIsPRE(node) && 
            /* Table <entry> processing */
            !DescendantOf(node, TidyTag_TH) && !DescendantOf(node, TidyTag_TD))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, 
                               "<programlisting>");
    else if(nodeIsA(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlLink(doc, indent, node);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlLinkEnd(doc, indent, node);
    }
    /* Table would require more processing */
    else if(nodeIsTABLE(node)) {
        if(sgmltag_type == SgmlTagStart)
            PrintSgmlTable(doc, indent, node);
        else if(sgmltag_type == SgmlTagEnd)
            PrintSgmlTableEnd(doc, indent, node);
    }
    else if(nodeIsTR(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<row>");
    else if(nodeIsTD(node) || nodeIsTH(node))
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, "<entry>");
    else if(nodeIsIMG(node)) { /* This is a StartEndTag */
        if(sgmltag_type == SgmlTagStart)    
            PrintSgmlImage(doc, indent, node);
    }

    else if(tmbstrcasecmp(node->element, "cite") == 0)
        PrintSgmlTagString(doc, mode, indent, sgmltag_type, 
                                "<citation>");
    /* We should distinguish tag_strong and tag_em later 
       haven't found proper docbook tag for <strong> */
    else if(nodeIsEM(node) || nodeIsSTRONG(node) ||
            tmbstrcasecmp(node->element, "address") == 0) {
        if(sgmltag_type == SgmlTagStart) {
            if(DescendantOf(node, TidyTag_P) || DescendantOf(node, TidyTag_PRE))
                PPrintString(doc, indent, "<emphasis>");
            else
                PPrintString(doc, indent, "<para><emphasis>");
        }
        else if(sgmltag_type == SgmlTagEnd) {
            if(DescendantOf(node, TidyTag_P) || DescendantOf(node, TidyTag_PRE))
                PPrintString(doc, indent, "</emphasis>");
            else 
                PPrintString(doc, indent, "</emphasis></para>");
        }
    }
    else {
        if(tmbstrcasecmp(node->element, "code") == 0 &&
            !(nodeIsDD(node->parent) || nodeIsLI(node->parent)))
                PrintSgmlTagString(doc, mode, indent, 
                                sgmltag_type, "<literal>");
        else if(strlen(node->element) == 2 && 
                node->element[0] == 'h' && 
                IsDigit(node->element[1])) {
            if(sgmltag_type == SgmlTagStart) {        
                int sectnum = DIGIT(node->element[1]) - 1;
                char str[10];
                if(seen_h[sectnum] == no)
                    seen_h[sectnum] = yes;
                else {
                    int i = level;
                    while(i > sectnum && seen_h[i] == yes) {
                        if(i == 5)
                            sprintf(str, "</simplesect>");
                        else
                            sprintf(str, "</sect%d>", SECT(i) + 1);
                        PPrintString(doc, indent, str);
                        seen_h[i] = no;
                        --i;
                    }
                    if(sectnum == 5)
                        sprintf(str, "</simplesect>");
                    else
                        sprintf(str, "</sect%d>", SECT(sectnum) + 1);
                    PPrintString(doc, indent, str);
                }
                /* H1 is not the first level 
                   like the curses man2html pages */
                if(level == -1 && sectnum > 0) 
                    startsect = sectnum;
                
                PrintSectTag(doc, indent, node, startsect);
                level = sectnum;
            }
            else
                PPrintString(doc, indent, "</title>");
        }
    }
}

void PrintSgml( TidyDocImpl* doc, uint mode, uint indent, Node *node )
{   Node *content;
    Bool xhtml = cfgBool( doc, TidyXhtmlOut);
    uint spaces = cfg( doc, TidyIndentSpaces );

    if (node == null)
        return;
    
    if (node->type == TextNode) {
        if(DescendantOf(node, TidyTag_DD) && !DescendantOf(node, TidyTag_A) &&
            !DescendantOf(node, TidyTag_P) && 
            /* We have to descide on this table stuff later
             * <entry> processing is complex */
            !DescendantOf(node, TidyTag_TD) && !DescendantOf(node, TidyTag_TH) &&
            /* <programlisting> doesn't allow <para> */
            !DescendantOf(node, TidyTag_PRE)    
            ) 
        /*  && wstrcasecmp(node->parent->element, "code") != 0)
         above line may be needed later to properly convert <code> stuff */
        {
            PPrintString(doc, indent, "<para>");
            PPrintText(doc, mode, indent, node);
            PPrintString(doc, indent, "</para>");
        }
        else {
            if(DescendantOf(node, TidyTag_STYLE))
                fprintf(stderr, "PrintSgml: skipping style elements\n\n");
            else
                PPrintText(doc, mode, indent, node);
        }
    }
    else if(node->type == CDATATag && TidyEscapeCdata)
        PPrintText(doc, mode, indent, node);
    else if (node->type == CommentTag)
        PPrintComment(doc, indent, node);
    else if (node->type == RootNode)
    {
        for (content = node->content;
                content != null;
                content = content->next)
           PrintSgml(doc, mode, indent, content);
    }
    else if (node->type == DocTypeTag)
        PPrintDocType(doc, indent, node);
    else if (node->type == CDATATag)
        PPrintCDATA(doc, indent, node);
    else if (node->type == SectionTag)
        PPrintSection(doc, indent, node);
    else if (node->type == AspTag || 
             node->type == JsteTag ||
             node->type == PhpTag )
        PrintSgmlDefault();
    else if (node->type == ProcInsTag)
        PPrintPI(doc, indent, node);
    else if (node->type == XmlDecl)// && DbXml May be this is needed
        PPrintXmlDecl(doc, indent, node);
    else if (node->tag->model & CM_EMPTY || 
            (node->type == StartEndTag && !xhtml))
    {
        if (!(node->tag->model & CM_INLINE))
            PCondFlushLine(doc, indent);

        if ( cfgBool(doc, TidyMakeClean) && nodeIsWBR(node) )
            PPrintString(doc, indent, " ");
        else
            PrintSgmlTag(doc, mode, indent, node, SgmlTagStart);
    }
    else {
        if (node->type == StartEndTag)
            node->type = StartTag;
        
        if (node->tag && node->tag->parser == ParsePre)
        {
            PCondFlushLine(doc, indent);

            indent = 0;
            PCondFlushLine(doc, indent);

            PrintSgmlTag(doc, mode, indent, node, SgmlTagStart);
            PFlushLine(doc, indent);

            for (content = node->content;
                    content != null;
                    content = content->next)
                PrintSgml(doc, (mode | PREFORMATTED | NOWRAP), 
                            indent, content);
            
            PCondFlushLine(doc, indent);
            PrintSgmlTag(doc, mode, indent, node, SgmlTagEnd);
            PFlushLine(doc, indent);

            if ( !cfg(doc, TidyIndentContent) && node->next != null )
                PFlushLine(doc, indent);
        }
        else if (node->tag->model & CM_INLINE)
        {   PrintSgmlTag(doc, mode, indent, node, SgmlTagStart);

            if (ShouldIndent(doc, node))
            {
                PCondFlushLine(doc, indent);
                indent += spaces;

                for (content = node->content;
                        content != null;
                        content = content->next)
                    PrintSgml(doc, mode, indent, content);

                PCondFlushLine(doc, indent);
                indent -= spaces;
                PCondFlushLine(doc, indent);
            }
            else
            {

                for (content = node->content;
                        content != null;
                        content = content->next)
                    PrintSgml(doc, mode, indent, content);
            }

            PrintSgmlTag(doc, mode, indent, node, SgmlTagEnd);
        }
        else 
        {   PCondFlushLine(doc, indent);
/*            if (TidySmartIndent && node->prev != null)
                PFlushLine(doc, indent);
*/
            PrintSgmlTag(doc, mode ,indent, node, SgmlTagStart);
            if (ShouldIndent(doc, node))
                PCondFlushLine(doc, indent);
            else if (node->tag->model & CM_HTML || 
                     nodeIsNOFRAMES(node) ||
                    (node->tag->model & CM_HEAD && !(nodeIsTITLE(node))))
                PFlushLine(doc, indent);

            if (ShouldIndent(doc, node))
            {   PCondFlushLine(doc, indent);
                indent += spaces;

                for (content = node->content;
                        content != null;
                        content = content->next)
                    PrintSgml(doc, mode, indent, content);
                PCondFlushLine(doc, indent);
                indent -= spaces;
                PCondFlushLine(doc, indent);
            }
            else
            {   Node *last;
                last = null;
                for (content = node->content;
                        content != null;
                        content = content->next) {
                    /* kludge for naked text before block level tag */
                    if (last && !cfg(doc, TidyIndentContent) && 
                        last->type == TextNode &&
                        content->tag && !(content->tag->model & CM_INLINE) )
                    {
                        /* PFlushLine(doc, indent); */
                        PFlushLine(doc, indent);
                    }

                    PrintSgml(doc, mode, 
                        (ShouldIndent(doc, node) ? indent+spaces : indent), 
                        content);
                    last = content;
                }
            }
            PrintSgmlTag(doc, mode, indent, node, SgmlTagEnd);
            PFlushLine(doc, indent);
            if (cfg(doc, TidyIndentContent) == no &&
                node->next != null &&
                TidyHideEndTags == no &&
                (node->tag->model & (CM_BLOCK|CM_LIST|CM_DEFLIST|CM_TABLE)))
                PFlushLine(doc, indent);
        }
    }
}
