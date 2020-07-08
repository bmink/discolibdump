#include <stdio.h>
#include <libgen.h>
#include <errno.h>
#include "blog.h"
#include "bstr.h"
#include "hiredis_helper.h"
#include "bcurl.h"
#include "cJSON.h"
#include "cJSON_helper.h"
#include "slsitem.h"

bstr_t	*user_tok;

int load_user_tok(void);
int dump_albums(void);
int save_album(const char *, int);

#define USER_AGENT	"discolibdump/0.1"

#define FORMAT		"vinyl"


int
main(int argc, char **argv)
{
	char		*execn;
	int		err;
	int		ret;
	bstr_t		*useragent;

	err = 0;
	useragent = NULL;

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

	ret = bcurl_init();
	if(ret != 0) {
		fprintf(stderr, "Couldn't initialize curl\n");
		err = -1;
		goto end_label;
	}

	ret = bcurl_header_add("Accept: application/json");
	if(ret != 0) {
		fprintf(stderr, "Couldn't add Accept: header\n");
		err = -1;
		goto end_label;
	}

	useragent = binit();
	if(useragent == NULL) {
		fprintf(stderr, "Can't allocate useragent\n");
		err = -1;
		goto end_label;
	}
	bprintf(useragent, "User-Agent: %s", USER_AGENT);

	ret = bcurl_header_add(bget(useragent));
	if(ret != 0) {
		fprintf(stderr, "Couldn't add User-Agent: header\n");
		err = -1;
		goto end_label;
	}

	ret = load_user_tok();
	if(ret != 0) {
		fprintf(stderr, "Couldn't load user token\nn");
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
	bcurl_uninit();

	blog_uninit();

	buninit(&useragent);
	buninit(&user_tok);
	
	return err;
}


#define FILEN_USER_TOK	".user_token"

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
		fprintf(stderr, "Couldn't load user token: %s\n",
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


#define URL_ALBUMS_FMT	"https://api.discogs.com/users/bmink/collection/folders/0/releases?sort=added&sort_order=desc&token=%s"

#define RECENT_CNT	20

#define REDIS_KEY_VINYL_ALL	"sls:vinyl:all"
#define REDIS_KEY_VINYL_ALL_TMP	"sls:vinyl:all.tmp"
#define REDIS_KEY_VINYL_REC	"sls:vinyl:recent"
#define REDIS_KEY_VINYL_REC_TMP	"sls:vinyl:recent.tmp"


int process_releases(cJSON *, int *);


int
dump_albums(void)
{
	int		err;
	bstr_t		*url;
	bstr_t		*resp;
	cJSON		*json;
	cJSON		*pagination;
	cJSON		*releases;
	cJSON		*urls;
	int		ret;
	int		idx;

	err = 0;
	url = NULL;
	resp = NULL;

	(void) hiredis_del(REDIS_KEY_VINYL_ALL_TMP);
	(void) hiredis_del(REDIS_KEY_VINYL_REC_TMP);

	url = binit();
	if(!url) {
		fprintf(stderr, "Couldn't allocate url\n");
		err = ENOMEM;
		goto end_label;
	}

	bprintf(url, URL_ALBUMS_FMT, bget(user_tok));

#if 0
	printf("%s\n", bget(url));
#endif

	idx = 0;
	while(1) {

		ret = bcurl_get(bget(url), &resp);
		if(ret != 0) {
			fprintf(stderr, "Couldn't get albums list\n");
			err = ret;
			goto end_label;
		}

#if 0
		printf("%s\n", bget(resp));
#endif

		json = cJSON_Parse(bget(resp));
		if(json == NULL) {
			fprintf(stderr, "Couldn't parse JSON\n");
			err = ENOEXEC;
			goto end_label;
		}

		releases = cJSON_GetObjectItemCaseSensitive(json, "releases");
		if(!releases) {
			blogf("Response didn't contain releases");
			err = ENOENT;
			goto end_label;
		}

		ret = process_releases(releases, &idx);
		if(ret != 0) {
			blogf("Couldn't process releases");
			err = ret;
			goto end_label;
		}

		pagination = cJSON_GetObjectItemCaseSensitive(json,
		    "pagination");
		if(!pagination) {
			break;
		}
		urls = cJSON_GetObjectItemCaseSensitive(pagination, "urls");
		if(!urls) {
			break;
		}

		bclear(url);

		ret = cjson_get_childstr(urls, "next", url);

		cJSON_Delete(json);
		json = NULL;

		if(ret != 0)
			break;
#if 0
		printf("next url: %s\n", bget(url));
#endif


	}
	


end_label:

	buninit(&url);
	buninit(&resp);

	(void) hiredis_del(REDIS_KEY_VINYL_ALL_TMP);
	(void) hiredis_del(REDIS_KEY_VINYL_REC_TMP);

	return err;
}


int
process_releases(cJSON *releases, int *idx)
{
	int	err;
	cJSON	*release;
	cJSON	*basic_information;
	cJSON	*artists;
	cJSON	*artist;
	int	ret;
	bstr_t	*albnam;
	bstr_t	*artistnam;
	bstr_t	*name;

	if(releases == NULL)
		return EINVAL;

	err = 0;
	albnam = NULL;
	artistnam = NULL;
	name = NULL;

	albnam = binit();
	if(!albnam) {
		blogf("Couldn't allocate albnam");
		err = ENOMEM;
		goto end_label;
	}

	artistnam = binit();
	if(!artistnam) {
		blogf("Couldn't allocate artistnam");
		err = ENOMEM;
		goto end_label;
	}

	name = binit();
	if(!name) {
		blogf("Couldn't allocate name");
		err = ENOMEM;
		goto end_label;
	}


	for(release = releases->child; release; release = release->next) {

		basic_information = cJSON_GetObjectItemCaseSensitive(release,
		    "basic_information");
		if(!basic_information) {
			blogf("Release didn't contain basic_information");
			err = ENOENT;
			goto end_label;
		}

		artists = cJSON_GetObjectItemCaseSensitive(basic_information,
		    "artists");
		if(!artists) {
			blogf("Release didn't contain artists");
			err = ENOENT;
			goto end_label;
		}

		for(artist = artists->child; artist; artist = artist->next) {
			ret = cjson_get_childstr(artist, "name", artistnam);
			if(ret != 0) {
				blogf("Artist didn't contain name");
				err = ENOENT;
				goto end_label;
			}

			if(!bstrempty(name))
				bstrcat(name, ", ");

			bstrcat(name, bget(artistnam));

			bclear(artistnam);
		}

		bstrcat(name, " - ");

		ret = cjson_get_childstr(basic_information, "title", albnam);
		if(ret != 0) {
			blogf("Release didn't contain title");
			err = ENOENT;
			goto end_label;
		}

		bstrcat(name, bget(albnam));

#if 1
		printf("%s\n", bget(name));
#endif

		ret = save_album(bget(name), *idx < RECENT_CNT?1:0);
		if(ret != 0) {
			blogf("Couldn't save album");
			err = ret;
			goto end_label;
		}

		bclear(albnam);
		bclear(name);

		++*idx;
	}


end_label:

	buninit(&albnam);
	buninit(&artistnam);
	buninit(&name);

	return err;
}


int
save_album(const char *name, int isrecent)
{
	if(xstrempty(name))
		return EINVAL;

	return 0;
}
