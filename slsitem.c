#include "slsitem.h"
#include <errno.h>
#include "blog.h"

slsitem_t *
slsitem_init(const char *format, const char *name, const char *url)
{
	slsitem_t	*slsi;
	int		err;

	if(xstrempty(format) || xstrempty(name))
		return NULL;

	err = 0;
	slsi = NULL;

	slsi = malloc(sizeof(slsitem_t));
	if(slsi == NULL) {
		blogf("Couldn't allocate slsitem_t");
		err = ENOMEM;
		goto end_label;
	}

	slsi->si_format = binit();
	if(slsi->si_format == NULL) {
		blogf("Couldn't allocate si_format");
		err = ENOMEM;
		goto end_label;
	}

	bstrcat(slsi->si_format, format);

	slsi->si_name = binit();
	if(slsi->si_name == NULL) {
		blogf("Couldn't allocate si_name");
		err = ENOMEM;
		goto end_label;
	}

	bstrcat(slsi->si_name, name);

	slsi->si_url = binit();
	if(slsi->si_url == NULL) {
		blogf("Couldn't allocate si_url");
		err = ENOMEM;
		goto end_label;
	}

	if(!xstrempty(url))
		bstrcat(slsi->si_url, url);

end_label:

	if(err != 0 && slsi) {
		buninit(&slsi->si_format);
		buninit(&slsi->si_name);
		buninit(&slsi->si_url);
		free(slsi);
		slsi = NULL;
	}

	return slsi;
}


void
slsitem_uinit(slsitem_t **slsip)
{
	if(slsip == NULL|| *slsip == NULL)
		return;

	buninit(&((*slsip)->si_format));
	buninit(&((*slsip)->si_name));
	buninit(&((*slsip)->si_url));
	free(*slsip);
	*slsip = NULL;
}

