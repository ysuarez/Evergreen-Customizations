/*
 * This file was generated automatically by ExtUtils::ParseXS version 2.2206 from the
 * contents of SpiderMonkey.xs. Do not edit this file, edit SpiderMonkey.xs instead.
 *
 *	ANY CHANGES MADE HERE WILL BE LOST! 
 *
 */

#line 1 "SpiderMonkey.xs"
/* --------------------------------------------------------------------- */
/* SpiderMonkey.xs -- Perl Interface to the SpiderMonkey JavaScript      */
/*                    implementation.                                    */
/*                                                                       */
/* Revision:     $Revision: 1.7 $                                        */
/* Last Checkin: $Date: 2010/05/29 06:49:31 $                            */
/* By:           $Author: thomas_busch $                                     */
/*                                                                       */
/* Author: Mike Schilli mschilli1@aol.com, 2001                          */
/* --------------------------------------------------------------------- */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "jsapi.h"
#include "SpiderMonkey.h"

#ifdef _MSC_VER
    /* As suggested in https://rt.cpan.org/Ticket/Display.html?id=6984 */
#define snprintf _snprintf 
#endif

/* JSRuntime needs this global class */
static
JSClass global_class = {
    "Global", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};

static int Debug = 0;

static int max_branch_operations = 0;

/* It's kinda silly that we have to replicate this for getters and setters,
 * but there doesn't seem to be a way to distinguish between getters
 * and setters if we use the same function. (Somewhere I read in a 
 * usenet posting there's something like IS_ASSIGN, but this doesn't
 * seem to be in SpiderMonkey 1.5).
 */

/* --------------------------------------------------------------------- */
JSBool getsetter_dispatcher(
    JSContext *cx, 
    JSObject  *obj,
    jsval      id,
    jsval     *vp,
    char      *what
/* --------------------------------------------------------------------- */
) {
    dSP; 

    /* Call back into perl */
    ENTER ; 
    SAVETMPS ;
    PUSHMARK(SP);
        /* A somewhat nasty trick: Since JS_DefineObject() down below
         * returns a *JS_Object, which is typemapped as T_PTRREF,
         * and which is a reference (!) pointing to the real C pointer,
         * we need to brutally obtain the obj's address by casting
         * it to an int and forming a scalar out of it.
         * On the other hand, when Spidermonkey.pm stores the 
         * object's setters/getters, it will dereference
         * what it gets from JS_DefineObject() (therefore
         * obtain the object's address in memory) to index its
         * hash table.
         * I hope all reasonable machines can hold an address in
         * an int.
         */
    XPUSHs(sv_2mortal(newSViv((int)obj)));
    XPUSHs(sv_2mortal(newSVpv(JS_GetStringBytes(JSVAL_TO_STRING(id)), 0)));
    XPUSHs(sv_2mortal(newSVpv(what, 0)));
    XPUSHs(sv_2mortal(newSVpv(JS_GetStringBytes(JSVAL_TO_STRING(*vp)), 0)));
    PUTBACK;
    call_pv("JavaScript::SpiderMonkey::getsetter_dispatcher", G_DISCARD);
    FREETMPS;
    LEAVE;

    return JS_TRUE;
}

/* --------------------------------------------------------------------- */
JSBool getter_dispatcher(
    JSContext *cx, 
    JSObject  *obj,
    jsval      id,
    jsval     *vp
/* --------------------------------------------------------------------- */
) {
    return getsetter_dispatcher(cx, obj, id, vp, "getter");
}

/* --------------------------------------------------------------------- */
JSBool setter_dispatcher(
    JSContext *cx, 
    JSObject  *obj,
    jsval      id,
    jsval     *vp
/* --------------------------------------------------------------------- */
) {
    return getsetter_dispatcher(cx, obj, id, vp, "setter");
}

/* --------------------------------------------------------------------- */
int debug_enabled(
/* --------------------------------------------------------------------- */
) {
    dSP; 

    int enabled = 0;
    int count   = 0;

    /* Call back into perl */
    ENTER ; 
    SAVETMPS ;
    PUTBACK;
    count = call_pv("JavaScript::SpiderMonkey::debug_enabled", G_SCALAR);
    if(count == 1) {
        if(POPi == 1) {
            enabled = 1;
        }
    }
    FREETMPS;
    LEAVE;

    return enabled;
}

