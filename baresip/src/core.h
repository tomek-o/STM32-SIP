/**
 * @file core.h  Internal API
 *
 * Copyright (C) 2010 Creytiv.com
 */


/**
 * RFC 3551:
 *
 *    0 -  95  Static payload types
 *   96 - 127  Dynamic payload types
 */
enum {
	PT_CN       = 13,
	PT_STAT_MIN = 0,
	PT_STAT_MAX = 95,
	PT_DYN_MIN  = 96,
	PT_DYN_MAX  = 127
};


enum {
	AUDIO_BANDWIDTH = 128000 /**< Bandwidth for audio in bits/s       */
};


/*
 * Account
 */


/** Defines the answermodes */
enum answermode {
	ANSWERMODE_MANUAL = 0,
	ANSWERMODE_EARLY,
	ANSWERMODE_AUTO
};

enum dtmf_fmt {
	DTMF_FMT_RFC2833 = 0,
	DTMF_FMT_SIP_INFO,
	DTMF_FMT_INBAND
};

struct account {
	char *buf;                   /**< Buffer for the SIP address         */
	char *buf_pwd;               /**< Buffer for the password (separated from SIP address because of decoding problems */
	struct sip_addr laddr;       /**< Decoded SIP address                */
	struct uri luri;             /**< Decoded AOR uri                    */
	char *dispname;              /**< Display name                       */
	char *aor;                   /**< Local SIP uri                      */

	/* parameters: */
	enum answermode answermode;  /**< Answermode for incoming calls      */
	struct le acv[16];           /**< List elements for aucodecl         */
	struct list aucodecl;        /**< List of preferred audio-codecs     */
	char *auth_user;             /**< Authentication username            */
	char *auth_pass;             /**< Authentication password            */
	char *mnatid;                /**< Media NAT handling                 */
	char *mencid;                /**< Media encryption type              */
	const struct mnat *mnat;     /**< MNAT module                        */
	const struct menc *menc;     /**< MENC module                        */
	char *outbound[2];           /**< Optional SIP outbound proxies      */
	uint32_t ptime;              /**< Configured packet time in [ms]     */
	uint32_t regint;             /**< Registration interval in [seconds] */
	char *regq;                  /**< Registration Q-value               */
	char *rtpkeep;               /**< RTP Keepalive mechanism            */
	char *sipnat;                /**< SIP Nat mechanism                  */
	char *stun_user;             /**< STUN Username                      */
	char *stun_pass;             /**< STUN Password                      */
	char *stun_host;             /**< STUN Hostname                      */
	uint16_t stun_port;          /**< STUN Port number                   */
	struct le vcv[4];            /**< List elements for vidcodecl        */
	struct list vidcodecl;       /**< List of preferred video-codecs     */
	uint32_t answer_any;         /**< This account can be used to handle ANY incoming call */
	enum dtmf_fmt dtmf_tx_format;/**< Method used for transmitting DTMF */
	char *cert;                  /**< SIP TLS client certificate+keyfile */ 	
};


/*
 * Audio Player
 */

struct auplay_st {
	struct auplay *ap;
};

struct auplay {
	struct le        le;
	const char      *name;
	auplay_alloc_h  *alloch;
};


/*
 * Audio Source
 */

struct ausrc_st {
	const struct ausrc *as;
};

struct ausrc {
	struct le        le;
	const char      *name;
	ausrc_alloc_h   *alloch;
};


/*
 * Audio Stream
 */

struct audio;

typedef void (audio_event_h)(int key, bool end, void *arg);
typedef void (audio_err_h)(int err, const char *str, void *arg);

int audio_alloc(struct audio **ap, const struct config *cfg,
		struct call *call, struct sdp_session *sdp_sess, int label,
		const struct mnat *mnat, struct mnat_sess *mnat_sess,
		const struct menc *menc, struct menc_sess *menc_sess,
		uint32_t ptime, const struct list *aucodecl,
		audio_event_h *eventh, audio_err_h *errh, void *arg);
/**
	\param forced_src_mod_name override audio mod name from config; NULL to use default
	\param forced_src_dev_name device name for audio source; NULL to use default (from config)
 */
int audio_alloc2(struct audio **ap, const struct config *cfg,
		const char* forced_src_mod, const char* forced_src_dev,
		struct sdp_media * sdp,
		uint32_t ptime,
		audio_event_h *eventh, audio_err_h *errh, void *arg);
