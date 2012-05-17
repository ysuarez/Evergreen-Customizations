/* This file is part of the YAZ toolkit.
 * Copyright (C) 1995-2012 Index Data
 * See the file LICENSE for details.
 */

/** 
 * \file ber_null.c
 * \brief Implements ber_null
 *
 * This source file implements BER encoding and decoding of
 * the NULL type.
 */
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "odr-priv.h"

/** 
 * ber_null: BER-en/decoder for NULL type.
 */
int ber_null(ODR o)
{
    switch (o->direction)
    {
    case ODR_ENCODE:
        if (odr_putc(o, 0X00) < 0)
            return 0;
        return 1;
    case ODR_DECODE:
        if (odr_max(o) < 1)
        {
            odr_seterror(o, OPROTO, 39);
            return 0;
        }
        if (*(o->bp++) != 0X00)
        {
            odr_seterror(o, OPROTO, 12);
            return 0;
        }
        return 1;
    case ODR_PRINT:
        return 1;
    default:
        odr_seterror(o, OOTHER, 13);
        return 0;
    }
}
/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

