/**
 * @file baresip.h  Public Interface to Baresip
 *
 * Copyright (C) 2010 Creytiv.com
 */

#ifndef BARESIP_H__
#define BARESIP_H__

#ifdef __cplusplus
extern "C" {
#endif


/** Defines the Baresip version string */
#define BARESIP_VERSION "0.4.6"

#include "baresip_dialog_info_status.h"
#include "baresip_dialog_info_direction.h"
#include "baresip_presence_status.h"
#include "baresip_recorder.h"
#include "baresip_zrtp.h"

/* forward declarations */
struct sa;
struct sdp_media;
struct sdp_session;
struct sip_msg;
struct ua;
struct vidframe;
struct vidrect;
struct vidsz;
struct config;
struct paging_tx;


/*
 * Account
 */

struct account;

int account_alloc(struct account **accp, const char *sipaddr, const char *pwd);
int account_debug(struct re_printf *pf, const struct account *acc);
int account_auth(const struct account *acc, char **username, char **password,
		 const char *realm);
struct list *account_aucodecl(const struct account *acc);
struct list *account_vidcodecl(const struct account *acc);


/*
 * Call
 */

enum call_event {
	CALL_EVENT_INCOMING,
	CALL_EVENT_TRYING,
	CALL_EVENT_RINGING,
	CALL_EVENT_OUTGOING,
	CALL_EVENT_PROGRESS,
	CALL_EVENT_ESTABLISHED,
	CALL_EVENT_CLOSED,
	CALL_EVENT_TRANSFER,
	CALL_EVENT_TRANSFER_FAILED,
	CALL_EVENT_REINVITE_RECEIVED
};

struct call;

typedef void (call_event_h)(struct call *call, enum call_event ev,
			    const char *str, void *arg);
typedef void (call_dtmf_h)(struct call *call, char key, void *arg);

int  call_modify(struct call *call);
int  call_hold(struct call *call, bool hold);
int  call_send_digit(struct call *call, char key);
int  call_start_tone(struct call *call, unsigned int tone_id, float amplitude, float frequency);
int  call_stop_tone(struct call *call, unsigned int tone_id);
int  call_start_audio(struct call *call);
/** \brief Start call audio source (only), e.g. microphone
*/
int  call_start_audio_extra_source(struct call *call);
bool call_has_audio(const struct call *call);
bool call_has_video(const struct call *call);
int  call_transfer(struct call *call, const char *uri);
int  call_status(struct re_printf *pf, const struct call *call);
int  call_debug(struct re_printf *pf, const struct call *call);
void call_set_handlers(struct call *call, call_event_h *eh,
		       call_dtmf_h *dtmfh, void *arg);
uint16_t      call_scode(const struct call *call);
uint32_t      call_duration(const struct call *call);
int           call_answer_after(const struct call *call);
const char   *call_peeruri(const struct call *call);
const char   *call_peername(const struct call *call);
const char   *call_localuri(const struct call *call);
const char   *call_alert_info(const struct call *call);
const char   *call_access_url(const struct call *call);
int           call_access_url_mode(const struct call *call);
const char   *call_initial_rx_invite(const struct call *call);
const char   *call_pai_peeruri(const struct call *call);
const char   *call_pai_peername(const struct call *call);
struct audio *call_audio(const struct call *call);
struct video *call_video(const struct call *call);
struct list  *call_streaml(const struct call *call);
void          call_enable_rtp_timeout(struct call *call, uint32_t timeout_ms);

/*
 * Paging TX
 */
enum paging_tx_event {
	PAGING_TX_STOPPED = 0
};
typedef int (paging_tx_h)(enum paging_tx_event ev);
int paging_tx_register_handler(paging_tx_h *pagingh);
int paging_tx_alloc(struct paging_tx **txp,
	const struct config *cfg, struct sa *addr,
	const char* forced_src_mod, const char* forced_src_dev,
	const char* codec, unsigned int ptime);
int paging_tx_start(struct paging_tx *tx);
int paging_tx_hangup(struct paging_tx *tx);
struct audio *paging_audio(const struct paging_tx *paging);


/*
 * Conf (utils)
 */


/** Defines the configuration line handler */
typedef int (confline_h)(const struct pl *addr);

int  conf_configure(void);
int  conf_modules(void);
int configure(void);
int load_module2(struct mod **modp, const struct pl *name);
int load_module_dynamic(struct mod **modp, const struct pl *modpath, const struct pl *name);
void conf_path_set(const char *path);
int  conf_path_get(char *path, size_t sz);
int  conf_parse(const char *filename, confline_h *ch);
int  conf_get_vidsz(const struct conf *conf, const char *name,
		    struct vidsz *sz);
bool conf_fileexist(const char *path);
struct conf *conf_cur(void);


/*
 * Config (core configuration)
 */

/** A range of numbers */
struct range {
	uint32_t min;  /**< Minimum number */
	uint32_t max;  /**< Maximum number */
};

/** Audio transmit mode */
enum audio_mode {
	AUDIO_MODE_POLL = 0,         /**< Polling mode                  */
	AUDIO_MODE_THREAD,           /**< Use dedicated thread          */
	AUDIO_MODE_THREAD_REALTIME,  /**< Use dedicated realtime-thread */
	AUDIO_MODE_TMR               /**< Use timer                     */
};

enum e_opus_application {
	OPUS_APP_AUDIO = 0,
	OPUS_APP_VOIP
};

/** Core configuration */
struct config {
	/** SIP User-Agent */
	struct config_sip {
		uint32_t trans_bsize;   /**< SIP Transaction bucket size    */
		char uuid[64];          /**< Universally Unique Identifier  */
		char local[64];         /**< Local SIP Address              */
		char cert[256];         /**< SIP Certificate                */
		char cafile[256];       /**< SIP CA-file                    */
		char capath[256];       /**< SIP CA-path                    */
		uint32_t transports;    /**< Supported transports mask      */
		enum sip_transp transp; /**< Default outgoing SIP transport protocol */
		bool use_windows_root_ca_store;
		bool verify_server;     /**< Enable SIP TLS verify server   */
	} sip;

	/** Audio */
	struct config_audio {
		char src_mod[24];       /**< Audio source module            */
		char src_dev[64];      /**< Audio source device            */
		char play_mod[24];      /**< Audio playback module          */
		char play_dev[64];     /**< Audio playback device          */
		char alert_mod[24];     /**< Audio alert module             */
		char alert_dev[64];    /**< Audio alert device             */
		char ring_mod[24];      /**< Audio module for incoming ring */
		char ring_dev[64];     /**< Audio device for incoming ring */
		struct range srate;     /**< Audio sampling rate in [Hz]    */
		struct range channels;  /**< Nr. of audio channels (1=mono) */
		uint32_t srate_play;    /**< Opt. sampling rate for player  */
		uint32_t srate_src;     /**< Opt. sampling rate for source  */
		bool src_first;         /**< Audio source opened first      */
		enum audio_mode txmode; /**< Audio transmit mode            */
		unsigned int softvol_tx;/**< Software volume control for TX; formula: sample = (sample * volume)/128 */
		unsigned int softvol_rx;/**< Software volume control for RX; formula: sample = (sample * volume)/128 */
		struct config_agc {
			bool enabled;		/**< on/off switch */
			uint16_t target;	/**< amplitude that AGC should regulate to */
			float max_gain;		/**< maximum allowed gain */
			float attack_rate;	/**<  */
			float release_rate;
		} agc_rx;
		double portaudioInSuggestedLatency;
		double portaudioOutSuggestedLatency;
		bool loop_ring_without_silence;
	} audio;

#ifdef USE_VIDEO
	/** Video */
	struct config_video {
		char src_mod[16];       /**< Video source module            */
		char src_dev[128];      /**< Video source device            */
		unsigned width, height; /**< Video resolution               */
		uint32_t bitrate;       /**< Encoder bitrate in [bit/s]     */
		uint32_t fps;           /**< Video framerate                */
	} video;
#endif

	/** Audio/Video Transport */
	struct config_avt {
		uint8_t rtp_tos;        /**< Type-of-Service for outg. RTP  */
		struct range rtp_ports; /**< RTP port range                 */
		struct range rtp_bw;    /**< RTP Bandwidth range [bit/s]    */
		bool rtcp_enable;       /**< RTCP is enabled                */
		bool rtcp_mux;          /**< RTP/RTCP multiplexing          */
		struct range jbuf_del;  /**< Delay, number of frames        */
		uint32_t rtp_timeout;   /**< RTP Timeout in seconds (0=off) */
	} avt;

	/* Audio recording */
	struct {
		bool enabled;
	} recording;

	/* Audio preprocessor: signal from microphone */
	struct {
		bool enabled;
		bool denoise_enabled;
		bool agc_enabled;
		bool vad_enabled;
		bool dereverb_enabled;
		int agc_level;
	} audio_preproc_tx;

	/* Acoustic Echo Canceller */
	enum e_aec {
		AEC_NONE = 0,
		AEC_SPEEX,
		AEC_WEBRTC
	} aec;

	struct {
		int msInSndCardBuf;
		int skew;
	} webrtc;
#if 0
	struct {
		bool stereo;
		bool sprop_stereo;

		bool set_bitrate;
		uint32_t bitrate;

		bool set_samplerate;
		uint32_t samplerate;

		bool set_cbr;
		bool cbr;

		bool set_inband_fec;
		bool inband_fec;

		bool set_dtx;
		bool dtx;

		bool mirror;
		uint32_t complexity;

		bool set_application;
		enum e_opus_application application;

		bool set_packet_loss;
		uint32_t packet_loss;
	} opus;

	struct {
		bool start_parallel;
		char zid_filename[256];
	} zrtp;
#endif
	/* Network */
	struct config_net {
		char ifname[64];        /**< Bind to interface (optional)   */
	} net;

#ifdef USE_VIDEO
	/* BFCP */
	struct config_bfcp {
		char proto[16];         /**< BFCP Transport (optional)      */
	} bfcp;
#endif

	struct {
		int reply_code;			/**< SIP code used when replying to incoming MESSAGE */
		char reply_reason[64];	/**< Text sent in reply for incoming MESSAGE */
		int do_not_reply;
	} messages;
};

int config_parse_conf(struct config *cfg, const struct conf *conf);
int config_print(struct re_printf *pf, const struct config *cfg);
int config_write_template(const char *file, const struct config *cfg);
struct config *conf_config(void);


/*
 * Contact
 */

struct contact;

typedef int (contact_dlginfo_h)(int id, const struct dialog_data *ddata, unsigned int ddata_cnt);
typedef int (contact_presence_h)(int id, enum presence_status status, const char *note);

int  contact_add(struct contact **contactp, const struct pl *addr, int id, contact_dlginfo_h *dlginfo_h, contact_presence_h *presence_h);
int  contacts_print(struct re_printf *pf, void *unused);
void contact_set_presence(struct contact *c, enum presence_status status, const struct pl *note);
struct sip_addr *contact_addr(const struct contact *c);
struct list     *contact_list(void);
const char      *contact_str(const struct contact *c);
const char      *contact_presence_str(enum presence_status status);
void			contact_set_dialog_info(struct contact *c, const struct dialog_data *ddata, unsigned int ddata_cnt);
const char		*contact_dialog_info_str(enum dialog_info_status status);


/*
 * Media Context
 */

/** Media Context */
struct media_ctx {
	const char *id;  /**< Media Context identifier */
};


/*
 * Message
 */

typedef void (message_recv_h)(const struct pl *peer, const struct pl *ctype,
			      struct mbuf *body, void *arg, int *reply_code, const char** reply_reason, int *do_not_reply);

int  message_init(message_recv_h *recvh, sip_resp_h *resph, void *arg);
void message_close(void);
int  message_send(struct ua *ua, const char *peer, const char *msg, void *resp_callback_arg);

/*
 * Custom requests
 */
int sip_req_send(struct ua *ua, const char *method, const char *uri,
		 sip_resp_h *resph, void *arg, const char *fmt, ...);	/* duplicating declaration from core.h */

/*
 * Audio Source
 */

struct ausrc;
struct ausrc_st;

/** Audio Source parameters */
struct ausrc_prm {
	int        fmt;         /**< Audio format (enum aufmt) */
	uint32_t   srate;       /**< Sampling rate in [Hz] */
	uint8_t    ch;          /**< Number of channels    */
	uint32_t   frame_size;  /**< Frame size in samples */
};

typedef void (ausrc_read_h)(const uint8_t *buf, size_t sz, void *arg);
typedef void (ausrc_error_h)(int err, const char *str, void *arg);

typedef int  (ausrc_alloc_h)(struct ausrc_st **stp, struct ausrc *ausrc,
			     struct media_ctx **ctx,
			     struct ausrc_prm *prm, const char *device,
			     ausrc_read_h *rh, ausrc_error_h *errh, void *arg);

int ausrc_register(struct ausrc **asp, const char *name,
		   ausrc_alloc_h *alloch);
const struct ausrc *ausrc_find(const char *name);
int ausrc_alloc(struct ausrc_st **stp, struct media_ctx **ctx,
		const char *name,
		struct ausrc_prm *prm, const char *device,
		ausrc_read_h *rh, ausrc_error_h *errh, void *arg);


/*
 * Audio Player
 */

struct auplay;
struct auplay_st;

/** Audio Player parameters */
struct auplay_prm {
	int        fmt;         /**< Audio format (enum aufmt) */
	uint32_t   srate;       /**< Sampling rate in [Hz] */
	uint8_t    ch;          /**< Number of channels    */
	uint32_t   frame_size;  /**< Frame size in samples */
};

typedef bool (auplay_write_h)(uint8_t *buf, size_t sz, void *arg);

typedef int  (auplay_alloc_h)(struct auplay_st **stp, struct auplay *ap,
			      struct auplay_prm *prm, const char *device,
			      auplay_write_h *wh, void *arg);

int auplay_register(struct auplay **pp, const char *name,
		    auplay_alloc_h *alloch);
const struct auplay *auplay_find(const char *name);
int auplay_alloc(struct auplay_st **stp, const char *name,
		 struct auplay_prm *prm, const char *device,
		 auplay_write_h *wh, void *arg);

/*
 * Received signal level - provided by softvol module; maximum from last 100 ms
 */
unsigned int softvol_get_rx_level(void);

/*
 * nullaudio_no_thread polling
 */
void nullaudio_no_thread_poll(void);

/*
 * Audio Filter
 */

struct aufilt;

/* Base class */
struct aufilt_enc_st {
	const struct aufilt *af;
	struct le le;
};

struct aufilt_dec_st {
	const struct aufilt *af;
	struct le le;
};

/** Audio Filter Parameters */
struct aufilt_prm {
	uint32_t srate;       /**< Sampling rate in [Hz]        */
	uint8_t  ch;          /**< Number of channels           */
	uint32_t frame_size;  /**< Number of samples per frame  */
};

typedef int (aufilt_encupd_h)(struct aufilt_enc_st **stp, void **ctx,
			      const struct aufilt *af, struct aufilt_prm *prm);
typedef int (aufilt_encode_h)(struct aufilt_enc_st *st,
			      int16_t *sampv, size_t *sampc);

typedef int (aufilt_decupd_h)(struct aufilt_dec_st **stp, void **ctx,
			      const struct aufilt *af, struct aufilt_prm *prm);
typedef int (aufilt_decode_h)(struct aufilt_dec_st *st,
			      int16_t *sampv, size_t *sampc);

struct aufilt {
	struct le le;
	const char *name;
	aufilt_encupd_h *encupdh;
	aufilt_encode_h *ench;
	aufilt_decupd_h *decupdh;
	aufilt_decode_h *dech;
};

void aufilt_register(struct aufilt *af);
void aufilt_unregister(struct aufilt *af);
struct list *aufilt_list(void);


/*
 * Menc - Media encryption (for RTP)
 */

struct menc;
struct menc_sess;
struct menc_media;


typedef void (menc_error_h)(int err, void *arg);

typedef int  (menc_sess_h)(struct menc_sess **sessp, struct sdp_session *sdp,
			   bool offerer, menc_error_h *errorh, void *arg);

typedef int  (menc_media_h)(struct menc_media **mp, struct menc_sess *sess,
			    struct rtp_sock *rtp, int proto,
			    void *rtpsock, void *rtcpsock,
			    struct sdp_media *sdpm);

struct menc {
	struct le le;
	const char *id;
	const char *sdp_proto;
	menc_sess_h *sessh;
	menc_media_h *mediah;
};

void menc_register(struct menc *menc);
void menc_unregister(struct menc *menc);
const struct menc *menc_find(const char *id);


/*
 * Net - Networking
 */

typedef void (net_change_h)(void *arg);

int  net_init(const struct config_net *cfg, int af);
void net_close(void);
int  net_dnssrv_add(const struct sa *sa);
void net_change(uint32_t interval, net_change_h *ch, void *arg);
bool net_check(void);
int  net_af(void);
int  net_debug(struct re_printf *pf, void *unused);
const struct sa *net_laddr_af(int af);
const char      *net_domain(void);
struct dnsc     *net_dnsc(void);


/*
 * Play - audio file player
 */

struct play;

int  play_file(struct play **playp, const char *mod, const char *dev, const char *filename, int repeat, bool loop_without_silence);
int  play_tone(struct play **playp, const char *mod, const char *dev, struct mbuf *tone,
	       uint32_t srate, uint8_t ch, int repeat, bool loop_without_silence);
void play_init(const struct config *cfg);
void play_close(void);
void play_set_path(const char *path);


/*
 * User Agent
 */

struct ua;

/** Events from User-Agent */
enum ua_event {
	UA_EVENT_REGISTERING = 0,
	UA_EVENT_REGISTER_OK,
	UA_EVENT_REGISTER_FAIL,
	UA_EVENT_UNREGISTERING,
	UA_EVENT_UNREGISTER_OK,
	UA_EVENT_UNREGISTER_FAIL,
	UA_EVENT_CALL_INCOMING,
	UA_EVENT_CALL_OUTGOING,
	UA_EVENT_CALL_TRYING,
	UA_EVENT_CALL_RINGING,
	UA_EVENT_CALL_PROGRESS,
	UA_EVENT_CALL_ESTABLISHED,
	UA_EVENT_CALL_CLOSED,
	UA_EVENT_CALL_TRANSFER_FAILED,
	UA_EVENT_CALL_DTMF_START,
	UA_EVENT_CALL_DTMF_END,
	UA_EVENT_CALL_TRANSFER,
	UA_EVENT_CALL_TRANSFER_OOD,	///< transfer (incoming REFER) outside of dialog
	UA_EVENT_CALL_REINVITE_RECEIVED,
	UA_EVENT_AUDIO_ERROR,

	UA_EVENT_MAX,
};

/** Video mode */
enum vidmode {
	VIDMODE_OFF = 0,    /**< Video disabled                */
	VIDMODE_ON,         /**< Video enabled                 */
};

/** Defines the User-Agent event handler */
typedef void (ua_event_h)(struct ua *ua, enum ua_event ev,
			  struct call *call, const char *prm, void *arg);
typedef void (options_resp_h)(int err, const struct sip_msg *msg, void *arg);

/* Multiple instances */
int  ua_alloc(struct ua **uap, const char *aor, const char *pwd, const char *cuser);
int  ua_connect(struct ua *ua, struct call **callp,
		const char *from_uri, const char *uri,
		const char *params, enum vidmode vmode, const char* extra_hdr_lines);
void ua_hangup(struct ua *ua, struct call *call,
	       uint16_t scode, const char *reason);
int  ua_answer(struct ua *ua, struct call *call, const char *audio_mod, const char *audio_dev);
int  ua_options_send(struct ua *ua, const char *uri,
		     options_resp_h *resph, void *arg);
int  ua_sipfd(const struct ua *ua);
int  ua_debug(struct re_printf *pf, const struct ua *ua);
int  ua_print_calls(struct re_printf *pf, const struct ua *ua);
int  ua_print_status(struct re_printf *pf, const struct ua *ua);
int  ua_print_supported(struct re_printf *pf, const struct ua *ua);
void ua_unregister(struct ua *ua);
bool ua_isregistered(const struct ua *ua);
unsigned int ua_regint(const struct ua *ua);
int	ua_reregister(struct ua *ua);
int ua_play_file(struct ua *ua, const char *audio_mod, const char *audio_dev, const char *filename, int repeat, bool loop_without_silence);
int ua_play_stop(struct ua *ua);
/** Start playing another, separate "ring", with no options to repeat/cancel */
int ua_play_file2(struct ua *ua, const char *audio_mod, const char *audio_dev, const char *filename);
const char     *ua_aor(const struct ua *ua);
const char     *ua_cuser(const struct ua *ua);
struct account *ua_account(const struct ua *ua);
const char     *ua_outbound(const struct ua *ua);
struct call    *ua_call(const struct ua *ua);
struct account *ua_prm(const struct ua *ua);


/* One instance */
int  ua_init(const char *software, bool udp, bool tcp, bool tls, bool prefer_ipv6);
void ua_log_messages(bool log, bool only_first_lines);
void ua_close(void);
void ua_stop_all(bool forced);
int  uag_reset_transp(bool reg, bool reinvite);
int  uag_event_register(ua_event_h *eh, void *arg);
void uag_event_unregister(ua_event_h *eh);
int  ua_print_sip_status(struct re_printf *pf, void *unused);
struct ua   *uag_find(const struct pl *cuser);
struct ua   *uag_find_aor(const char *aor);
struct sip  *uag_sip(void);
const char  *uag_event_str(enum ua_event ev);
struct list *uag_list(void);
struct sipsess_sock  *uag_sipsess_sock(void);
struct sipevent_sock *uag_sipevent_sock(void);


/*
 * Command interface
 */

/** Command flags */
enum {
	CMD_PRM  = (1<<0),              /**< Command with parameter */
	CMD_PROG = (1<<1),              /**< Show progress          */

	CMD_IPRM = CMD_PRM | CMD_PROG,  /**< Interactive parameter  */
};

/** Command arguments */
struct cmd_arg {
	char key;         /**< Which key was pressed  */
	char *prm;        /**< Optional parameter     */
	bool complete;    /**< True if complete       */
};

/** Defines a command */
struct cmd {
	char key;         /**< Input character        */
	int flags;        /**< Optional command flags */
	const char *desc; /**< Description string     */
	re_printf_h *h;   /**< Command handler        */
};

struct cmd_ctx;

int  cmd_register(const struct cmd *cmdv, size_t cmdc);
void cmd_unregister(const struct cmd *cmdv);
int  cmd_process(struct cmd_ctx **ctxp, char key, struct re_printf *pf);
int  cmd_print(struct re_printf *pf, void *unused);


/*
 * Video Source
 */

struct vidsrc;
struct vidsrc_st;

/** Video Source parameters */
struct vidsrc_prm {
	int orient;       /**< Wanted picture orientation (enum vidorient) */
	int fps;          /**< Wanted framerate                            */
};

typedef void (vidsrc_frame_h)(struct vidframe *frame, void *arg);
typedef void (vidsrc_error_h)(int err, void *arg);

typedef int  (vidsrc_alloc_h)(struct vidsrc_st **vsp, struct vidsrc *vs,
			      struct media_ctx **ctx, struct vidsrc_prm *prm,
			      const struct vidsz *size,
			      const char *fmt, const char *dev,
			      vidsrc_frame_h *frameh,
			      vidsrc_error_h *errorh, void *arg);

typedef void (vidsrc_update_h)(struct vidsrc_st *st, struct vidsrc_prm *prm,
			       const char *dev);

int vidsrc_register(struct vidsrc **vp, const char *name,
		    vidsrc_alloc_h *alloch, vidsrc_update_h *updateh);
const struct vidsrc *vidsrc_find(const char *name);
struct list *vidsrc_list(void);
int vidsrc_alloc(struct vidsrc_st **stp, const char *name,
		 struct media_ctx **ctx, struct vidsrc_prm *prm,
		 const struct vidsz *size, const char *fmt, const char *dev,
		 vidsrc_frame_h *frameh, vidsrc_error_h *errorh, void *arg);


/*
 * Video Display
 */

struct vidisp;
struct vidisp_st;

/** Video Display parameters */
struct vidisp_prm {
	void *view;  /**< Optional view (set by application or module) */
};

typedef void (vidisp_resize_h)(const struct vidsz *size, void *arg);

typedef int  (vidisp_alloc_h)(struct vidisp_st **vp,
			      struct vidisp *vd, struct vidisp_prm *prm,
			      const char *dev,
			      vidisp_resize_h *resizeh, void *arg);
typedef int  (vidisp_update_h)(struct vidisp_st *st, bool fullscreen,
			       int orient, const struct vidrect *window);
typedef int  (vidisp_disp_h)(struct vidisp_st *st, const char *title,
			     const struct vidframe *frame);
typedef void (vidisp_hide_h)(struct vidisp_st *st);

int vidisp_register(struct vidisp **vp, const char *name,
		    vidisp_alloc_h *alloch, vidisp_update_h *updateh,
		    vidisp_disp_h *disph, vidisp_hide_h *hideh);
int vidisp_alloc(struct vidisp_st **stp, const char *name,
		 struct vidisp_prm *prm, const char *dev,
		 vidisp_resize_h *resizeh, void *arg);
int vidisp_display(struct vidisp_st *st, const char *title,
		   const struct vidframe *frame);
const struct vidisp *vidisp_find(const char *name);


/*
 * Audio Codec
 */

/** Audio Codec parameters */
struct auenc_param {
	uint32_t ptime;  /**< Packet time in [ms]   */
};

struct auenc_state;
struct audec_state;
struct aucodec;

/* TODO: sample count excludes channel info ? */

typedef int (auenc_update_h)(struct auenc_state **aesp,
			     const struct aucodec *ac,
			     struct auenc_param *prm, const char *fmtp);
typedef int (auenc_encode_h)(struct auenc_state *aes, uint8_t *buf,
			     size_t *len, const int16_t *sampv, size_t sampc);

typedef int (audec_update_h)(struct audec_state **adsp,
			     const struct aucodec *ac, const char *fmtp);
typedef int (audec_decode_h)(struct audec_state *ads, int16_t *sampv,
			     size_t *sampc, const uint8_t *buf, size_t len);
typedef int (audec_plc_h)(struct audec_state *ads,
			  int16_t *sampv, size_t *sampc);

struct aucodec {
	struct le le;
	const char *pt;
	const char *name;
	uint32_t srate;
	uint8_t ch;
	const char *fmtp;
	auenc_update_h *encupdh;
	auenc_encode_h *ench;
	audec_update_h *decupdh;
	audec_decode_h *dech;
	audec_plc_h    *plch;
	sdp_fmtp_enc_h *fmtp_ench;
	sdp_fmtp_cmp_h *fmtp_cmph;
};

void aucodec_register(struct aucodec *ac);
void aucodec_unregister(struct aucodec *ac);
const struct aucodec *aucodec_find(const char *name, uint32_t srate,
				   uint8_t ch);
struct list *aucodec_list(void);


/*
 * Video Codec
 */

/** Video Codec parameters */
struct videnc_param {
	unsigned bitrate;  /**< Encoder bitrate in [bit/s] */
	unsigned pktsize;  /**< RTP packetsize in [bytes]  */
	unsigned fps;      /**< Video framerate            */
	uint32_t max_fs;
};

struct videnc_state;
struct viddec_state;
struct vidcodec;

typedef int (videnc_packet_h)(bool marker, const uint8_t *hdr, size_t hdr_len,
			      const uint8_t *pld, size_t pld_len, void *arg);

typedef int (videnc_update_h)(struct videnc_state **vesp,
			      const struct vidcodec *vc,
			      struct videnc_param *prm, const char *fmtp);
typedef int (videnc_encode_h)(struct videnc_state *ves, bool update,
			      const struct vidframe *frame,
			      videnc_packet_h *pkth, void *arg);

typedef int (viddec_update_h)(struct viddec_state **vdsp,
			      const struct vidcodec *vc, const char *fmtp);
typedef int (viddec_decode_h)(struct viddec_state *vds, struct vidframe *frame,
			      bool marker, uint16_t seq, struct mbuf *mb);

struct vidcodec {
	struct le le;
	const char *pt;
	const char *name;
	const char *variant;
	const char *fmtp;
	videnc_update_h *encupdh;
	videnc_encode_h *ench;
	viddec_update_h *decupdh;
	viddec_decode_h *dech;
	sdp_fmtp_enc_h *fmtp_ench;
	sdp_fmtp_cmp_h *fmtp_cmph;
};

void vidcodec_register(struct vidcodec *vc);
void vidcodec_unregister(struct vidcodec *vc);
const struct vidcodec *vidcodec_find(const char *name, const char *variant);
struct list *vidcodec_list(void);


/*
 * Video Filter
 */

struct vidfilt;

/* Base class */
struct vidfilt_enc_st {
	const struct vidfilt *vf;
	struct le le;
};

struct vidfilt_dec_st {
	const struct vidfilt *vf;
	struct le le;
};

typedef int (vidfilt_encupd_h)(struct vidfilt_enc_st **stp, void **ctx,
			       const struct vidfilt *vf);
typedef int (vidfilt_encode_h)(struct vidfilt_enc_st *st,
			       struct vidframe *frame);

typedef int (vidfilt_decupd_h)(struct vidfilt_dec_st **stp, void **ctx,
			       const struct vidfilt *vf);
typedef int (vidfilt_decode_h)(struct vidfilt_dec_st *st,
			       struct vidframe *frame);

struct vidfilt {
	struct le le;
	const char *name;
	vidfilt_encupd_h *encupdh;
	vidfilt_encode_h *ench;
	vidfilt_decupd_h *decupdh;
	vidfilt_decode_h *dech;
};

void vidfilt_register(struct vidfilt *vf);
void vidfilt_unregister(struct vidfilt *vf);
struct list *vidfilt_list(void);
int vidfilt_enc_append(struct list *filtl, void **ctx,
		       const struct vidfilt *vf);
int vidfilt_dec_append(struct list *filtl, void **ctx,
		       const struct vidfilt *vf);


/*
 * Audio stream
 */

struct audio;

void audio_mute(struct audio *a, bool muted);
/* change audio device: before call */
void audio_set_rx_device(struct audio *a, const char *mod, const char *dev);
void audio_set_tx_device(struct audio *a, const char *mod, const char *dev);
/* change audio device: during call */
int  audio_set_source(struct audio *au, const char *mod, const char *device);
int  audio_set_player(struct audio *au, const char *mod, const char *device);
void audio_encoder_cycle(struct audio *audio);
int  audio_debug(struct re_printf *pf, const struct audio *a);
/* Get name of the codec used in RX direction */
const char* audio_get_rx_aucodec_name(const struct audio *a);


/*
 * Video stream
 */

struct video;

void  video_mute(struct video *v, bool muted);
void *video_view(const struct video *v);
int   video_set_fullscreen(struct video *v, bool fs);
int   video_set_orient(struct video *v, int orient);
void  video_vidsrc_set_device(struct video *v, const char *dev);
int   video_set_source(struct video *v, const char *name, const char *dev);
void  video_set_devicename(struct video *v, const char *src, const char *disp);
void  video_encoder_cycle(struct video *video);
int   video_debug(struct re_printf *pf, const struct video *v);


/*
 * Media NAT
 */

struct mnat;
struct mnat_sess;
struct mnat_media;

typedef void (mnat_estab_h)(int err, uint16_t scode, const char *reason,
			    void *arg);

typedef int (mnat_sess_h)(struct mnat_sess **sessp, struct dnsc *dnsc,
			  const char *srv, uint16_t port,
			  const char *user, const char *pass,
			  struct sdp_session *sdp, bool offerer,
			  mnat_estab_h *estabh, void *arg);

typedef int (mnat_media_h)(struct mnat_media **mp, struct mnat_sess *sess,
			   int proto, void *sock1, void *sock2,
			   struct sdp_media *sdpm);

typedef int (mnat_update_h)(struct mnat_sess *sess);

int mnat_register(struct mnat **mnatp, const char *id, const char *ftag,
		  mnat_sess_h *sessh, mnat_media_h *mediah,
		  mnat_update_h *updateh);


/*
 * Real-time
 */
int realtime_enable(bool enable, int fps);


/*
 * SDP
 */

bool sdp_media_has_media(const struct sdp_media *m);
int  sdp_media_find_unused_pt(const struct sdp_media *m);
int  sdp_fingerprint_decode(const char *attr, struct pl *hash,
			    uint8_t *md, size_t *sz);
uint32_t sdp_media_rattr_u32(const struct sdp_media *sdpm, const char *name);
const char *sdp_rattr(const struct sdp_session *s, const struct sdp_media *m,
		      const char *name);


/*
 * Modules
 */

#ifdef STATIC
#define DECL_EXPORTS(name) exports_ ##name
#else
#define DECL_EXPORTS(name) exports
#endif


#ifdef __cplusplus
}
#endif


#endif /* BARESIP_H__ */
