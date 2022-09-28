/* static.c - manually updated */
#include <re_types.h>
#include <re_mod.h>

extern const struct mod_export exports_g711;
extern const struct mod_export exports_stun;
extern const struct mod_export exports_speex_aec;
extern const struct mod_export exports_speex_pp;
extern const struct mod_export exports_g722;
extern const struct mod_export exports_l16;
extern const struct mod_export exports_presence;
extern const struct mod_export exports_dialog_info;
extern const struct mod_export exports_softvol;
extern const struct mod_export exports_audio_dac;
extern const struct mod_export exports_nullaudio_no_thread;


const struct mod_export *mod_table[] = {
	&exports_g711,
	&exports_stun,
	&exports_speex_aec,
	&exports_speex_pp,
	&exports_g722,
	&exports_l16,
	&exports_presence,
	&exports_dialog_info,
	&exports_softvol,
	&exports_audio_dac,
	&exports_nullaudio_no_thread,
	NULL
};
