#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "partial_read.h"

#define err(st, args...)        do{fprintf(stderr, "%s[%u] " st, __func__, __LINE__, ##args); }while(0)

static char *g_pInName = 0;
static char *g_pOutName = 0;
static char *g_pOutCMakeName = 0;
static char *g_pOutBatName = 0;

#if 0 //ndef strncasecmp
static int strncasecmp(const char *s1, const char *s2, size_t n)
{
    int ret;

    if (n == 0)
        return 0;

    while ((ret = ((unsigned char) tolower (*s1)
                   - (unsigned char) tolower (*s2))) == 0
            && *s1++)
    {
        if (--n == 0)
            return 0;
        ++s2;
    }
    return ret;
}
#endif

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

static void gen_uid(char *pGuidStr, int GuidStrLen)
{
    char    *pDefStrTable = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    char    *pActStrTable = 0;

    do {
        int         i;
        size_t      table_len = strlen(pDefStrTable);
        if( !(pActStrTable = malloc(table_len + 1)) )
        {
            err("malloc (%d) fail !!\n", table_len + 1);
            break;
        }
        memset(pActStrTable, 0x0, table_len + 1);
        memcpy(pActStrTable, pDefStrTable, table_len);

        for(i = 0; i < table_len - 1; i++)
        {
            int     j = rand() % (table_len  - i);
            char    tmp = pActStrTable[i];

            pActStrTable[i] = pActStrTable[j];
            pActStrTable[j] = tmp;
        }

        //Data1 - 8 characters.
        for(i = 0; i < 8; i++, pGuidStr++)
            *pGuidStr = pActStrTable[rand() % table_len];

        //Data2 - 4 characters.
        *pGuidStr++ = '_';
        for(i = 0; i < 4; i++, pGuidStr++)
            *pGuidStr = pActStrTable[rand() % table_len];

        //Data3 - 4 characters.
        *pGuidStr++ = '_';
        for(i = 0; i < 4; i++, pGuidStr++)
            *pGuidStr = pActStrTable[rand() % table_len];

        //Data4 - 4 characters.
        *pGuidStr++ = '_';
        for(i = 0; i < 4; i++, pGuidStr++)
            *pGuidStr = pActStrTable[rand() % table_len];

        //Data5 - 12 characters.
        *pGuidStr++ = '_';
        for(i = 0; i < 12; i++, pGuidStr++)
            *pGuidStr = pActStrTable[rand() % table_len];

        *pGuidStr = L'\0';

    }while(0);

    if( pActStrTable )      free(pActStrTable);
    return;
}

static void usage(char **argv)
{
    static char str[] =
        "Copyright (c) 2018~ Wei-Lun Hsu. All rights reserved.\n"
        "Usage: %s [options]\n"
        "       -i: input config file\n"
        "       -oh: output header file\n"
        "       -om: output cmake file\n"
        "       -ob: output batch file\n"
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
        } else if (!strncmp(argv[0], "-oh", strlen("-oh"))) {
            argv++; argc--;
            g_pOutName = argv[0];
        } else if (!strncmp(argv[0], "-om", strlen("-om"))) {
            argv++; argc--;
            g_pOutCMakeName = argv[0];
        } else if (!strncmp(argv[0], "-ob", strlen("-ob"))) {
            argv++; argc--;
            g_pOutBatName = argv[0];
        } else {
            err("unknown option %s\n", argv[0]);
        }
        argv++; argc--;
    }
}

int main(int argc, char **argv)
{
    partial_read_t  hReader = {0};
    FILE            *fin = 0, *fout = 0, *foutm = 0, *foutb = 0;

    getparams(argc, argv);

    do {
        // open output
        if( !(fout = fopen(g_pOutName, "w")) )
        {
            err("open %s fail \n", g_pOutName);
            break;
        }

        { // uuid
            char    filename[128] = {0};
            char    uuid[128] = {0};
            char    *pTmp = 0;

            srand((unsigned int)time(NULL));

            snprintf(filename, 128, "%s", g_pOutName);
            pTmp = strchr(filename, '.');
            if( pTmp )      *pTmp = '\0';

            gen_uid(uuid, 128);

            fprintf(fout, "#ifndef __%s_H_%s__\n", filename, uuid);
            fprintf(fout, "#define __%s_H_%s__\n\n", filename, uuid);
        }

        fprintf(fout, "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");
        // fprintf(fout, "/**\n *  Automatically generated file; DO NOT EDIT.\n */\n");

        if( g_pOutCMakeName )
        {
            if( !(foutm = fopen(g_pOutCMakeName, "w")) )
            {
                err("open %s fail \n", g_pOutCMakeName);
                break;
            }
            // fprintf(foutm, "#\n#  Automatically generated file; DO NOT EDIT.\n#\n");
        }

        if( g_pOutBatName )
        {
            if( !(foutb = fopen(g_pOutBatName, "w")) )
            {
                err("open %s fail \n", g_pOutBatName);
                break;
            }
        }

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

                if( pAct_str[0] == '\n' || !strlen(pAct_str) )
                {
                    fprintf(fout, "\n");
                    if( foutm )
                        fprintf(foutm, "\n");
                    continue;
                }

                if( pAct_str[0] == '#' )
                {
                    if( !strstr(pAct_str, "is not set") )
                    {
                        fprintf(fout, "//%s\n", pAct_str + 1);
                        if( foutm )
                            fprintf(foutm, "%s\n", pAct_str);
                    }

                    continue;
                }

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

                if( !strncasecmp(pValue, "Y", 1) || !strncasecmp(pValue, "YES", 3) )
                {
                    fprintf(fout, "#define %s 1\n", pName);
                    if( foutm )
                        fprintf(foutm, "set(%s TRUE)\n", pName);

                    if( foutb )
                        fprintf(foutb, "set %s=y\n", pName);
                }
                else
                {
                    fprintf(fout, "#define %s %s\n", pName, pValue);
                    if( foutm )
                        fprintf(foutm, "set(%s %s)\n", pName, pValue);

                    if( foutb )
                    {
                        char    buf[128] = {0};
                        char    *pCur = 0;

                        snprintf(buf, 128, "%s", pValue);
                        pCur = strchr(buf, '\"');
                        if( pCur )
                        {
                            pCur++;
                            pValue = pCur;
                            while( (pCur = strchr(pValue, '\"' )) )
                                *pCur++ = '\0';

                        }

                        fprintf(foutb, "set %s=%s\n", pName, pValue);
                    }
                }
            }
        }

        fprintf(fout, "\n\n#ifdef __cplusplus\n}\n#endif\n\n");

    } while(0);

    if( hReader.pBuf )     free(hReader.pBuf);
    if( fin )   fclose(fin);
    if( fout )  fclose(fout);
    if( foutm ) fclose(foutm);
    if( foutb ) fclose(foutb);

    return 0;
}
