#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "partial_read.h"

#define err(st, args...)        do{fprintf(stderr, "%s[%u] " st, __func__, __LINE__, ##args); }while(0)

static char *g_pInName = 0;
static char *g_pOutName = 0;

static int
_post_read(unsigned char *pBuf, int buf_size)
{
    int     i;
    for(i = 0; i < buf_size; ++i)
    {
        if( pBuf[i] == '\n' || pBuf[i] == '\r' )
            pBuf[i] = '\0';
    }
    return 0;
}

static void usage(char **argv)
{
    static char str[] =
        "Usage: %s [options]\n"
        "       -i: input config file\n"
        "       -o: output header file\n"
        "\n";

    fprintf(stderr, str, argv[0]);
}

static void getparams(int argc, char **argv)
{
    if (argc < 2) {
        usage(argv);
        exit(-1);
    }

    argv++; argc--;
    while(argc) {
        if (!strncmp(argv[0], "-i", strlen("-i"))) {
            argv++; argc--;
            g_pInName = argv[0];
        } else if (!strncmp(argv[0], "-o", strlen("-o"))) {
            argv++; argc--;
            g_pOutName = argv[0];
        } else {
            err("unknown option %s\n", argv[0]);
        }
        argv++; argc--;
    }
}

int main(int argc, char **argv)
{
    partial_read_t  hReader = {0};
    FILE            *fin = 0, *fout = 0;

    getparams(argc, argv);
    printf("%s, %s\n", g_pInName, g_pOutName);

    do {
        // open output
        if( !(fout = fopen(g_pOutName, "w")) )
        {
            err("open %s fail \n", g_pOutName);
            break;
        }

        fprintf(fout, "/**\n *  Automatically generated file; DO NOT EDIT.\n */\n");

        // open input
        if( !(fin = fopen(g_pInName, "r")) )
        {
            err("open %s fail \n", g_pInName);
            break;
        }

        hReader.fp       = fin;
        hReader.buf_size = 2048;
        if( !(hReader.pBuf = malloc(hReader.buf_size)) )
        {
            err("malloc %ld fail \n", hReader.buf_size);
            break;
        }
        hReader.pCur  = hReader.pBuf;
        hReader.pEnd  = hReader.pCur;

        fseek(hReader.fp, 0l, SEEK_END);
        hReader.file_size = ftell(hReader.fp);
        fseek(hReader.fp, 0l, SEEK_SET);

        hReader.file_remain = hReader.file_size;

        // activate
        partial_read__full_buf(&hReader, _post_read);
        while( hReader.pCur < hReader.pEnd )
        {
            if( partial_read__full_buf(&hReader, _post_read) )
            {
                break;
            }

            {   // start parsing a line
                int     rval = 0;
                char    *pAct_str = 0, *pName = 0, *pValue = 0;
                char    conf_name[128] = {0};
                char    conf_value[256] = {0};

                pAct_str = (char*)hReader.pCur;
                hReader.pCur += (strlen((char*)hReader.pCur) + 1);

                // clear prefix white space
                while( *pAct_str == '\t' || *pAct_str == ' ' )
                    pAct_str++;

                if( pAct_str[0] == '#' || pAct_str[0] == '\n' || !strlen(pAct_str) )
                    continue;

                pName  = conf_name;
                pValue = conf_value ;
                rval = sscanf(pAct_str, "%[^=]=%s", pName, pValue);
                if( rval == -1 )
                {
                    fprintf(stderr, "sscanf fail '%s' \n", pAct_str);
                    continue;
                }

                // clear prefix white space
                while( *pName == '\t' || *pName == ' ' )
                    pName++;

                // clear prefix white space
                while( *pValue == '\t' || *pValue == ' ' )
                    pValue++;

                // fprintf(stderr, "%s:%s\n", pName, pValue);

                if( !strncmp(pValue, "y", 1) || !strncmp(pValue, "yes", 3) )
                    fprintf(fout, "#define %s 1\n", pName);
                else
                    fprintf(fout, "#define %s %s\n", pName, pValue);
            }
        }

    } while(0);

    if( hReader.pBuf )     free(hReader.pBuf);
    if( fin )   fclose(fin);
    if( fout )  fclose(fout);

    return 0;
}
