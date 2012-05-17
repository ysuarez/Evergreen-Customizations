/* This file is part of the YAZ toolkit.
 * Copyright (C) 1995-2012 Index Data
 * See the file LICENSE for details.
 */
/**
 * \file odr_null.c
 * \brief Implements ODR NULL codec
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "odr-priv.h"

/*
 * Top level null en/decoder.
 * Returns 1 on success, 0 on error.
 */
int odr_null(ODR o, Odr_null **p, int opt, const char *name)
{
    int res, cons = 0;

    if (o->error)
        return 0;
    if (o->op->t_class < 0)
    {
        o->op->t_class = ODR_UNIVERSAL;
        o->op->t_tag = ODR_NULL;
    }
    res = ber_tag(o, p, o->op->t_class, o->op->t_tag, &cons, opt, name);
    if (res < 0)
        return 0;
    if (!res)
        return odr_missing(o, opt, name);
    if (o->direction == ODR_PRINT)
    {
        odr_prname(o, name);
        odr_printf(o, "NULL\n");
        return 1;
    }
    if (cons)
    {
#ifdef ODR_STRICT_NULL
        odr_seterror(OPROTO, 42);
        return 0;
#else
        /* Warning: Bad NULL */
#endif
    }
    if (o->direction == ODR_DECODE)
        *p = odr_nullval();
    return ber_null(o);
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