int  audio_start(struct audio *a);
/** \brief Start only audio source
*/
int audio_start_extra_source(struct audio *a);
void audio_stop(struct audio *a);
int  audio_encoder_set(struct audio *a, const struct aucodec *ac,
		       int pt_tx, const char *params);
int  audio_decoder_set(struct audio *a, const struct aucodec *ac,
		       int pt_rx, const char *params);
struct stream *audio_strm(const struct audio *a);
int  audio_send_digit_rfc2833(struct audio *a, char key);
int  audio_send_digit_inband(struct audio *a, char key);
int audio_start_tone(struct audio *a, unsigned int tone_id, float amplitude, float frequency);
int audio_stop_tone(struct audio *a, unsigned int tone_id);
void audio_sdp_attr_decode(struct audio *a);

/*
 * BFCP
 */

struct bfcp;
int bfcp_alloc(struct bfcp **bfcpp, struct sdp_session *sdp_sess,
	       const char *proto, bool offerer,
	       const struct mnat *mnat, struct mnat_sess *mnat_sess);
int bfcp_start(struct bfcp *bfcp);


/*
 * Call Control
 */

struct call;

/** Call parameters */
struct call_prm {
	enum vidmode vidmode;
	int af;
};

int  call_alloc(struct call **callp, const struct config *cfg,
		struct list *lst,
		const char *local_name, const char *local_uri,
		struct account *acc, struct ua *ua, const struct call_prm *prm,
		const struct sip_msg *msg, struct call *xcall,
		call_event_h *eh, void *arg);
int  call_connect(struct call *call, const struct pl *paddr, const struct pl *extra_hdr_lines);
int  call_accept(struct call *call, struct sipsess_sock *sess_sock,
		 const struct sip_msg *msg);
int  call_hangup(struct call *call, uint16_t scode, const char *reason);
int  call_progress(struct call *call);
int  call_answer(struct call *call, uint16_t scode, const char *audio_mod, const char *audio_dev);
int  call_sdp_get(const struct call *call, struct mbuf **descp, bool offer);
int  call_jbuf_stat(struct re_printf *pf, const struct call *call);
int  call_info(struct re_printf *pf, const struct call *call);
int  call_reset_transp(struct call *call);
int  call_notify_sipfrag(struct call *call, uint16_t scode,
			 const char *reason, ...);
int  call_af(const struct call *call);
struct ua  *call_get_ua(const struct call *call);


/*
 * Conf
 */

int conf_get_range(const struct conf *conf, const char *name,
		   struct range *rng);
int conf_get_csv(const struct conf *conf, const char *name,
		 char *str1, size_t sz1, char *str2, size_t sz2);


/*
 * Media control
 */

int mctrl_handle_media_control(struct pl *body, bool *pfu);


/*
 * Media NAT traversal
 */

struct mnat {
	struct le le;
	const char *id;
	const char *ftag;
	mnat_sess_h *sessh;
	mnat_media_h *mediah;
	mnat_update_h *updateh;
};

const struct mnat *mnat_find(const char *id);


/*
 * Module
 */

int module_init(const struct conf *conf);
void module_app_unload(void);


/*
 * Network
 */

int net_reset(void);


/*
 * Register client
 */

struct reg;

int  reg_add(struct list *lst, struct ua *ua, int regid);
int  reg_register(struct reg *reg, const char *reg_uri,
		    const char *params, uint32_t regint, const char *outbound);
void reg_unregister(struct reg *reg);
bool reg_isok(const struct reg *reg);
int  reg_sipfd(const struct reg *reg);
int  reg_debug(struct re_printf *pf, const struct reg *reg);
int  reg_status(struct re_printf *pf, const struct reg *reg);


/*
 * RTP keepalive
 */

struct rtpkeep;

int  rtpkeep_alloc(struct rtpkeep **rkp, const char *method, int proto,
		   struct rtp_sock *rtp, struct sdp_media *sdp);
void rtpkeep_refresh(struct rtpkeep *rk, uint32_t ts);


/*
 * SIP Request
 */

int sip_req_send(struct ua *ua, const char *method, const char *uri,
		 sip_resp_h *resph, void *arg, const char *fmt, ...);


