/*
** wifimgr.h
*/

/*
** Copyright (c) 2009, J.R. Oldroyd, Open Advisors Limited
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the author, the author's organization nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
** 
** THIS SOFTWARE IS PROVIDED BY OPEN ADVISORS LIMITED ''AS IS'' AND ANY
** EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL OPEN ADVISORS LIMITED BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
** $Id: wifimgr.h 102 2014-05-10 20:53:58Z jr $
*/

#define NETWORKS_FILE	"/etc/wpa_supplicant.conf"
#define RC_CONF_FILE	"/etc/rc.conf"
#define RC_CONF_LOCAL_FILE	"/etc/rc.conf.local"

#define ICON_PATH	"/usr/local/share/wifimgr/icons"

#define PATH_CAT	"/bin/cat"
#define PATH_CP		"/bin/cp"
#define PATH_DIFF	"/usr/bin/diff"
#define PATH_IFCONFIG	"/sbin/ifconfig"
#define PATH_NETIF	"/etc/rc.d/netif"
#define PATH_SED	"/usr/bin/sed"
#define PATH_WIFIMGRSU	"/usr/local/libexec/wifimgrsu"

#define ETCDIR		"/etc"
#define TMPDIR		"/tmp"

struct wifi_net {
	int			wn_enabled;
	unsigned int		wn_security;
#define WN_SEC_UNKNOWN		0x0
#define WN_SEC_NONE		0x1
#define WN_SEC_WEP		0x2
#define WN_SEC_WPA		0x4
#define WN_SEC_RSN		0x8
	unsigned int		wn_km;
#define WN_KM_UNKNOWN		0x0
#define WN_KM_NONE		0x1
#define WN_KM_PSK		0x2
#define WN_KM_EAP		0x4
	char			wn_key[256];
	char			wn_comment[256];
	int			wn_any_bssid;
	int			wn_chan;
	int			wn_rate;
	int			wn_bars;
	struct wifi_net *	wn_next;

	int			wn_show_password;
	/* next four are really GtkWidget * pointers */
	void *			w_wn_key;
	void *			w_sup_password;
	void *			w_sup_private_key_passwd;
	void *			w_sup_private_key2_passwd;

	char			sup_anonymous_identity[256];
	char			sup_auth_alg[256];
	char			sup_bssid[256];
	char			sup_ca_cert[256];
	char			sup_ca_cert2[256];
	char			sup_client_cert[256];
	char			sup_client_cert2[256];
	char			sup_dh_file[256];
	char			sup_dh_file2[256];
	char			sup_eap[256];
	int			sup_eapol_flags;
	char			sup_eappsk[256];
	int			sup_eap_workaround;
	char			sup_group[256];
	char			sup_identity[256];
	char			sup_key_mgmt[256];
	int			sup_mixed_cell;
	int			sup_mode;
	char			sup_nai[256];
	char			sup_pac_file[256];
	char			sup_pairwise[256];
	char			sup_password[256];
	char			sup_phase1[256];
	char			sup_phase2[256];
	int			sup_priority;
	char			sup_private_key[256];
	char			sup_private_key2[256];
	char			sup_private_key_passwd[256];
	char			sup_private_key2_passwd[256];
	char			sup_proto[256];
	char			sup_psk[256];
	int			sup_scan_ssid;
	char			sup_server_nai[256];
	char			sup_subject_match[256];
	char			sup_subject_match2[256];
	char			sup_ssid[256];
	char			sup_wep_key0[256];
	char			sup_wep_key1[256];
	char			sup_wep_key2[256];
	char			sup_wep_key3[256];
	int			sup_wep_tx_keyidx;
};

struct conflist {
	char *			cl_name;
	int			cl_value;
};

struct conflist			conf_auth_alg[4];
struct conflist			conf_eap[8];
struct conflist			conf_group[5];
struct conflist			conf_key_mgmt[4];
struct conflist			conf_pairwise[4];
struct conflist			conf_proto[4];
extern struct wifi_net *	nets;
extern char *			wifi_if;
extern int			wifi_if_status;
#define WIFI_IF_DOWN		0
#define WIFI_IF_UP		1

extern int			toggle_intf_up_down(char *);
extern int			ifconfig_intf_status(char *);
extern char *			ifconfig_associated_network(char *);
extern struct wifi_net *	build_network_list(char *);
extern int			save_networks_file(char *);
extern int			restart_intf(void);

extern int			chop(char * s);

extern int			nl_insert(struct wifi_net **, struct wifi_net *);
#define NL_INSERT_ERROR		0
#define NL_INSERT_OK		1
#define NL_INSERT_SEC_CHG	2
extern int			nl_delete_list(struct wifi_net **);

extern int			gui_changes;
extern char			gui_response[];
extern int			gui_init();
extern int			gui_message(const char *, int);
#define	MSG_INFO		0
#define	MSG_WARNING		1
#define	MSG_QUESTION		2
#define	MSG_ERROR		3
#define	MSG_INPUT		4
#define MSG_RESPONSE_OK		0
#define MSG_RESPONSE_FAIL	1
#define MSG_RESPONSE_YES	2
#define MSG_RESPONSE_NO		3
extern void			gui_loop();