/* --------------------------------------------------------------------- */
static JSBool
FunctionDispatcher(JSContext *cx, JSObject *obj, uintN argc, 
    jsval *argv, jsval *rval) {
/* --------------------------------------------------------------------- */
    dSP; 
    SV          *sv;
    char        *n_jstr;
    int         n_jnum;
    double      n_jdbl;
    unsigned    i;
    int         count;
    JSFunction  *fun;
    fun = JS_ValueToFunction(cx, argv[-2]);

    /* printf("Function %s received %d arguments\n", 
           (char *) JS_GetFunctionName(fun),
           (int) argc); */

    /* Call back into perl */
    ENTER ; 
    SAVETMPS ;
    PUSHMARK(SP);
    XPUSHs(sv_2mortal(newSViv((int)obj)));
    XPUSHs(sv_2mortal(newSVpv(
        JS_GetFunctionName(fun), 0)));
    for(i=0; i<argc; i++) {
        XPUSHs(sv_2mortal(newSVpv(
            JS_GetStringBytes(JS_ValueToString(cx, argv[i])), 0)));
    }
    PUTBACK;
    count = call_pv("JavaScript::SpiderMonkey::function_dispatcher", G_SCALAR);
    SPAGAIN;

    if(Debug)
        fprintf(stderr, "DEBUG: Count is %d\n", count);

    if( count > 0) {
        sv = POPs;        
        if(SvROK(sv)) {
            /* Im getting a perl reference here, the user
             * seems to want to send a perl object to jscript
             * ok, we will do it, although it seems like a painful
             * thing to me.
             */

            if(Debug)
                fprintf(stderr, "DEBUG: %lx is a ref!\n", (long) sv);
            *rval = OBJECT_TO_JSVAL(SvIV(SvRV(sv)));
        }
        else if(SvIOK(sv)) {
            /* It appears that we have been sent an int return
             * value.  Thats fine we can give javascript an int
             */
            n_jnum=SvIV(sv);
            if(Debug)
                fprintf(stderr, "DEBUG: %lx is an int (%d)\n", (long) sv,n_jnum);
            *rval = INT_TO_JSVAL(n_jnum);
        } else if(SvNOK(sv)) {
            /* It appears that we have been sent an double return
             * value.  Thats fine we can give javascript an double
             */
            n_jdbl=SvNV(sv);

            if(Debug) 
                fprintf(stderr, "DEBUG: %lx is a double(%f)\n", (long) sv,n_jdbl);
            *rval = DOUBLE_TO_JSVAL(JS_NewDouble(cx, n_jdbl));
        } else if(SvPOK(sv)) {
            n_jstr = SvPV(sv, PL_na);
            //warn("DEBUG: %s (%d)\n", n_jstr);
            *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, n_jstr));
        }
    }

    PUTBACK;
    FREETMPS;
    LEAVE;

    return JS_TRUE;
}

/* --------------------------------------------------------------------- */
static void
ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report) {
/* --------------------------------------------------------------------- */
     char msg[400];
     if (report->linebuf) {
         int i = 0;
         int printed = 
             snprintf (msg, sizeof(msg), 
                       "Error: %s at line %d: ", message, report->lineno
                       );
         /* Don't print the \n at the end of report->linebuf. */
         while (printed < sizeof (msg) - 1) {
             if (report->linebuf[i] == '\n')
                 break;
             msg[printed] = report->linebuf[i];
             printed++;
             i++;
         }
         msg[printed] = '\0';
     } else {
         /*
           Fix for following bug (report->linebuf is null at runtime):
           https://rt.cpan.org/Public/Bug/Display.html?id=57617
           BKB 2010-05-24 10:12:45
          */
         snprintf(msg, sizeof(msg), 
                  "Error: %s at line %d", message, report->lineno);
     }
     sv_setpv(get_sv("@", TRUE), msg);
}

/* --------------------------------------------------------------------- */
static JSBool
BranchHandler(JSContext *cx, JSScript *script) {
/* --------------------------------------------------------------------- */
  PJS_Context* pcx = (PJS_Context*) JS_GetContextPrivate(cx);

  pcx->branch_count++;
  if (pcx->branch_count > pcx->branch_max) {
    return JS_FALSE;
  } else {
    return JS_TRUE;
  }
}



#line 268 "SpiderMonkey.c"
#ifndef PERL_UNUSED_VAR
#  define PERL_UNUSED_VAR(var) if (0) var = var
#endif

#ifndef PERL_ARGS_ASSERT_CROAK_XS_USAGE
#define PERL_ARGS_ASSERT_CROAK_XS_USAGE assert(cv); assert(params)

/* prototype to pass -Wmissing-prototypes */
STATIC void
S_croak_xs_usage(pTHX_ const CV *const cv, const char *const params);