/*
 * SDP
 */

int sdp_decode_multipart(const struct pl *ctype, struct mbuf *mb);
const struct sdp_format *sdp_media_format_cycle(struct sdp_media *m);


/*
 * Stream
 */

struct stream;
struct rtp_header;

enum {STREAM_PRESZ = 4+12}; /* same as RTP_HEADER_SIZE */

typedef void (stream_rtp_h)(const struct rtp_header *hdr, struct mbuf *mb,
			    void *arg);
typedef void (stream_rtcp_h)(struct rtcp_msg *msg, void *arg);

typedef void (stream_error_h)(struct stream *strm, int err, void *arg);
int  stream_alloc(struct stream **sp, const struct config_avt *cfg,
		  struct call *call, struct sdp_session *sdp_sess,
		  const char *name, int label,
		  const struct mnat *mnat, struct mnat_sess *mnat_sess,
		  const struct menc *menc, struct menc_sess *menc_sess,
		  stream_rtp_h *rtph, stream_rtcp_h *rtcph, void *arg);
int stream_alloc2(struct stream **sp, const struct config_avt *cfg,
		 struct sdp_media * sdp,
         int af,
		 const char *name, int label,
		 stream_rtp_h *rtph, stream_rtcp_h *rtcph, void *arg);		  
struct sdp_media *stream_sdpmedia(const struct stream *s);
int  stream_start(struct stream *s);
int  stream_send(struct stream *s, bool marker, int pt, uint32_t ts,
		 struct mbuf *mb);
void stream_update(struct stream *s, const char *cname);
void stream_update_encoder(struct stream *s, int pt_enc);
int  stream_jbuf_stat(struct re_printf *pf, const struct stream *s);
void stream_hold(struct stream *s, bool hold);
void stream_set_srate(struct stream *s, uint32_t srate_tx, uint32_t srate_rx);
void stream_send_fir(struct stream *s, bool pli);
void stream_reset(struct stream *s);
void stream_set_bw(struct stream *s, uint32_t bps);
void stream_set_error_handler(struct stream *strm,
			      stream_error_h *errorh, void *arg);
int  stream_debug(struct re_printf *pf, const struct stream *s);
int  stream_print(struct re_printf *pf, const struct stream *s);
void stream_enable_rtp_timeout(struct stream *strm, uint32_t timeout_ms);


/*
 * User-Agent
 */

struct ua;

void         ua_event(struct ua *ua, enum ua_event ev, struct call *call,
		      const char *fmt, ...);
void         ua_printf(const struct ua *ua, const char *fmt, ...);

struct tls  *uag_tls(void);
const char  *uag_allowed_methods(void);

void u32mask_enable(uint32_t *mask, uint8_t bit, bool enable);
bool u32mask_enabled(uint32_t mask, uint8_t bit); 

/*
 * Video Display
 */

struct vidisp {
	struct le        le;
	const char      *name;
	vidisp_alloc_h  *alloch;
	vidisp_update_h *updateh;
	vidisp_disp_h   *disph;
	vidisp_hide_h   *hideh;
};

struct vidisp *vidisp_get(struct vidisp_st *st);


/*
 * Video Source
 */

struct vidsrc {
	struct le         le;
	const char       *name;
	vidsrc_alloc_h   *alloch;
	vidsrc_update_h  *updateh;
};

struct vidsrc *vidsrc_get(struct vidsrc_st *st);


/*
 * Video Stream
 */

struct video;

int  video_alloc(struct video **vp, const struct config *cfg,
		 struct call *call, struct sdp_session *sdp_sess, int label,
		 const struct mnat *mnat, struct mnat_sess *mnat_sess,
		 const struct menc *menc, struct menc_sess *menc_sess,
		 const char *content, const struct list *vidcodecl);
int  video_start(struct video *v, const char *peer);
void video_stop(struct video *v);
int  video_encoder_set(struct video *v, struct vidcodec *vc,
		       int pt_tx, const char *params);
int  video_decoder_set(struct video *v, struct vidcodec *vc, int pt_rx,
		       const char *fmtp);
struct stream *video_strm(const struct video *v);
void video_update_picture(struct video *v);
void video_sdp_attr_decode(struct video *v);
int  video_print(struct re_printf *pf, const struct video *v);

