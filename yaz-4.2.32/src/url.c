/* This file is part of the YAZ toolkit.
 * Copyright (C) 1995-2012 Index Data
 * See the file LICENSE for details.
 */
/**
 * \file url.c
 * \brief URL fetch utility
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaz/url.h>
#include <yaz/comstack.h>
#include <yaz/log.h>

struct yaz_url {
    ODR odr_in;
    ODR odr_out;
    char *proxy;
};

yaz_url_t yaz_url_create(void)
{
    yaz_url_t p = xmalloc(sizeof(*p));
    p->odr_in = odr_createmem(ODR_DECODE);
    p->odr_out = odr_createmem(ODR_ENCODE);
    p->proxy = 0;
    return p;
}

void yaz_url_destroy(yaz_url_t p)
{
    if (p)
    {
        odr_destroy(p->odr_in);
        odr_destroy(p->odr_out);
        xfree(p->proxy);
        xfree(p);
    }
}

void yaz_url_set_proxy(yaz_url_t p, const char *proxy)
{
    xfree(p->proxy);
    p->proxy = 0;
    if (proxy && *proxy)
        p->proxy = xstrdup(proxy);
}

Z_HTTP_Response *yaz_url_exec(yaz_url_t p, const char *uri,
                              const char *method,
                              Z_HTTP_Header *headers,
                              const char *buf, size_t len)
{
    Z_HTTP_Response *res = 0;
    int number_of_redirects = 0;

    while (1)
    {
        void *add;
        COMSTACK conn = 0;
        int code;
        struct Z_HTTP_Header **last_header_entry;
        const char *location = 0;
        Z_GDU *gdu = z_get_HTTP_Request_uri(p->odr_out, uri, 0,
                                            p->proxy ? 1 : 0);
        gdu->u.HTTP_Request->method = odr_strdup(p->odr_out, method);

        res = 0;
        last_header_entry = &gdu->u.HTTP_Request->headers;
        while (*last_header_entry)
            last_header_entry = &(*last_header_entry)->next;
        *last_header_entry = headers; /* attach user headers */

        if (buf && len)
        {
            gdu->u.HTTP_Request->content_buf = (char *) buf;
            gdu->u.HTTP_Request->content_len = len;
        }
        if (!z_GDU(p->odr_out, &gdu, 0, 0))
        {
            yaz_log(YLOG_WARN, "Can not encode HTTP request URL:%s", uri);
            return 0;
        }
        conn = cs_create_host_proxy(uri, 1, &add, p->proxy);
        if (!conn)
        {
            yaz_log(YLOG_WARN, "Bad address for URL:%s", uri);
        }
        else if (cs_connect(conn, add) < 0)
        {
            yaz_log(YLOG_WARN, "Can not connect to URL:%s", uri);
        }
        else
        {
            int len;
            char *buf = odr_getbuf(p->odr_out, &len, 0);
            
            if (cs_put(conn, buf, len) < 0)
                yaz_log(YLOG_WARN, "cs_put failed URL:%s", uri);
            else
            {
                char *netbuffer = 0;
                int netlen = 0;
                int cs_res = cs_get(conn, &netbuffer, &netlen);
                if (cs_res <= 0)
                {
                    yaz_log(YLOG_WARN, "cs_get failed URL:%s", uri);
                }
                else
                {
                    Z_GDU *gdu;
                    odr_setbuf(p->odr_in, netbuffer, cs_res, 0);
                    if (!z_GDU(p->odr_in, &gdu, 0, 0)
                        || gdu->which != Z_GDU_HTTP_Response)
                    {
                        yaz_log(YLOG_WARN, "HTTP decoding failed "
                                "URL:%s", uri);
                    }
                    else
                    {
                        res = gdu->u.HTTP_Response;
                    }
                }
                xfree(netbuffer);
            }
        }
        if (conn)
            cs_close(conn);
        if (!res)
            break;
        code = res->code;
        location = z_HTTP_header_lookup(res->headers, "Location");
        if (++number_of_redirects < 10 &&
            location && (code == 301 || code == 302 || code == 307))
        {
            odr_reset(p->odr_out);
            uri = odr_strdup(p->odr_out, location);
            odr_reset(p->odr_in);
        }
        else
            break;
    }
    return res;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

