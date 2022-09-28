/**
 * @file notifier.c Presence notifier
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <re.h>
#include <baresip.h>
#include "presence.h"

#define DEBUG_MODULE "presence-notifier"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

/*
 * Notifier - other people are subscribing to the status of our AOR.
 * we must maintain a list of active notifications. we receive a SUBSCRIBE
 * message from peer, and send NOTIFY to all peers when the Status changes
 */


struct notifier {
	struct le le;
	struct sipevent_sock *sock;
	struct sipnot *not;
	struct ua *ua;
};

static enum presence_status my_status = PRESENCE_OPEN;
static struct list notifierl;
static struct sipevent_sock *evsock;


static const char *presence_status_str(enum presence_status st)
{
	switch (st) {

	case PRESENCE_OPEN:   return "open";
	case PRESENCE_CLOSED: return "closed";
	default: return "?";
	}
}


static int notify(struct notifier *not, enum presence_status status)
{
	const char *aor = ua_aor(not->ua);
	struct mbuf *mb;
	int err;

	mb = mbuf_alloc(1024);
	if (!mb)
		return ENOMEM;

	err = mbuf_printf(mb,
	"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\r\n"
	"<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\r\n"
	"    xmlns:dm=\"urn:ietf:params:xml:ns:pidf:data-model\"\r\n"
	"    xmlns:rpid=\"urn:ietf:params:xml:ns:pidf:rpid\"\r\n"
	"    entity=\"%s\">\r\n"
	"  <dm:person id=\"p4159\"><rpid:activities/></dm:person>\r\n"
	"  <tuple id=\"t4109\">\r\n"
	"    <status>\r\n"
	"      <basic>%s</basic>\r\n"
	"    </status>\r\n"
	"    <contact>%s</contact>\r\n"
	"  </tuple>\r\n"
	"</presence>\r\n"
		    ,aor, presence_status_str(status), aor);
	if (err)
		goto out;

	mb->pos = 0;

	err = sipevent_notify(not->not, mb, SIPEVENT_ACTIVE, 0, 0);
	if (err) {
		DEBUG_WARNING("presence: notify to %s failed (%m)\n", aor, err);
	}

 out:
	mem_deref(mb);
	return err;
}


static void sipnot_close_handler(int err, const struct sip_msg *msg,
				 void *arg)
{
	struct notifier *not = arg;

	if (err) {
		DEBUG_INFO("presence: notifier closed (%m)\n", err);
	}
	else if (msg) {
		DEBUG_INFO("presence: notifier closed (%u %r)\n",
		     msg->scode, &msg->reason);
	}

	not = mem_deref(not);
}


static void destructor(void *arg)
{
	struct notifier *not = arg;

	list_unlink(&not->le);
	mem_deref(not->not);
	mem_deref(not->sock);
	mem_deref(not->ua);
}


static int auth_handler(char **username, char **password,
			const char *realm, void *arg)
{
	return account_auth(arg, username, password, realm);
}


static int notifier_alloc(struct notifier **notp, struct sipevent_sock *sock,
			  const struct sip_msg *msg,
			  const struct sipevent_event *se, struct ua *ua)
{
	struct notifier *not;
	int err;

	if (!sock || !msg || !se)
		return EINVAL;

	not = mem_zalloc(sizeof(*not), destructor);
	if (!not)
		return ENOMEM;

	not->sock = mem_ref(sock);
	not->ua   = mem_ref(ua);

	err = sipevent_accept(&not->not, sock, msg, NULL, se, 200, "OK",
			      600, 600, 600,
			      ua_cuser(not->ua), "application/pidf+xml",
			      auth_handler, ua_prm(not->ua), true,
			      sipnot_close_handler, not, NULL);
	if (err) {
		DEBUG_WARNING("presence: sipevent_accept failed: %m\n", err);
		goto out;
	}

	list_append(&notifierl, &not->le, not);

 out:
	if (err)
		mem_deref(not);
	else if (notp)
		*notp = not;

	return err;
}


static int notifier_add(struct sipevent_sock *sock, const struct sip_msg *msg,
			struct ua *ua)
{
	const struct sip_hdr *hdr;
	struct sipevent_event se;
	struct notifier *not;
	int err;

	hdr = sip_msg_hdr(msg, SIP_HDR_EVENT);
	if (!hdr)
		return EPROTO;

	err = sipevent_event_decode(&se, &hdr->val);
	if (err)
		return err;

	if (pl_strcasecmp(&se.event, "presence")) {
		DEBUG_INFO("presence: unexpected event '%r'\n", &se.event);
		return EPROTO;
	}

	err = notifier_alloc(&not, sock, msg, &se, ua);
	if (err)
		return err;

	(void)notify(not, my_status);

	return 0;
}


static void notifier_update_status(enum presence_status status)
{
	struct le *le;

	if (status == my_status)
		return;

	DEBUG_INFO("presence: update my status from '%s' to '%s'\n",
	     contact_presence_str(my_status),
	     contact_presence_str(status));

	my_status = status;

	for (le = notifierl.head; le; le = le->next) {

		struct notifier *not = le->data;

		(void)notify(not, status);
	}
}


static int cmd_online(struct re_printf *pf, void *arg)
{
	(void)pf;
	(void)arg;
	notifier_update_status(PRESENCE_OPEN);
	return 0;
}


static int cmd_offline(struct re_printf *pf, void *arg)
{
	(void)pf;
	(void)arg;
	notifier_update_status(PRESENCE_CLOSED);
	return 0;
}


static const struct cmd cmdv[] = {
	{'[', 0, "Set presence online",   cmd_online  },
	{']', 0, "Set presence offline",  cmd_offline },
};


static bool sub_handler(const struct sip_msg *msg, void *arg)
{
	struct ua *ua;

	(void)arg;

	ua = uag_find(&msg->uri.user);
	if (!ua) {
		DEBUG_WARNING("presence: no UA found for %r\n", &msg->uri.user);
		(void)sip_treply(NULL, uag_sip(), msg, 404, "Not Found");
		return true;
	}

	if (notifier_add(evsock, msg, ua))
		(void)sip_treply(NULL, uag_sip(), msg, 400, "Bad Presence");

	return true;
}


int notifier_init(void)
{
	int err;

	err = sipevent_listen(&evsock, uag_sip(), 32, 32, sub_handler, NULL);
	if (err)
		return err;

	return cmd_register(cmdv, ARRAY_SIZE(cmdv));
}


void notifier_close(void)
{
	cmd_unregister(cmdv);
	list_flush(&notifierl);
	evsock = mem_deref(evsock);
}
