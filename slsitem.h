#ifndef SLSITEM_H
#define SLSITEM_H

#include "bstr.h"

typedef struct slsitem {
        bstr_t  *si_format;
        bstr_t  *si_name;
        bstr_t  *si_url;
} slsitem_t;


slsitem_t *slsitem_init(const char *, const char *, const char *);
void slsitem_uinit(slsitem_t **);

#endif