STATIC void
S_croak_xs_usage(pTHX_ const CV *const cv, const char *const params)
{
    const GV *const gv = CvGV(cv);

    PERL_ARGS_ASSERT_CROAK_XS_USAGE;

    if (gv) {
        const char *const gvname = GvNAME(gv);
        const HV *const stash = GvSTASH(gv);
        const char *const hvname = stash ? HvNAME(stash) : NULL;

        if (hvname)
            Perl_croak(aTHX_ "Usage: %s::%s(%s)", hvname, gvname, params);
        else
            Perl_croak(aTHX_ "Usage: %s(%s)", gvname, params);
    } else {
        /* Pants. I don't think that it should be possible to get here. */
        Perl_croak(aTHX_ "Usage: CODE(0x%"UVxf")(%s)", PTR2UV(cv), params);
    }
}
#undef  PERL_ARGS_ASSERT_CROAK_XS_USAGE

#ifdef PERL_IMPLICIT_CONTEXT
#define croak_xs_usage(a,b)	S_croak_xs_usage(aTHX_ a,b)
#else
#define croak_xs_usage		S_croak_xs_usage
#endif

#endif

/* NOTE: the prototype of newXSproto() is different in versions of perls,
 * so we define a portable version of newXSproto()
 */
#ifdef newXS_flags
#define newXSproto_portable(name, c_impl, file, proto) newXS_flags(name, c_impl, file, proto, 0)
#else
#define newXSproto_portable(name, c_impl, file, proto) (PL_Sv=(SV*)newXS(name, c_impl, file), sv_setpv(PL_Sv, proto), (CV*)PL_Sv)
#endif /* !defined(newXS_flags) */

#line 320 "SpiderMonkey.c"

XS(XS_JavaScript__SpiderMonkey_JS_GetImplementationVersion); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_GetImplementationVersion)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 0)
       croak_xs_usage(cv,  "");
    {
	char *	RETVAL;
	dXSTARG;
#line 266 "SpiderMonkey.xs"
    {
        RETVAL = (char *) JS_GetImplementationVersion();
    }
#line 339 "SpiderMonkey.c"
	sv_setpv(TARG, RETVAL); XSprePUSH; PUSHTARG;
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_NewRuntime); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_NewRuntime)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 1)
       croak_xs_usage(cv,  "maxbytes");
    {
	int	maxbytes = (int)SvIV(ST(0));
#line 278 "SpiderMonkey.xs"
    JSRuntime *rt;
#line 360 "SpiderMonkey.c"
	JSRuntime *	RETVAL;
#line 280 "SpiderMonkey.xs"
    {
        rt = JS_NewRuntime(maxbytes);
        if(!rt) {
            XSRETURN_UNDEF;
        }
        RETVAL = rt;
    }
#line 370 "SpiderMonkey.c"
	ST(0) = sv_newmortal();
	sv_setref_pv(ST(0), Nullch, (void*)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_DestroyRuntime); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_DestroyRuntime)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 1)
       croak_xs_usage(cv,  "rt");
    {
	JSRuntime *	rt;
	int	RETVAL;
	dXSTARG;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    rt = INT2PTR(JSRuntime *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DestroyRuntime",
			"rt");
#line 296 "SpiderMonkey.xs"
    {
        JS_DestroyRuntime(rt);
        RETVAL = 0;
    }
#line 406 "SpiderMonkey.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_Init); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_Init)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 1)
       croak_xs_usage(cv,  "maxbytes");
    {
	int	maxbytes = (int)SvIV(ST(0));
#line 309 "SpiderMonkey.xs"
    JSRuntime *rt;
#line 427 "SpiderMonkey.c"
	JSRuntime *	RETVAL;
#line 311 "SpiderMonkey.xs"
    {
        rt = JS_Init(maxbytes);
        if(!rt) {
            XSRETURN_UNDEF;
        }
            /* Replace this by Debug = debug_enabled(); once 
             * Log::Log4perl 0.47 is out */
        Debug = 0;
        RETVAL = rt;
    }
#line 440 "SpiderMonkey.c"
	ST(0) = sv_newmortal();
	sv_setref_pv(ST(0), Nullch, (void*)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_NewContext); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_NewContext)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 2)
       croak_xs_usage(cv,  "rt, stack_chunk_size");
    {
	JSRuntime *	rt;
	int	stack_chunk_size = (int)SvIV(ST(1));
#line 331 "SpiderMonkey.xs"
    JSContext *cx;
#line 463 "SpiderMonkey.c"
	JSContext *	RETVAL;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    rt = INT2PTR(JSRuntime *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_NewContext",
			"rt");
#line 333 "SpiderMonkey.xs"
    {
        PJS_Context* pcx;
        cx = JS_NewContext(rt, stack_chunk_size);
        if(!cx) {
            XSRETURN_UNDEF;
        }
#ifdef E4X
        JS_SetOptions(cx,JSOPTION_XML);
#endif

        Newz(1, pcx, 1, PJS_Context);
        JS_SetContextPrivate(cx, (void *)pcx);

        RETVAL = cx;
    }
#line 490 "SpiderMonkey.c"
	ST(0) = sv_newmortal();
	sv_setref_pv(ST(0), Nullch, (void*)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_DestroyContext); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_DestroyContext)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 1)
       croak_xs_usage(cv,  "cx");
    {
	JSContext *	cx;
	int	RETVAL;
	dXSTARG;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DestroyContext",
			"cx");
