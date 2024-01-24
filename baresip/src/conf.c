/**
 * @file conf.c  Configuration utils
 *
 * Copyright (C) 2010 Creytiv.com
 */
#define _BSD_SOURCE 1
#include <lwip/sockets.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <sys/stat.h>
#ifdef HAVE_IO_H
#include <io.h>
#endif
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "core.h"


#define DEBUG_MODULE "conf"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


#if defined(WIN32) || defined(__WIN32__)
#define DIR_SEP "\\"
#else
#define DIR_SEP "/"
#endif


static struct conf *conf_obj;


#if 0
static void print_populated(const char *what, uint32_t n)
{
	(void)re_printf("Populated %u %s%s\n", n, what, 1==n ? "" : "s");
}
#endif


/**
 * Load all modules from config file
 *
 * @return 0 if success, otherwise errorcode
 */
int conf_modules(void)
{
#if 0
	char path[256], file[256];
	int err;

	err = conf_path_get(path, sizeof(path));
	if (err)
		return err;

	if (re_snprintf(file, sizeof(file), "%s/config", path) < 0)
		return ENOMEM;

	err = conf_alloc(&conf_obj, file);
	if (err)
		goto out;

	err = module_init(conf_obj);
	if (err) {
		DEBUG_WARNING("configure module parse error (%m)\n", err);
		goto out;
	}

	print_populated("audio codec",  list_count(aucodec_list()));
	print_populated("audio filter", list_count(aufilt_list()));
#ifdef USE_VIDEO
	print_populated("video codec",  list_count(vidcodec_list()));
	print_populated("video filter", list_count(vidfilt_list()));
#endif

 out:
	conf_obj = mem_deref(conf_obj);
	return err;
#else
	return 0;
#endif
}


/**
 * Get the current configuration object
 *
 * @return Config object
 *
 * @note It is only available during init
 */
struct conf *conf_cur(void)
{
	return conf_obj;
}

int configure(void)
{
	int err = 0;

	struct pl dummy;
	struct pl modname;
	pl_set_str(&dummy, "");

	//dns_server_handler(&dummy, NULL);

	pl_set_str(&modname, "g711");
	load_module2(NULL, &modname);
	pl_set_str(&modname, "g722");
	load_module2(NULL, &modname);
	//pl_set_str(&modname, "gsm");
	//load_module2(NULL, &modname);
	pl_set_str(&modname, "l16");
	load_module2(NULL, &modname);
	//pl_set_str(&modname, "opus");
	//load_module2(NULL, &modname);

	// load all audio I/O modules
	pl_set_str(&modname, "audio_dac");
	load_module2(NULL, &modname);

	pl_set_str(&modname, "audio_adc");
	load_module2(NULL, &modname);

	pl_set_str(&modname, "nullaudio_no_thread");
	load_module2(NULL, &modname);

#if 0
	if (cfg->aec == AEC_SPEEX) {
		pl_set_str(&modname, "speex_aec");
		load_module2(NULL, &modname);
	} else if (cfg->aec == AEC_WEBRTC) {
		pl_set_str(&modname, "webrtc_aec");
		load_module2(NULL, &modname);
	}

	if (cfg->audio_preproc_tx.enabled) {
		pl_set_str(&modname, "speex_pp");
		load_module2(NULL, &modname);
    }

	if (cfg->recording.enabled) {
		pl_set_str(&modname, "recorder");
		load_module2(NULL, &modname);
	}
#endif
	// it is debatable where software volume control should be placed
	// placing it after recorder seems to be safer
	// it may actually require separating this into two modules: softvol_tx and softvol_rx
	pl_set_str(&modname, "softvol");
	load_module2(NULL, &modname);

	pl_set_str(&modname, "stun");
	load_module2(NULL, &modname);
#if 0
	pl_set_str(&modname, "gzrtp");
	load_module2(NULL, &modname);

	pl_set_str(&modname, "srtp");
	load_module2(NULL, &modname);
#endif
	return err;
}
