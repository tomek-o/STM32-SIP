/**
 * @file
 * "NULL" audio device. For input: silence, for output: discarded data.
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "nullaudio_no_thread_internal.h"


#define DEBUG_MODULE "nullaudio_no_thread"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


static struct ausrc *ausrc;
static struct auplay *auplay;


static int nullaudio_no_thread_init(void)
{
	int err;

	err  = ausrc_register(&ausrc, "nullaudio_no_thread", nullaudio_no_thread_src_alloc);
	err |= auplay_register(&auplay, "nullaudio_no_thread", nullaudio_no_thread_play_alloc);

	return err;
}

static int nullaudio_no_thread_close(void)
{
	ausrc = mem_deref(ausrc);
	auplay = mem_deref(auplay);

	return 0;
}

EXPORT_SYM const struct mod_export DECL_EXPORTS(nullaudio_no_thread) = {
	"nullaudio_no_thread",
	"sound",
	nullaudio_no_thread_init,
	nullaudio_no_thread_close
};


static struct nullaudio_no_thread_entry {
    poll_callback *cb;
    void *arg;
} entries[4] = { 0 };

int nullaudio_no_thread_register(poll_callback* cb, void *arg)
{
    unsigned int i;
    if (arg == NULL)
        return EINVAL;
    for (i=0; i<ARRAY_SIZE(entries); i++) {
        struct nullaudio_no_thread_entry *e = &entries[i];
        if (e->arg == NULL) {
            e->cb = cb;
            e->arg = arg;
            return 0;
        }
    }
    return ENOMEM;
}

void nullaudio_no_thread_unregister(void *arg)
{
    unsigned int i;
    for (i=0; i<ARRAY_SIZE(entries); i++) {
        struct nullaudio_no_thread_entry *e = &entries[i];
        if (e->arg == arg) {
            e->cb = NULL;
            e->arg = NULL;
        }
    }
}

void nullaudio_no_thread_poll(void)
{
    unsigned int i;
    for (i=0; i<ARRAY_SIZE(entries); i++) {
        struct nullaudio_no_thread_entry *e = &entries[i];
        if (e->arg && e->cb) {
            e->cb(e->arg);
        }
    }
}