#line 357 "SpiderMonkey.xs"
    {
        JS_DestroyContext(cx);
        Safefree(JS_GetContextPrivate(cx));
        RETVAL = 0;
    }
#line 527 "SpiderMonkey.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_NewObject); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_NewObject)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 4)
       croak_xs_usage(cv,  "cx, class, proto, parent");
    {
	JSContext *	cx;
	JSClass *	class;
	JSObject *	proto;
	JSObject *	parent;
#line 374 "SpiderMonkey.xs"
    JSObject *obj;
#line 551 "SpiderMonkey.c"
	JSObject *	RETVAL;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_NewObject",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    class = INT2PTR(JSClass *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_NewObject",
			"class");

	if (SvROK(ST(2))) {
	    IV tmp = SvIV((SV*)SvRV(ST(2)));
	    proto = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_NewObject",
			"proto");

	if (SvROK(ST(3))) {
	    IV tmp = SvIV((SV*)SvRV(ST(3)));
	    parent = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_NewObject",
			"parent");
#line 376 "SpiderMonkey.xs"
    {
        obj = JS_NewObject(cx, class, NULL, NULL);
        if(!obj) {
            XSRETURN_UNDEF;
        }
        RETVAL = obj;
    }
#line 597 "SpiderMonkey.c"
	ST(0) = sv_newmortal();
	sv_setref_pv(ST(0), Nullch, (void*)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_InitClass); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_InitClass)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 10)
       croak_xs_usage(cv,  "cx, iobj, parent_proto, clasp, constructor, nargs, ps, fs, static_ps, static_fs");
    {
	JSContext *	cx;
	JSObject *	iobj;
	JSObject *	parent_proto;
	JSClass *	clasp;
	JSNative	constructor;
	int	nargs = (int)SvIV(ST(5));
	JSPropertySpec *	ps;
	JSFunctionSpec *	fs;
	JSPropertySpec *	static_ps;
	JSFunctionSpec *	static_fs;
#line 401 "SpiderMonkey.xs"
    JSObject *obj;
    uintN     na;
#line 629 "SpiderMonkey.c"
	JSObject *	RETVAL;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitClass",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    iobj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitClass",
			"iobj");

	if (SvROK(ST(2))) {
	    IV tmp = SvIV((SV*)SvRV(ST(2)));
	    parent_proto = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitClass",
			"parent_proto");

	if (SvROK(ST(3))) {
	    IV tmp = SvIV((SV*)SvRV(ST(3)));
	    clasp = INT2PTR(JSClass *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitClass",
			"clasp");

	if (SvROK(ST(4))) {
	    IV tmp = SvIV((SV*)SvRV(ST(4)));
	    constructor = INT2PTR(JSNative,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitClass",
			"constructor");

	if (SvROK(ST(6))) {
	    IV tmp = SvIV((SV*)SvRV(ST(6)));
	    ps = INT2PTR(JSPropertySpec *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitClass",
			"ps");

	if (SvROK(ST(7))) {
	    IV tmp = SvIV((SV*)SvRV(ST(7)));
	    fs = INT2PTR(JSFunctionSpec *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitClass",
			"fs");

	if (SvROK(ST(8))) {
	    IV tmp = SvIV((SV*)SvRV(ST(8)));
	    static_ps = INT2PTR(JSPropertySpec *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitClass",
			"static_ps");

	if (SvROK(ST(9))) {
	    IV tmp = SvIV((SV*)SvRV(ST(9)));
	    static_fs = INT2PTR(JSFunctionSpec *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitClass",
			"static_fs");
#line 404 "SpiderMonkey.xs"
    na = (uintN) nargs;
#line 714 "SpiderMonkey.c"
#line 406 "SpiderMonkey.xs"
    {
        obj = JS_InitClass(cx, iobj, parent_proto, clasp,
                           constructor, nargs, ps, fs, static_ps,
                           static_fs);
        if(!obj) {
            XSRETURN_UNDEF;
        }
        RETVAL = obj;
    }
#line 725 "SpiderMonkey.c"
	ST(0) = sv_newmortal();
	sv_setref_pv(ST(0), Nullch, (void*)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_GlobalClass); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_GlobalClass)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 0)
       croak_xs_usage(cv,  "");
    {
#line 423 "SpiderMonkey.xs"
    JSClass *gc;
#line 746 "SpiderMonkey.c"
	JSClass *	RETVAL;
#line 425 "SpiderMonkey.xs"
    {
        gc = &global_class;
        RETVAL = gc;
    }
#line 753 "SpiderMonkey.c"
	ST(0) = sv_newmortal();
	sv_setref_pv(ST(0), Nullch, (void*)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_EvaluateScript); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_EvaluateScript)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 6)
       croak_xs_usage(cv,  "cx, gobj, script, length, filename, lineno");
    {
	JSContext *	cx;
	JSObject *	gobj;
	char *	script = (char *)SvPV_nolen(ST(2));
	int	length = (int)SvIV(ST(3));
	char *	filename = (char *)SvPV_nolen(ST(4));
	int	lineno = (int)SvIV(ST(5));
#line 443 "SpiderMonkey.xs"
    uintN len;
    uintN ln;
    int    rc;
    jsval  jsval;
#line 783 "SpiderMonkey.c"
	int	RETVAL;
	dXSTARG;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_EvaluateScript",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    gobj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_EvaluateScript",
			"gobj");
#line 448 "SpiderMonkey.xs"
    len = (uintN) length;
    ln  = (uintN) lineno;
#line 807 "SpiderMonkey.c"
#line 451 "SpiderMonkey.xs"
    {
        rc = JS_EvaluateScript(cx, gobj, script, len, filename,
                               ln, &jsval);
        if(!rc) {
            XSRETURN_UNDEF;
        }
        RETVAL = rc;
    }
#line 817 "SpiderMonkey.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_InitStandardClasses); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_InitStandardClasses)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 2)
       croak_xs_usage(cv,  "cx, gobj");
    {
	JSContext *	cx;
	JSObject *	gobj;
#line 469 "SpiderMonkey.xs"
    JSBool rc;
#line 839 "SpiderMonkey.c"
	int	RETVAL;
	dXSTARG;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitStandardClasses",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    gobj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_InitStandardClasses",
			"gobj");
#line 471 "SpiderMonkey.xs"
    {
        rc = JS_InitStandardClasses(cx, gobj);
        if(!rc) {
            XSRETURN_UNDEF;
        }
        RETVAL = (int) rc;
    }
#line 868 "SpiderMonkey.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_DefineFunction); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_DefineFunction)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 5)
       croak_xs_usage(cv,  "cx, obj, name, nargs, flags");
    {
	JSContext *	cx;
	JSObject *	obj;
	char *	name = (char *)SvPV_nolen(ST(2));
	int	nargs = (int)SvIV(ST(3));
	int	flags = (int)SvIV(ST(4));
#line 491 "SpiderMonkey.xs"
    JSFunction *rc;
#line 893 "SpiderMonkey.c"
	int	RETVAL;
	dXSTARG;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DefineFunction",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    obj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DefineFunction",
			"obj");
#line 493 "SpiderMonkey.xs"
    {
        rc = JS_DefineFunction(cx, obj,
             (const char *) name, FunctionDispatcher,
             (uintN) nargs, (uintN) flags);
        if(!rc) {
            XSRETURN_UNDEF;
        }
        RETVAL = (int) rc;
    }
#line 924 "SpiderMonkey.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_SetErrorReporter); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_SetErrorReporter)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 1)
       croak_xs_usage(cv,  "cx");
    {
	JSContext *	cx;
	int	RETVAL;
	dXSTARG;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_SetErrorReporter",
			"cx");
#line 511 "SpiderMonkey.xs"
    {
        JS_SetErrorReporter(cx, ErrorReporter);
        RETVAL = 0;
    }
#line 959 "SpiderMonkey.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_DefineObject); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_DefineObject)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 5)
       croak_xs_usage(cv,  "cx, obj, name, class, proto");
    {
	JSContext *	cx;
	JSObject *	obj;
	char *	name = (char *)SvPV_nolen(ST(2));
	JSClass *	class;
	JSObject *	proto;
#line 528 "SpiderMonkey.xs"
    SV       *sv = sv_newmortal();
#line 984 "SpiderMonkey.c"
	JSObject *	RETVAL;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DefineObject",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    obj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DefineObject",
			"obj");

	if (SvROK(ST(3))) {
	    IV tmp = SvIV((SV*)SvRV(ST(3)));
	    class = INT2PTR(JSClass *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DefineObject",
			"class");

	if (SvROK(ST(4))) {
	    IV tmp = SvIV((SV*)SvRV(ST(4)));
	    proto = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DefineObject",
			"proto");
#line 530 "SpiderMonkey.xs"
    {
        RETVAL = JS_DefineObject(cx, obj, name, class, proto, 0);
    }
#line 1026 "SpiderMonkey.c"
	ST(0) = sv_newmortal();
	sv_setref_pv(ST(0), Nullch, (void*)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_DefineProperty); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_DefineProperty)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 4)
       croak_xs_usage(cv,  "cx, obj, name, value");
    {
	JSContext *	cx;
	JSObject *	obj;
	char *	name = (char *)SvPV_nolen(ST(2));
	char *	value = (char *)SvPV_nolen(ST(3));
#line 548 "SpiderMonkey.xs"
    JSBool rc;
    JSString *str;
#line 1052 "SpiderMonkey.c"
	int	RETVAL;
	dXSTARG;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DefineProperty",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    obj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_DefineProperty",
			"obj");
#line 551 "SpiderMonkey.xs"
    {
        str = JS_NewStringCopyZ(cx, value); 

        /* This implementation is somewhat sub-optimal, since it
         * calls back into perl even if no getters/setters have
         * been defined. The necessity for a callback is determined
         * at the perl level, where there's a data structure mapping
         * out each object's properties and their getter/setter settings.
         */
        rc = JS_DefineProperty(cx, obj, name, STRING_TO_JSVAL(str), 
                               getter_dispatcher, setter_dispatcher, 0);
        RETVAL = (int) rc;
    }
#line 1087 "SpiderMonkey.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_GetProperty); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_GetProperty)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 3)
       croak_xs_usage(cv,  "cx, obj, name");
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
	JSContext *	cx;
	JSObject *	obj;
	char *	name = (char *)SvPV_nolen(ST(2));
#line 575 "SpiderMonkey.xs"
    JSBool rc;
    jsval  vp;
    JSString *str;
    SV       *sv = sv_newmortal();
#line 1115 "SpiderMonkey.c"

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_GetProperty",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    obj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_GetProperty",
			"obj");
#line 580 "SpiderMonkey.xs"
    {
        rc = JS_TRUE;
        rc = JS_GetProperty(cx, obj, name, &vp);
        if(rc) {
            str = JS_ValueToString(cx, vp);
            if(strcmp(JS_GetStringBytes(str), "undefined") == 0) {
                sv = &PL_sv_undef;
            } else {
                sv_setpv(sv, JS_GetStringBytes(str));
            }
        } else {
            sv = &PL_sv_undef;
        }
        XPUSHs(sv);
    }
#line 1150 "SpiderMonkey.c"
	PUTBACK;
	return;
    }
}


