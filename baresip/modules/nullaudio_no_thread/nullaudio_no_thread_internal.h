#ifndef NULLAUDIO_NO_THREAD_INTERNAL_H
#define NULLAUDIO_NO_THREAD_INTERNAL_H

/**
 * @file
 * "NULL" audio device for STM32 (no extra thread)
 */


struct dspbuf {
	struct mbuf *mb;
};


int nullaudio_no_thread_src_alloc(struct ausrc_st **stp, struct ausrc *as,
			  struct media_ctx **ctx,
			  struct ausrc_prm *prm, const char *device,
			  ausrc_read_h *rh, ausrc_error_h *errh, void *arg);
int nullaudio_no_thread_play_alloc(struct auplay_st **stp, struct auplay *ap,
		       struct auplay_prm *prm, const char *device,
		       auplay_write_h *wh, void *arg);

typedef void (poll_callback)(void *arg);
int nullaudio_no_thread_register(poll_callback* cb, void *arg);
void nullaudio_no_thread_unregister(void *arg);

#endif
