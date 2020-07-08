#include "slsitem.h"
#include <errno.h>
#include "blog.h"

slsitem_t *
slsitem_init(const char *name, const char *url)
{
	slsitem_t	*slsi;
	int		err;

	if(xstrempty(name))
		return NULL;

	err = 0;
	slsi = NULL;

	slsi = malloc(sizeof(slsitem_t));
	if(slsi == NULL) {
		blogf("Couldn't allocate slsitem_t");
		err = ENOMEM;
		goto end_label;
	}

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
		buninit(&slsi->si_name);
		buninit(&slsi->si_url);
		free(slsi);
		slsi = NULL;
	}

	return slsi;
}