XS(XS_JavaScript__SpiderMonkey_JS_NewArrayObject); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_NewArrayObject)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 1)
       croak_xs_usage(cv,  "cx");
    {
	JSContext *	cx;
#line 602 "SpiderMonkey.xs"
    JSObject *rc;
#line 1171 "SpiderMonkey.c"
	JSObject *	RETVAL;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_NewArrayObject",
			"cx");
#line 604 "SpiderMonkey.xs"
    {
        rc = JS_NewArrayObject(cx, 0, NULL);
        RETVAL = rc;
    }
#line 1187 "SpiderMonkey.c"
	ST(0) = sv_newmortal();
	sv_setref_pv(ST(0), Nullch, (void*)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_SetElement); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_SetElement)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 4)
       croak_xs_usage(cv,  "cx, obj, idx, valptr");
    {
	JSContext *	cx;
	JSObject *	obj;
	int	idx = (int)SvIV(ST(2));
	char *	valptr = (char *)SvPV_nolen(ST(3));
#line 620 "SpiderMonkey.xs"
    JSBool rc;
    JSString  *str;
    jsval val;
#line 1214 "SpiderMonkey.c"
	int	RETVAL;
	dXSTARG;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_SetElement",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    obj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_SetElement",
			"obj");
#line 624 "SpiderMonkey.xs"
    {
        str = JS_NewStringCopyZ(cx, valptr);
        val = STRING_TO_JSVAL(str); 
        rc = JS_SetElement(cx, obj, idx, &val);
        if(rc) {
            RETVAL = 1;
        } else {
            RETVAL = 0;
        }
    }
