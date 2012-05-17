/* This file is part of the YAZ toolkit.
 * Copyright (C) 1995-2012 Index Data
 * See the file LICENSE for details.
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include <yaz/test.h>
#include <yaz/log.h>
#include <yaz/rpn2solr.h>
#include <yaz/wrbuf.h>
#include <yaz/pquery.h>

static int compare(solr_transform_t ct, const char *pqf, const char *solr)
{
    int ret = 0;
    ODR odr = odr_createmem(ODR_ENCODE);
    WRBUF w = wrbuf_alloc();
    Z_RPNQuery *q = p_query_rpn(odr, pqf);

    if (q)
    {
        int r = solr_transform_rpn2solr_wrbuf(ct, w, q);

        if (r != 0)
        {
            /* transform error */
            yaz_log(YLOG_LOG, "%s -> Error %d", pqf, r);
            if (!solr) /* also expected error? */
                ret = 1;
        }
        else if (r == 0)
        {
            yaz_log(YLOG_LOG, "%s -> %s", pqf, wrbuf_cstr(w));
            if (solr && !strcmp(wrbuf_cstr(w), solr))
            {
                ret = 1;
            }
        }
    }
    wrbuf_destroy(w);
    odr_destroy(odr);
    return ret;
}

static void tst1(void)
{
    solr_transform_t ct = solr_transform_create();

    YAZ_CHECK(compare(ct, "abc", "abc"));
    YAZ_CHECK(compare(ct, "\"a b c\"", "\"a b c\""));
    YAZ_CHECK(compare(ct, "@not a b", "a AND NOT b"));
    YAZ_CHECK(compare(ct, "@and @or a b c", "(a OR b) AND c"));
    YAZ_CHECK(compare(ct, "@and a b", "a AND b"));
    YAZ_CHECK(compare(ct, "@or a b", "a OR b"));
    YAZ_CHECK(compare(ct, "@attr 1=field abc", "field:abc"));
    YAZ_CHECK(compare(ct, "@attr 1=field \"a b c\"", "field:\"a b c\""));
    YAZ_CHECK(compare(ct, "@attr 1=4 abc", 0)); /* should fail */

    solr_transform_define_pattern(ct, "index.title", "1=4");
    YAZ_CHECK(compare(ct, "@attr 1=4 abc", "title:abc"));

    solr_transform_define_pattern(ct, "index.foo", "1=bar");
    YAZ_CHECK(compare(ct, "@attr 1=bar abc", "foo:abc"));

    /* Truncation */
    YAZ_CHECK(compare(ct, "@attr 5=1 water", "water*"));
    YAZ_CHECK(compare(ct, "@attr 5=2 water", 0));
    YAZ_CHECK(compare(ct, "@attr 5=3 water", 0));
    YAZ_CHECK(compare(ct, "@attr 5=100 water", "water"));
    YAZ_CHECK(compare(ct, "@attr 5=101 water", 0));
    YAZ_CHECK(compare(ct, "@attr 5=104 w#ter", "w?ter"));
    YAZ_CHECK(compare(ct, "@attr 5=104 w#t?r", "w?t*r"));
    YAZ_CHECK(compare(ct, "@attr 5=104 w#te?", "w?te*"));
    YAZ_CHECK(compare(ct, "@attr 5=104 w\\#te?", "w?te*")); /* PQF eats # */
    YAZ_CHECK(compare(ct, "@attr 5=104 w\\\\#te?", "w#te*"));

    /* reserved characters */
    YAZ_CHECK(compare(ct, "@attr 5=104 \\\"\\\\\\\\",
                      "\\\"" "\\\\"));
    YAZ_CHECK(compare(ct, "@attr 5=104 \\\"\\\\\\\\\\\"",
                      "\\\"" "\\\\" "\\\""));
    YAZ_CHECK(compare(ct, "@attr 5=104 \\\"\\\\", "\\\"\\\\"));

    YAZ_CHECK(compare(ct, "@attr 5=104 \\{:\\}", "\\{\\:\\}"));

    YAZ_CHECK(compare(ct, "@attr 5=104 \\\"\\\\\\\\\\\"",
                      "\\\"" "\\\\" "\\\""));

    solr_transform_close(ct);
}

static void tst2(void)
{
    WRBUF w = wrbuf_alloc();
    solr_transform_t ct = 0;
    const char *srcdir = getenv("srcdir");
    if (srcdir)
    {
        wrbuf_puts(w, srcdir);
        wrbuf_puts(w, "/");
    }
    wrbuf_puts(w, "../etc/pqf.properties");
    
    ct = solr_transform_open_fname(wrbuf_cstr(w));
    YAZ_CHECK(compare(ct, "@attr 1=4 abc", "dc.title:abc"));
#if 0
    YAZ_CHECK(compare(ct, "@attr 1=4 @attr 4=108 abc", "dc.title:abc"));
#endif

    YAZ_CHECK(compare(ct, "@attr 1=4 @attr 3=1 @attr 6=1 abc", "dc.title:abc"));
    YAZ_CHECK(compare(ct, "@attr 1=4 @attr 4=1 @attr 6=1 abc", "dc.title:abc"));
#if 0
    YAZ_CHECK(compare(ct, "@attr 1=1016 abc", "abc"));
    YAZ_CHECK(compare(ct, "@attr 2=1 @attr 1=30 1980", "dc.date:[* to 1980]"));
    YAZ_CHECK(compare(ct, "@attr 1=30 @attr 2=3 1980", "dc.date:1980"));
    YAZ_CHECK(compare(ct, "@attr 1=30 @attr 2=5 1980", "dc.date:[* to 1980]"));
    YAZ_CHECK(compare(ct, "@attr 1=30 @attr 2=2 1980", "dc.date:[* to 1980]"));
    YAZ_CHECK(compare(ct, "@attr 1=30 @attr 2=4 1980", "dc.date:[1980 to *]"));

    YAZ_CHECK(compare(ct, "@attr 2=103 @attr 1=_ALLRECORDS 1", "solr.allRecords=1"));
#endif
    YAZ_CHECK(compare(ct, "@attr 1=500 abc", 0));
    solr_transform_close(ct);
    wrbuf_destroy(w);
}

int main (int argc, char **argv)
{
    YAZ_CHECK_INIT(argc, argv);
    YAZ_CHECK_LOG();
    tst1();
    tst2();
    YAZ_CHECK_TERM;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

