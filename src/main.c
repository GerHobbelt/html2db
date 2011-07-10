#include <tidy.h>
#include <buffio.h>
#include <stdio.h>
#include <errno.h>
#include <tidy-int.h>

#include "main.h"

char *progname;
void usage()
{
    printf("Usage: %s [option] <html file>\n", progname);
    printf("Options:\n");
    printf("\t-dbsgml\t\tDocbook SGML output\n");
    printf("\t-dbxml\t\tDocbook XML output\n");
    printf("\t-help\t\tPrint Help Message\n");
}

int main(int argc, char **argv )
{
    TidyDoc tdoc;
    TidyDocImpl *doc;
    StreamOut *out;
    int rc = -1;
    char *file = NULL;
    int i;
    progname = argv[0];

    dbxml = no;
    dbsgml = no;
    /* read command line */
    for(i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            ctmbstr arg = argv[i] + 1;
            if ( strcasecmp(arg, "dbxml") == 0) {
                dbxml = yes;
            }
            else if( strcasecmp(arg, "dbsgml") == 0) {
                dbsgml = yes;
            }
            else if( strcasecmp(arg, "help") == 0) {
                usage();
                exit(0);
            }
            else {
                printf("Error: Invalid option %s\n", argv[i]);
                usage();
                exit(0);
            }
        }
        else {
            file = argv[i];
            break;
        }
    }

    if(!dbsgml && !dbxml)
        dbsgml = yes;
    if(!file) {
        printf("Error: No input file specified\n");
        usage();
        exit(1);
    }

    tdoc = tidyCreate();                     // Initialize "document"
    tidyOptSetBool( tdoc, TidyXhtmlOut, yes );  // Convert to XHTML
    rc = tidyParseFile( tdoc, file);

    if ( rc >= 0 ) {
        tidyCleanAndRepair( tdoc ); 
        tidyRunDiagnostics( tdoc );
        tidyErrorSummary( tdoc );
        tidyGeneralInfo( tdoc );
        
        doc = tidyDocToImpl( tdoc );
        out = FileOutput(stdout, 0, '\n');
        doc->docOut = out;
        PrintSgml( doc, 0, 0, doc->root );
        doc->docOut = null;
    }
    else {
        fprintf(stderr, "Problem parsing file %s\n", file); 
    }
    tidyRelease( tdoc );
    return 0;
}