#line 1246 "SpiderMonkey.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_SetElementAsObject); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_SetElementAsObject)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 4)
       croak_xs_usage(cv,  "cx, obj, idx, elobj");
    {
	JSContext *	cx;
	JSObject *	obj;
	int	idx = (int)SvIV(ST(2));
	JSObject *	elobj;
#line 646 "SpiderMonkey.xs"
    JSBool rc;
    jsval val;
#line 1271 "SpiderMonkey.c"
	int	RETVAL;
	dXSTARG;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_SetElementAsObject",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    obj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_SetElementAsObject",
			"obj");

	if (SvROK(ST(3))) {
	    IV tmp = SvIV((SV*)SvRV(ST(3)));
	    elobj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_SetElementAsObject",
			"elobj");
#line 649 "SpiderMonkey.xs"
    {
        val = OBJECT_TO_JSVAL(elobj); 
        rc = JS_SetElement(cx, obj, idx, &val);
        if(rc) {
            RETVAL = 1;
        } else {
            RETVAL = 0;
        }
    }
#line 1311 "SpiderMonkey.c"
	XSprePUSH; PUSHi((IV)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_GetElement); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_GetElement)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 3)
       croak_xs_usage(cv,  "cx, obj, idx");
    PERL_UNUSED_VAR(ax); /* -Wall */
    SP -= items;
    {
	JSContext *	cx;
	JSObject *	obj;
	int	idx = (int)SvIV(ST(2));
#line 669 "SpiderMonkey.xs"
    JSBool rc;
    jsval  vp;
    JSString *str;
    SV       *sv = sv_newmortal();
#line 1339 "SpiderMonkey.c"

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_GetElement",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    obj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_GetElement",
			"obj");
