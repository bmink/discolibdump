#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include "blog.h"
#include "bstr.h"
#include "hiredis_helper.h"
#include "bcurl.h"

bstr_t	*user_tok;

int load_user_tok(void);
int dump_albums(void);


int
main(int argc, char **argv)
{
	char		*execn;
	int		err;
	int		ret;

	err = 0;

	execn = basename(argv[0]);
	if(xstrempty(execn)) {
		fprintf(stderr, "Can't get executable name\n");
		err = -1;
		goto end_label;
	}

	ret = blog_init(execn);
	if(ret != 0) {
		fprintf(stderr, "Could not initialize logging: %s\n",
		    strerror(ret));
		err = -1;
		goto end_label;
	}

	ret = hiredis_init();
	if(ret != 0) {
		fprintf(stderr, "Could not connect to redis\n");
		err = -1;
		goto end_label;
	}

	ret = load_user_tok();
	if(ret != 0) {
		fprintf(stderr, "Couldn't load user token\nn");
		err = -1;
		goto end_label;
	}

	ret = bcurl_header_add("Accept: application/json");
	if(ret != 0) {
		fprintf(stderr, "Couldn't add Accept: header\n");
		err = -1;
		goto end_label;
	}

	ret = dump_albums();
	if(ret != 0) {
		fprintf(stderr, "Couldn't dump albums: %s\n", strerror(ret));
		err = -1;
		goto end_label;
	}

end_label:

	hiredis_uninit();

	blog_uninit();

	buninit(&user_tok);
	
	return err;
}


#define FILEN_USER_TOK	".access_token"

int
load_user_tok(void)
{
	int     err;
	int     ret;

	err = 0;

	user_tok = binit();
	if(user_tok == NULL) {
		fprintf(stderr, "Couldn't allocate user_tok\n");
		err = ENOMEM;
		goto end_label;
	}

	ret = bfromfile(user_tok, FILEN_USER_TOK);
	if(ret != 0) {
		fprintf(stderr, "Couldn't load access token: %s\n",
		    strerror(ret));
		err = ret;
		goto end_label;
	}

	ret = bstrchopnewline(user_tok);
	if(ret != 0) {
		fprintf(stderr, "Couldn't chop newline from user token: %s\n",
		    strerror(ret));
		err = ret;
		goto end_label;
	}

end_label:

	if(err != 0) {
		buninit(&user_tok);
	}

	return err;

}


int
dump_albums(void)
{
	return 0;
}