#line 674 "SpiderMonkey.xs"
    {
        rc = JS_GetElement(cx, obj, idx, &vp);
        if(rc) {
            str = JS_ValueToString(cx, vp);
            if(strcmp(JS_GetStringBytes(str), "undefined") == 0) {
                sv = &PL_sv_undef;
            } else {
                sv_setpv(sv, JS_GetStringBytes(str));
            }
        } else {
            sv = &PL_sv_undef;
        }
        XPUSHs(sv);
    }
#line 1373 "SpiderMonkey.c"
	PUTBACK;
	return;
    }
}


XS(XS_JavaScript__SpiderMonkey_JS_GetClass); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_GetClass)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 2)
       croak_xs_usage(cv,  "cx, obj");
    {
	JSContext *	cx;
	JSObject *	obj;
#line 696 "SpiderMonkey.xs"
    JSClass *rc;
#line 1395 "SpiderMonkey.c"
	JSClass *	RETVAL;

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_GetClass",
			"cx");

	if (SvROK(ST(1))) {
	    IV tmp = SvIV((SV*)SvRV(ST(1)));
	    obj = INT2PTR(JSObject *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_GetClass",
			"obj");
#line 698 "SpiderMonkey.xs"
    {
#ifdef JS_THREADSAFE
        rc = JS_GetClass(cx, obj);
#else
        rc = JS_GetClass(obj);
#endif
        RETVAL = rc;
    }
#line 1424 "SpiderMonkey.c"
	ST(0) = sv_newmortal();
	sv_setref_pv(ST(0), Nullch, (void*)RETVAL);
    }
    XSRETURN(1);
}


XS(XS_JavaScript__SpiderMonkey_JS_SetMaxBranchOperations); /* prototype to pass -Wmissing-prototypes */
XS(XS_JavaScript__SpiderMonkey_JS_SetMaxBranchOperations)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    if (items != 2)
       croak_xs_usage(cv,  "cx, max_branch_operations");
    {
	JSContext *	cx;
	int	max_branch_operations = (int)SvIV(ST(1));

	if (SvROK(ST(0))) {
	    IV tmp = SvIV((SV*)SvRV(ST(0)));
	    cx = INT2PTR(JSContext *,tmp);
	}
	else
	    Perl_croak(aTHX_ "%s: %s is not a reference",
			"JavaScript::SpiderMonkey::JS_SetMaxBranchOperations",
			"cx");
#line 717 "SpiderMonkey.xs"
    {
        PJS_Context* pcx = (PJS_Context *) JS_GetContextPrivate(cx);
        pcx->branch_count = 0;
        pcx->branch_max = max_branch_operations;
        JS_SetBranchCallback(cx, BranchHandler);
    }
#line 1461 "SpiderMonkey.c"
    }
    XSRETURN_EMPTY;
}

#ifdef __cplusplus
extern "C"
#endif
XS(boot_JavaScript__SpiderMonkey); /* prototype to pass -Wmissing-prototypes */
XS(boot_JavaScript__SpiderMonkey)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
#if (PERL_REVISION == 5 && PERL_VERSION < 9)
    char* file = __FILE__;
#else
    const char* file = __FILE__;
#endif

    PERL_UNUSED_VAR(cv); /* -W */
    PERL_UNUSED_VAR(items); /* -W */
    XS_VERSION_BOOTCHECK ;

        newXS("JavaScript::SpiderMonkey::JS_GetImplementationVersion", XS_JavaScript__SpiderMonkey_JS_GetImplementationVersion, file);
        newXS("JavaScript::SpiderMonkey::JS_NewRuntime", XS_JavaScript__SpiderMonkey_JS_NewRuntime, file);
        newXS("JavaScript::SpiderMonkey::JS_DestroyRuntime", XS_JavaScript__SpiderMonkey_JS_DestroyRuntime, file);
        newXS("JavaScript::SpiderMonkey::JS_Init", XS_JavaScript__SpiderMonkey_JS_Init, file);
        newXS("JavaScript::SpiderMonkey::JS_NewContext", XS_JavaScript__SpiderMonkey_JS_NewContext, file);
        newXS("JavaScript::SpiderMonkey::JS_DestroyContext", XS_JavaScript__SpiderMonkey_JS_DestroyContext, file);
        newXS("JavaScript::SpiderMonkey::JS_NewObject", XS_JavaScript__SpiderMonkey_JS_NewObject, file);
        newXS("JavaScript::SpiderMonkey::JS_InitClass", XS_JavaScript__SpiderMonkey_JS_InitClass, file);
        newXS("JavaScript::SpiderMonkey::JS_GlobalClass", XS_JavaScript__SpiderMonkey_JS_GlobalClass, file);
        newXS("JavaScript::SpiderMonkey::JS_EvaluateScript", XS_JavaScript__SpiderMonkey_JS_EvaluateScript, file);
        newXS("JavaScript::SpiderMonkey::JS_InitStandardClasses", XS_JavaScript__SpiderMonkey_JS_InitStandardClasses, file);
        newXS("JavaScript::SpiderMonkey::JS_DefineFunction", XS_JavaScript__SpiderMonkey_JS_DefineFunction, file);
        newXS("JavaScript::SpiderMonkey::JS_SetErrorReporter", XS_JavaScript__SpiderMonkey_JS_SetErrorReporter, file);
        newXS("JavaScript::SpiderMonkey::JS_DefineObject", XS_JavaScript__SpiderMonkey_JS_DefineObject, file);
        newXS("JavaScript::SpiderMonkey::JS_DefineProperty", XS_JavaScript__SpiderMonkey_JS_DefineProperty, file);
        newXS("JavaScript::SpiderMonkey::JS_GetProperty", XS_JavaScript__SpiderMonkey_JS_GetProperty, file);
        newXS("JavaScript::SpiderMonkey::JS_NewArrayObject", XS_JavaScript__SpiderMonkey_JS_NewArrayObject, file);
        newXS("JavaScript::SpiderMonkey::JS_SetElement", XS_JavaScript__SpiderMonkey_JS_SetElement, file);
        newXS("JavaScript::SpiderMonkey::JS_SetElementAsObject", XS_JavaScript__SpiderMonkey_JS_SetElementAsObject, file);
        newXS("JavaScript::SpiderMonkey::JS_GetElement", XS_JavaScript__SpiderMonkey_JS_GetElement, file);
        newXS("JavaScript::SpiderMonkey::JS_GetClass", XS_JavaScript__SpiderMonkey_JS_GetClass, file);
        newXS("JavaScript::SpiderMonkey::JS_SetMaxBranchOperations", XS_JavaScript__SpiderMonkey_JS_SetMaxBranchOperations, file);
#if (PERL_REVISION == 5 && PERL_VERSION >= 9)
  if (PL_unitcheckav)
       call_list(PL_scopestack_ix, PL_unitcheckav);
#endif
    XSRETURN_YES;
}

