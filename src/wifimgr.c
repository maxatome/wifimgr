/*
** wifimgr.c
**	manage WiFi networks
**	scans for available WiFi networks
**	edits /etc/wpa_supplicant.conf
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
** $Id: wifimgr.c 102 2014-05-10 20:53:58Z jr $
*/

#include "wifimgr.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef WITHOUT_NLS
#include <libintl.h>
#else
#define gettext(x)	(x)
#endif

char *			wifi_if;
int			wifi_if_status;
struct wifi_net *	nets;
char			conf_ctrl_interface[256];
char			conf_ctrl_interface_group[256];
int			conf_eapol_version;
int			conf_ap_scan;
int			conf_fast_reauth;
FILE *			sucmd;

struct conflist	conf_auth_alg[] = {
	{ "OPEN",	0x01 },
	{ "SHARED",	0x02 },
	{ "LEAP",	0x04 },
	{ NULL,		0 }
};

struct conflist	conf_eap[] = {
	{ "MD5",	0x01 },
	{ "MSCHAPV2",	0x02 },
	{ "OTP",	0x04 },
	{ "GTC",	0x08 },
	{ "TLS",	0x10 },
	{ "PEAP",	0x20 },
	{ "TTLS",	0x40 },
	{ NULL,		0 }
};

struct conflist	conf_group[] = {
	{ "WEP40",	0x01 },
	{ "WEP104",	0x02 },
	{ "CCMP",	0x04 },
	{ "TKIP",	0x08 },
	{ NULL,		0 }
};

struct conflist	conf_key_mgmt[] = {
	{ "NONE",	0x01 },
	{ "WPA-PSK",	0x02 },
	{ "WPA-EAP",	0x04 },
	{ NULL,		0 }
};

struct conflist	conf_pairwise[] = {
	{ "NONE",	0 },
	{ "CCMP",	0x01 },
	{ "TKIP",	0x02 },
	{ NULL,		0 }
};

struct conflist	conf_proto[] = {
	{ "WPA",	0x01 },
	{ "RSN",	0x02 },
	{ "WPA2",	0x02 },	/* same value as RSN */
	{ NULL,		0 }
};

/*
** parse /etc/rc.conf to find WiFi interface
*/
char *
find_wifi_if(char * file) {
	FILE *			fp;
	char			line[1024];
	char *			p;
	static char		wifi_if[256];

	if ((fp = fopen(file, "r")) == NULL)
		return NULL;

	while(fgets(line, sizeof(line), fp) != NULL) {
		for (p = line; *p == ' ' || *p == '\t'; p++);
		if (*p == '#')
			continue;
		if (strncasecmp(p, "ifconfig_", 9) == 0
		    && strcasestr(p, "=\"")
		    && strcasestr(p, "WPA")
		) {
			sscanf(p, "ifconfig_%[^=]=", wifi_if);
			fclose(fp);
			return wifi_if;
		}
	}

	fclose(fp);

	return NULL;
}

/*
** validate list values in /etc/wpa_supplicant.conf file
*/
int
parselist(char * s, char * n, struct conflist * cl) {
	char *			p;
	char			w[256];
	int			i;
	int			j;
	unsigned		dup = 0;

	while (*n) {
		for (p = n; *p == ' ' || *p == '\t'; p++);
		for (n = p; *p && *p != ' ' && *p != '\t'; p++);
		strncpy(w, n, p-n);
		w[p-n] = '\0';
		n = p;
		for (i = 0; cl[i].cl_name; i++)
			if (strcasecmp(w, cl[i].cl_name) == 0)
				break;
		if (!cl[i].cl_name)
			return 0;
		if (cl[i].cl_value > 0 && !(dup & cl[i].cl_value)) {
			dup |= cl[i].cl_value;
			if (*s)
				strcat(s, " ");
			for (j = 0; cl[j].cl_value != cl[i].cl_value; j++);
			strcat(s, cl[j].cl_name);
		}
	}

	return 1;
}

/*
** parse /etc/wpa_supplicant.conf for currently configured networks
*/
void
read_networks_file(char * file) {
	char			line[1024];
	char			resp[256];
	char *			p;
	struct wifi_net *	new;
	int			extra_config = 0;

	fprintf(sucmd, "cat_netfile\n");

	while(fgets(line, sizeof(line), sucmd) != NULL) {
		chop(line);
		if (strcmp(line, "EOF") == 0)
			break;
		for (p = line; *p == ' ' || *p == '\t'; p++);
		if (! *p)
			continue;
		else if (strncmp(p, "ctrl_interface=", 15) == 0)
			strcpy(conf_ctrl_interface, p+15);
		else if (strncmp(p, "ctrl_interface_group=", 21) == 0)
			strcpy(conf_ctrl_interface_group, p+21);
		else if (strncmp(p, "eapol_version=", 14) == 0)
			sscanf(p, "eapol_version=%d", &conf_eapol_version);
		else if (strncmp(p, "ap_scan=", 8) == 0)
			sscanf(p, "ap_scan=%d", &conf_ap_scan);
		else if (strncmp(p, "fast_reauth=", 12) == 0)
			sscanf(p, "fast_reauth=%d", &conf_fast_reauth);
		else if (strncmp(p, "network={", 9) == 0) {
			if ((new = (struct wifi_net *) malloc(sizeof(struct wifi_net))) == NULL) {
				char		buf[256];
				sprintf(buf, gettext("Out of memory."));
				gui_message(buf, MSG_ERROR);
				exit(1);
			}
			memset(new, 0, sizeof(*new));

			new->wn_enabled = 1;
			new->wn_next = NULL;
		}
		else if (new && strncmp(p, "#:", 2) == 0) {
			for (p += 2; *p == ' ' || *p == '\t'; p++);
			strcpy(new->wn_comment, p);
		}
		else if (new && strncmp(p, "#+", 2) == 0) {
			for (p += 2; *p == ' ' || *p == '\t'; p++);
			if (strcmp(p, "any_bssid") == 0)
				new->wn_any_bssid = 1;
		}
		else if (*p == '#')
			continue;
		else if (new && strncmp(p, "anonymous_identity=", 19) == 0)
			sscanf(p, "anonymous_identity=\"%[^\"]\"", new->sup_anonymous_identity);
		else if (new && strncmp(p, "auth_alg=", 9) == 0) {
			if (!parselist(new->sup_auth_alg, p+9, conf_auth_alg))
				extra_config++;
		}
		else if (new && strncmp(p, "bssid=", 6) == 0)
			strcpy(new->sup_bssid, p+6);
		else if (new && strncmp(p, "ca_cert=", 8) == 0)
			sscanf(p, "ca_cert=\"%[^\"]\"", new->sup_ca_cert);
		else if (new && strncmp(p, "ca_cert2=", 9) == 0)
			sscanf(p, "ca_cert2=\"%[^\"]\"", new->sup_ca_cert2);
		else if (new && strncmp(p, "client_cert=", 12) == 0)
			sscanf(p, "client_cert=\"%[^\"]\"", new->sup_client_cert);
		else if (new && strncmp(p, "client_cert2=", 13) == 0)
			sscanf(p, "client_cert2=\"%[^\"]\"", new->sup_client_cert2);
		else if (new && strncmp(p, "dh_file=", 8) == 0)
			sscanf(p, "dh_file=\"%[^\"]\"", new->sup_dh_file);
		else if (new && strncmp(p, "dh_file2=", 9) == 0)
			sscanf(p, "dh_file2=\"%[^\"]\"", new->sup_dh_file2);
		else if (new && strncmp(p, "eap=", 4) == 0) {
			if (!parselist(new->sup_eap, p+4, conf_eap))
				extra_config++;
			new->wn_km |= WN_KM_EAP;
			if (! new->wn_security) {
				if (strstr(new->sup_eap, "MD5") ||
				    strstr(new->sup_eap, "MSCHAPV2") ||
				    strstr(new->sup_eap, "OTP") ||
				    strstr(new->sup_eap, "GTC"))
					new->wn_security |= WN_SEC_RSN;
				else
					new->wn_security |= WN_SEC_RSN | WN_SEC_WPA;
			}
		}
		else if (new && strncmp(p, "eapol_flags=", 12) == 0)
			sscanf(p, "eapol_flags=%d", &new->sup_eapol_flags);
		else if (new && strncmp(p, "eappsk=", 7) == 0)
			strcpy(new->sup_eappsk, p+7);
		else if (new && strncmp(p, "eap_workaround=", 15) == 0)
			sscanf(p, "eap_workaround=%d", &new->sup_eap_workaround);
		else if (new && strncmp(p, "group=", 6) == 0) {
			if (!parselist(new->sup_group, p+6, conf_group))
				extra_config++;
		}
		else if (new && strncmp(p, "identity=", 9) == 0)
			sscanf(p, "identity=\"%[^\"]\"", new->sup_identity);
		else if (new && strncmp(p, "key_mgmt=", 9) == 0) {
			if (!parselist(new->sup_key_mgmt, p+9, conf_key_mgmt))
				extra_config++;
			if (strstr(new->sup_key_mgmt, "WPA-EAP"))
				new->wn_km |= WN_KM_EAP;
			if (strstr(new->sup_key_mgmt, "WPA-PSK"))
				new->wn_km |= WN_KM_PSK;
		}
		else if (new && strncmp(p, "mixed_cell=", 11) == 0)
			sscanf(p, "mixed_cell=%d", &new->sup_mixed_cell);
		else if (new && strncmp(p, "mode=", 5) == 0)
			sscanf(p, "mode=%d", &new->sup_mode);
		else if (new && strncmp(p, "nai=", 4) == 0)
			strcpy(new->sup_nai, p+4);
		else if (new && strncmp(p, "pac_file=", 9) == 0)
			sscanf(p, "pac_file=\"%[^\"]\"", new->sup_pac_file);
		else if (new && strncmp(p, "pairwise=", 9) == 0) {
			if (!parselist(new->sup_pairwise, p+9, conf_pairwise))
				extra_config++;
		}
		else if (new && strncmp(p, "password=", 9) == 0)
			sscanf(p, "password=\"%[^\"]\"", new->sup_password);
		else if (new && strncmp(p, "phase1=", 7) == 0)
			sscanf(p, "phase1=\"%[^\"]\"", new->sup_phase1);
		else if (new && strncmp(p, "phase2=", 7) == 0)
			sscanf(p, "phase2=\"%[^\"]\"", new->sup_phase2);
		else if (new && strncmp(p, "priority=", 9) == 0)
			sscanf(p, "priority=%d", &new->sup_priority);
		else if (new && strncmp(p, "private_key=", 12) == 0)
			sscanf(p, "private_key=\"%[^\"]\"", new->sup_private_key);
		else if (new && strncmp(p, "private_key2=", 13) == 0)
			sscanf(p, "private_key2=\"%[^\"]\"", new->sup_private_key2);
		else if (new && strncmp(p, "private_key_passwd=", 19) == 0)
			sscanf(p, "private_key_passwd=\"%[^\"]\"", new->sup_private_key_passwd);
		else if (new && strncmp(p, "private_key2_passwd=", 20) == 0)
			sscanf(p, "private_key2_passwd=\"%[^\"]\"", new->sup_private_key2_passwd);
		else if (new && strncmp(p, "proto=", 6) == 0) {
			if (!parselist(new->sup_proto, p+6, conf_proto))
				extra_config++;
			if (strstr(new->sup_proto, "RSN"))
				new->wn_security |= WN_SEC_RSN;
			if (strstr(new->sup_proto, "WPA"))
				new->wn_security |= WN_SEC_WPA;
		}
		else if (new && strncmp(p, "psk=", 4) == 0) {
			if (index(p, '"'))
				sscanf(p, "psk=\"%[^\"]\"", new->sup_psk);
			else
				strcpy(new->sup_psk, p+4);
			strcpy(new->wn_key, new->sup_psk);
			new->wn_km |= WN_KM_PSK;
		}
		else if (new && strncmp(p, "scan_ssid=", 10) == 0)
			sscanf(p, "scan_ssid=%d", &new->sup_scan_ssid);
		else if (new && strncmp(p, "server_nai=", 11) == 0)
			strcpy(new->sup_server_nai, p+11);
		else if (new && strncmp(p, "ssid=", 5) == 0) {
			sscanf(p, "ssid=\"%[^\"]\"", new->sup_ssid);
			if (*new->sup_ssid == '*') {
				strcpy(new->sup_ssid, &new->sup_ssid[1]);
				new->sup_priority = -1;
			}
		}
		else if (new && strncmp(p, "subject_match=", 14) == 0)
			sscanf(p, "subject_match=\"%[^\"]\"", new->sup_subject_match);
		else if (new && strncmp(p, "subject_match2=", 15) == 0)
			sscanf(p, "subject_match2=\"%[^\"]\"", new->sup_subject_match2);
		else if (new && strncmp(p, "wep_key0=", 9) == 0) {
			if (index(p, '"'))
				sscanf(p, "wep_key0=\"%[^\"]\"", new->sup_wep_key0);
			else
				strcpy(new->sup_wep_key0, p+9);
			new->wn_security |= WN_SEC_WEP;
		}
		else if (new && strncmp(p, "wep_key1=", 9) == 0) {
			if (index(p, '"'))
				sscanf(p, "wep_key1=\"%[^\"]\"", new->sup_wep_key1);
			else
				strcpy(new->sup_wep_key1, p+9);
			new->wn_security |= WN_SEC_WEP;
		}
		else if (new && strncmp(p, "wep_key2=", 9) == 0) {
			if (index(p, '"'))
				sscanf(p, "wep_key2=\"%[^\"]\"", new->sup_wep_key2);
			else
				strcpy(new->sup_wep_key2, p+9);
			new->wn_security |= WN_SEC_WEP;
		}
		else if (new && strncmp(p, "wep_key3=", 9) == 0) {
			if (index(p, '"'))
				sscanf(p, "wep_key3=\"%[^\"]\"", new->sup_wep_key3);
			else
				strcpy(new->sup_wep_key3, p+9);
			new->wn_security |= WN_SEC_WEP;
		}
		else if (new && strncmp(p, "wep_tx_keyidx=", 14) == 0) {
			sscanf(p, "wep_tx_keyidx=%d", &new->sup_wep_tx_keyidx);
			new->wn_security |= WN_SEC_WEP;
		}
		else if (new && *p == '}') {
			/* if security unset, but there is an eap, assume RSN|WPA */
			if (new->wn_security == WN_SEC_UNKNOWN && new->wn_km & WN_KM_EAP)
				new->wn_security = WN_SEC_RSN | WN_SEC_WPA;

			/* if security unset, but there is a psk, assume WPA */
			if (new->wn_security == WN_SEC_UNKNOWN && new->wn_km & WN_KM_PSK)
				new->wn_security = WN_SEC_WPA;

			/* check for unsecured network */
			if (strstr(new->sup_key_mgmt, "NONE") && !(new->wn_security & WN_SEC_WEP))
				new->wn_security = WN_SEC_NONE;

			/* for WEP, select WEP key */
			if (new->wn_security & WN_SEC_WEP) {
				switch (new->sup_wep_tx_keyidx) {
				case 0:
					strcpy(new->wn_key, new->sup_wep_key0);
					break;
				case 1:
					strcpy(new->wn_key, new->sup_wep_key1);
					break;
				case 2:
					strcpy(new->wn_key, new->sup_wep_key2);
					break;
				case 3:
					strcpy(new->wn_key, new->sup_wep_key3);
					break;
				}
			}

			/* insert it */
			if (!nl_insert(&nets, new)) {
				char		buf[256];

				sprintf(buf, gettext("Error adding SSID <b>%s</b>"), new->sup_ssid);
				gui_message(buf, MSG_ERROR);
				exit(1);
			}

			/* done with this network */
			new = NULL;
		}
		else {
			extra_config++;
		}
	}
	fgets(resp, sizeof(resp), sucmd);
	chop(resp);
	if (strcmp(resp, "OK") != 0) {
		char		buf[256];
		char *		err;

		err = index(resp, ' ') + 1;
		sprintf(buf, gettext("Cannot read configuration file <b>%s</b> - %s."), file, err);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	/* if unknown config directives were found, try to make backup copy */
	if (extra_config) {
		char		resp[256];
		char		save_file[256];

		sprintf(save_file, "%s.save", file);
		if (eaccess(save_file, F_OK) == 0) {
			fprintf(sucmd, "diff_netfile\n");
			fgets(resp, sizeof(resp), sucmd);
			chop(resp);
			if (strcmp(resp, "OK") != 0) {
				char		buf[256];
				/* NOTE TO TRANSLATOR: leave "%s.save" in English */
				sprintf(buf, gettext("Configuration file contains additional directives.\n"
						    "A backup file <b>%s.save</b> with\n"
						    "different contents already exists.  You must rename\n"
						    "the configuration file by hand."), file);
				gui_message(buf, MSG_ERROR);
				exit(1);
			}
		}
		fprintf(sucmd, "backup_netfile\n");
		fgets(resp, sizeof(resp), sucmd);
		chop(resp);
		if (strcmp(resp, "OK") != 0) {
			char		buf[256];
			/* NOTE TO TRANSLATOR: leave "%s.save" in English */
			sprintf(buf, gettext("Cannot write backup configuration file <b>%s.save</b>."), file);
			gui_message(buf, MSG_ERROR);
			exit(1);
		}
		else {
			char		buf[256];
			/* NOTE TO TRANSLATOR: leave "%s.save" in English */
			sprintf(buf, gettext("Configuration file contains additional directives;\n"
				    "a backup copy has been made in <b>%s.save</b>."), file);
			gui_message(buf, MSG_INFO);
		}
	}
}

/*
** check that /etc/wpa_supplicant.conf exists with something in it
*/
int
check_networks_file(char * file) {
	struct stat		sb;

	/*
	** if we know of networks, file must exist or
	** interface is up which also means file must exist
	*/
	if (nets)
		return 1;

	/* if file exists with something in it, don't mess with it */
	if (stat(file, &sb) >= 0 && sb.st_size > 0)
		return 1;

	/* create file with just a comment */
	if (! save_networks_file(file))
		return 0;

	return 1;
}

/*
** check for hexadeximal key of suitable length
*/
int
hexkey(char * s, int security) {

	/* check length */
	if (security & WN_SEC_RSN || security & WN_SEC_WPA)
		if (!(strlen(s) == 64))
			return 0;
	if (security & WN_SEC_WEP)
		if (!(strlen(s) == 10 || strlen(s) == 26))
			return 0;

	/* check for hex chars only */
	while (*s) {
		if (!isxdigit(*s++))
			return 0;
	}

	return 1;
}

/*
** (re-)write /etc/wpa_supplicant.conf
*/
int
save_networks_file(char * file) {
	char *			tmpdir;
	char			tmpfile[256];
	char			resp[256];
	FILE *			fp;
	struct wifi_net *	p;

	/* validate the key lengths */
	for (p = nets; p; p = p->wn_next)
		if (p->wn_enabled) {
			if (p->wn_km & WN_KM_PSK) {
				/* if EAP in use, PSK can be null */
				if (p->wn_km & WN_KM_EAP && *p->sup_eap != '\0' && strlen(p->wn_key) == 0)
					continue;
				if (hexkey(p->wn_key, p->wn_security))
					continue;
				if (strlen(p->wn_key) < 8 || strlen(p->wn_key) > 63) {
					char		buf[256];
					sprintf(buf, gettext("Invalid key for network <b>%s</b>.\n\n"
						    "WPA keys must be 8 to 63 characters long or\n"
						    "a 64 character hexadecimal number."), p->sup_ssid);
					gui_message(buf, MSG_WARNING);
					return 0;
				}
			}
			else if (p->wn_security & WN_SEC_WEP) {
				if (hexkey(p->wn_key, p->wn_security))
					continue;
				if (!(strlen(p->wn_key) == 5 || strlen(p->wn_key) == 13)) {
					char		buf[256];
					sprintf(buf, gettext("Invalid key for network <b>%s</b>.\n\n"
						    "WEP keys must be 5 or 13 character strings or\n"
						    "10 or 26 character hexadecimal numbers."), p->sup_ssid);
					gui_message(buf, MSG_WARNING);
					return 0;
				}
			}
		}

	umask(077);
	tmpdir = getenv("TMPDIR");
	if (!tmpdir)
		tmpdir = TMPDIR;
	sprintf(tmpfile, "%s/wfm.%06d", tmpdir, getpid());
	if ((fp = fopen(tmpfile, "w")) == NULL) {
		char		buf[256];
		sprintf(buf, gettext("Cannot write temporary networks file <b>%s</b>."), tmpfile);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	fprintf(fp, "# /etc/wpa_supplicant.conf written by wifimgr(8)\n\n");

	if (*conf_ctrl_interface)
		fprintf(fp, "	ctrl_interface=%s\n", conf_ctrl_interface);
	if (*conf_ctrl_interface_group)
		fprintf(fp, "	ctrl_interface_group=%s\n", conf_ctrl_interface_group);
	if (conf_eapol_version)
		fprintf(fp, "	eapol_version=%d\n", conf_eapol_version);
	if (conf_ap_scan)
		fprintf(fp, "	ap_scan=%d\n", conf_ap_scan);
	if (conf_fast_reauth)
		fprintf(fp, "	fast_reauth=%d\n", conf_fast_reauth);

	if (*conf_ctrl_interface || *conf_ctrl_interface_group || conf_eapol_version ||
	    conf_ap_scan || conf_fast_reauth)
		fprintf(fp, "\n");

	for (p = nets; p; p = p->wn_next)
		if (p->wn_enabled) {
			fprintf(fp, "network={\n");
			if (p->wn_comment[0] != '\0')
				fprintf(fp, "	#: %s\n", p->wn_comment);
			if (p->sup_priority >= 0)
				fprintf(fp, "	ssid=\"%s\"\n", p->sup_ssid);
			else
				fprintf(fp, "	ssid=\"*%s\"\n", p->sup_ssid);
			if (p->sup_scan_ssid)
				fprintf(fp, "	scan_ssid=%d\n", p->sup_scan_ssid);
			if (p->sup_priority > 0)
				fprintf(fp, "	priority=%d\n", p->sup_priority);
			if (!p->wn_any_bssid) {
				if (strcmp(p->sup_bssid, "") != 0)
					fprintf(fp, "	bssid=%s\n", p->sup_bssid);
			}
			else
				fprintf(fp, "	#+ any_bssid\n");
			if (p->wn_km & WN_KM_EAP || p->wn_km & WN_KM_PSK) {
				if (p->wn_km & WN_KM_EAP && p->wn_km & WN_KM_PSK)
					fprintf(fp, "	key_mgmt=WPA-EAP WPA-PSK\n");
				else if (p->wn_km & WN_KM_EAP) {
					if (p->wn_security & WN_SEC_WEP)
						fprintf(fp, "	key_mgmt=IEEE8021X\n");
					else
						fprintf(fp, "	key_mgmt=WPA-EAP\n");
				}
				else
					fprintf(fp, "	key_mgmt=WPA-PSK\n");
				if (*p->sup_proto)
					fprintf(fp, "	proto=%s\n", p->sup_proto);
			}
			if (p->wn_km & WN_KM_EAP) {
				if (*p->sup_eap)
					fprintf(fp, "	eap=%s\n", p->sup_eap);
				if (*p->sup_pairwise)
					fprintf(fp, "	pairwise=%s\n", p->sup_pairwise);
				if (*p->sup_group)
					fprintf(fp, "	group=%s\n", p->sup_group);
				if (*p->sup_identity)
					fprintf(fp, "	identity=\"%s\"\n", p->sup_identity);
				if (*p->sup_anonymous_identity)
					fprintf(fp, "	anonymous_identity=\"%s\"\n", p->sup_anonymous_identity);
				if (p->sup_mixed_cell)
					fprintf(fp, "	mixed_cell=%d\n", p->sup_mixed_cell);
				if (*p->sup_password)
					fprintf(fp, "	password=\"%s\"\n", p->sup_password);
				if (*p->sup_ca_cert)
					fprintf(fp, "	ca_cert=\"%s\"\n", p->sup_ca_cert);
				if (*p->sup_client_cert)
					fprintf(fp, "	client_cert=\"%s\"\n", p->sup_client_cert);
				if (*p->sup_private_key)
					fprintf(fp, "	private_key=\"%s\"\n", p->sup_private_key);
				if (*p->sup_private_key_passwd)
					fprintf(fp, "	private_key_passwd=\"%s\"\n", p->sup_private_key_passwd);
				if (*p->sup_dh_file)
					fprintf(fp, "	dh_file=\"%s\"\n", p->sup_dh_file);
				if (*p->sup_subject_match)
					fprintf(fp, "	subject_match=\"%s\"\n", p->sup_subject_match);
				if (*p->sup_phase1)
					fprintf(fp, "	phase1=\"%s\"\n", p->sup_phase1);
				if (*p->sup_ca_cert2)
					fprintf(fp, "	ca_cert2=\"%s\"\n", p->sup_ca_cert2);
				if (*p->sup_client_cert2)
					fprintf(fp, "	client_cert2=\"%s\"\n", p->sup_client_cert2);
				if (*p->sup_private_key2)
					fprintf(fp, "	private_key2=\"%s\"\n", p->sup_private_key2);
				if (*p->sup_private_key2_passwd)
					fprintf(fp, "	private_key2_passwd=\"%s\"\n", p->sup_private_key2_passwd);
				if (*p->sup_dh_file2)
					fprintf(fp, "	dh_file2=\"%s\"\n", p->sup_dh_file2);
				if (*p->sup_subject_match2)
					fprintf(fp, "	subject_match2=\"%s\"\n", p->sup_subject_match2);
				if (*p->sup_phase2)
					fprintf(fp, "	phase2=\"%s\"\n", p->sup_phase2);
				if (*p->sup_eappsk)
					fprintf(fp, "	eappsk=\"%s\"\n", p->sup_eappsk);
				if (*p->sup_nai)
					fprintf(fp, "	nai=\"%s\"\n", p->sup_nai);
				if (*p->sup_server_nai)
					fprintf(fp, "	server_nai=\"%s\"\n", p->sup_server_nai);
				if (*p->sup_pac_file)
					fprintf(fp, "	pac_file=\"%s\"\n", p->sup_pac_file);
				if (p->sup_eap_workaround)
					fprintf(fp, "	eap_workaround=%d\n", p->sup_eap_workaround);
			}
			if (p->wn_km & WN_KM_PSK) {
				if (!hexkey(p->wn_key, p->wn_security))
					fprintf(fp, "	psk=\"%s\"\n", p->wn_key);
				else
					fprintf(fp, "	psk=%s\n", p->wn_key);
			}
			if (p->wn_security & WN_SEC_WEP) {
				fprintf(fp, "	key_mgmt=NONE\n");
				switch(p->sup_wep_tx_keyidx) {
				case 0:
					strcpy(p->sup_wep_key0, p->wn_key);
					break;
				case 1:
					strcpy(p->sup_wep_key1, p->wn_key);
					break;
				case 2:
					strcpy(p->sup_wep_key2, p->wn_key);
					break;
				case 3:
					strcpy(p->sup_wep_key3, p->wn_key);
					break;
				}
				fprintf(fp, "	wep_tx_keyidx=%d\n", p->sup_wep_tx_keyidx);
				if (*p->sup_wep_key0) {
					if (!hexkey(p->sup_wep_key0, p->wn_security))
						fprintf(fp, "	wep_key0=\"%s\"\n", p->sup_wep_key0);
					else
						fprintf(fp, "	wep_key0=%s\n", p->sup_wep_key0);
				}
				if (*p->sup_wep_key1) {
					if (!hexkey(p->sup_wep_key1, p->wn_security))
						fprintf(fp, "	wep_key1=\"%s\"\n", p->sup_wep_key1);
					else
						fprintf(fp, "	wep_key1=%s\n", p->sup_wep_key1);
				}
				if (*p->sup_wep_key2) {
					if (!hexkey(p->sup_wep_key2, p->wn_security))
						fprintf(fp, "	wep_key2=\"%s\"\n", p->sup_wep_key2);
					else
						fprintf(fp, "	wep_key2=%s\n", p->sup_wep_key2);
				}
				if (*p->sup_wep_key3) {
					if (!hexkey(p->sup_wep_key3, p->wn_security))
						fprintf(fp, "	wep_key3=\"%s\"\n", p->sup_wep_key3);
					else
						fprintf(fp, "	wep_key3=%s\n", p->sup_wep_key3);
				}
			}
			if (p->wn_security & WN_SEC_NONE) {
				fprintf(fp, "	key_mgmt=NONE\n");
				if (p->sup_mode)
					fprintf(fp, "	mode=%d\n", p->sup_mode);
			}
			fprintf(fp, "}\n\n");
		}

	fclose(fp);

	/* execute su command to copy file */
	fprintf(sucmd, "write_netfile %s\n", tmpfile);
	fgets(resp, sizeof(resp), sucmd);
	chop(resp);
	unlink(tmpfile);
	if (strcmp(resp, "OK") != 0) {
		char		buf[256];
		char *		err;

		err = index(resp, ' ') + 1;
		sprintf(buf, gettext("Cannot write configuration file <b>%s</b> - %s."), NETWORKS_FILE, err);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	return 1;
}

/*
** restart interface
*/
int
restart_intf() {
	char			resp[256];
	char			count = 0;

	/* there must be a config file for wpa_supplicant(8) to start */
	check_networks_file(NETWORKS_FILE);

	fprintf(sucmd, "restart_netif %s\n", wifi_if);
	fgets(resp, sizeof(resp), sucmd);
	chop(resp);
	if (strcmp(resp, "OK") != 0) {
		char		buf[256];
		char *		err;

		err = index(resp, ' ') + 1;
		sprintf(buf, gettext("Cannot reset interface <b>%s</b> - %s."), wifi_if, err);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	/* loop up to 10 seconds for interface to re-associate */
	while (!ifconfig_associated_network(wifi_if) && count++ < 20)
		usleep(500000);

	return 1;
}

/*
** scan for currently available networks
*/
void
ifconfig_network_scan(char * intf) {
	char			line[1024];
	char			cmd[256];
	FILE *			fp;
	char			ssid[256];
	char			bssid[256];
	int			chan;
	char			rate[256];
	int			sn_sig;
	int			sn_noise;
	char			caps[256];
	char			rest[1024];
	int			cloaked;
	struct wifi_net *	new;

	if (wifi_if_status != WIFI_IF_UP)
		return;

	sprintf(cmd, "%s -v %s list scan | %s 's/^ /-/'", PATH_IFCONFIG, intf, PATH_SED);

	if ((fp = popen(cmd, "r")) == NULL) {
		char		buf[256];
		sprintf(buf, gettext("Cannot exec %s."), PATH_IFCONFIG);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	fgets(line, sizeof(line), fp);	/* skip header line */
	while(fgets(line, sizeof(line), fp) != NULL) {
		char *			p;

		sscanf(line, "%32c %s %d %s %d:%d %*d %s %[^\n]\n",
		    ssid, bssid, &chan, rate, &sn_sig, &sn_noise, caps, rest);
		for (p = &ssid[31]; p >= ssid && *p == ' '; p--)
			*p = '\0';
		cloaked = 0;
		if (*ssid == '-' && *(ssid+1) == '\0') {
			/* XXX: SSIDs longer than 24 chars will be truncated "foo..." here */
			sscanf(rest, "SSID<%[^>]>", ssid);
			cloaked = 1;
		}

		if ((new = (struct wifi_net *) malloc(sizeof(struct wifi_net))) == NULL) {
			char		buf[256];
			sprintf(buf, gettext("Out of memory."));
			gui_message(buf, MSG_ERROR);
			exit(1);
		}
		memset(new, 0, sizeof(*new));

		new->wn_enabled = 0;
		strcpy(new->sup_ssid, ssid);
		strcpy(new->sup_bssid, bssid);
		new->sup_scan_ssid = cloaked;
		new->wn_chan = chan;
		new->wn_rate = atoi(rate);
		new->wn_bars = (sn_sig - sn_noise) * 4;
		if (index(caps, 'P')) {
			if (strstr(rest, "RSN<") || strstr(rest, "WPA<")) {
				/* set security methods */
				strcpy(new->sup_proto, "");
				if (strstr(rest, "RSN<")) {
					new->wn_security |= WN_SEC_RSN;
					strcpy(new->sup_proto, "RSN");
				}
				if (strstr(rest, "WPA<")) {
					new->wn_security |= WN_SEC_WPA;
					if (*new->sup_proto)
						strcat(new->sup_proto, " ");
					strcat(new->sup_proto, "WPA");
				}

				/* set key management type */
				if (strstr(rest, "km:8021X-UNSPEC"))
					new->wn_km |= WN_KM_EAP;
				if (strstr(rest, "km:8021X-PSK"))
					new->wn_km |= WN_KM_PSK;
			}
			if (strstr(rest, "WPS<"))
				new->wn_km |= WN_KM_PSK;
			if (new->wn_security == WN_SEC_UNKNOWN)
				new->wn_security = WN_SEC_WEP;
		}
		else
			new->wn_security = WN_SEC_NONE;
		new->wn_next = NULL;

		switch(nl_insert(&nets, new)) {
			char		buf[256];

		case NL_INSERT_OK:
			break;

		case NL_INSERT_ERROR:	
			sprintf(buf, gettext("Error adding SSID <b>%s</b>"), new->sup_ssid);
			gui_message(buf, MSG_ERROR);
			exit(1);

		case NL_INSERT_SEC_CHG:
			sprintf(buf, gettext("Network SSID <b>%s</b> has changed security method.  "
			    "Update that entry and re-save."), new->sup_ssid);
			gui_message(buf, MSG_INFO);
			gui_changes++;
		}
	}

	pclose(fp);
}

/*
** run ifconfig to find interface UP/DOWN status
*/
int
ifconfig_intf_status(char * intf) {
	char			cmd[256];
	FILE *			fp;
	char			line[1024];
	int			status;

	/* run ifconfig command */
	sprintf(cmd, "%s %s", PATH_IFCONFIG, intf);
	if ((fp = popen(cmd, "r")) == NULL) {
		char		buf[256];
		sprintf(buf, gettext("Cannot exec %s."), PATH_IFCONFIG);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	status = WIFI_IF_DOWN;

	/* look for "<UP," */
	if (fgets(line, sizeof(line), fp) != NULL)
		if (strstr(line, "<UP,"))
			status = WIFI_IF_UP;

	/* close pipe */
	pclose(fp);

	return status;
}

/*
** run ifconfig to find active network
**	return BSSID if network is specified with SSID/BSSID
**	return SSID if network is specified with SSID/any_bssid
*/
char *
ifconfig_associated_network(char * intf) {
	char			cmd[256];
	FILE *			fp;
	char			line[1024];
	char *			p;
	char *			q;
	int			associated = 0;
	static char		ssid[256];
	static char		bssid[256];
	struct wifi_net *	net;

	/* run ifconfig command */
	sprintf(cmd, "%s %s", PATH_IFCONFIG, intf);
	if ((fp = popen(cmd, "r")) == NULL) {
		char		buf[256];
		sprintf(buf, gettext("Cannot exec %s."), PATH_IFCONFIG);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	/* clear any previous ssid or bssid */
	ssid[0] = '\0';
	bssid[0] = '\0';

	/* look for ssid */
	while(fgets(line, sizeof(line), fp) != NULL) {
		for (p = line; *p == ' ' || *p == '\t'; p++);
		if (strstr(p, "status: associated"))
			associated = 1;
		if (associated) {
			if ((q = strstr(p, "ssid \"")) != NULL)
				sscanf(q, "ssid \"%[^\"]\"", ssid);
			else if ((q = strstr(p, "ssid ")) != NULL)
				sscanf(q, "ssid %s", ssid);
			if ((q = strstr(p, "bssid ")) != NULL)
				sscanf(q, "bssid %s", bssid);
			if (*ssid && *bssid)
				break;
		}
	}

	/* close pipe */
	pclose(fp);

	if (associated) {
		for (net = nets; net; net = net->wn_next)
			if (strcasecmp(net->sup_ssid, ssid) == 0)
				break;

		if (!net)
			return NULL;

		return (net->wn_any_bssid) ? ssid : bssid;
	}

	return NULL;
}

/*
** toggle interface up or down
*/
int
toggle_intf_up_down(char * intf) {
	char			resp[256];
	int			count = 0;

	/* if we are turning the interface up, there must be a config file for wpa_supplicant(8) to start */
	if (wifi_if_status == WIFI_IF_DOWN)
		check_networks_file(NETWORKS_FILE);

	/* execute su command enable/disable interface */
	fprintf(sucmd, "%s_netif %s\n", (wifi_if_status == WIFI_IF_DOWN) ? "start" : "stop", wifi_if);
	fgets(resp, sizeof(resp), sucmd);
	chop(resp);
	if (strcmp(resp, "OK") != 0) {
		char		buf[256];
		char *		err;

		err = index(resp, ' ') + 1;
		sprintf(buf, gettext("Failed to %s WiFi interface."),
		    (wifi_if_status == WIFI_IF_DOWN) ? gettext("enable") : gettext("disable"));
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	if (wifi_if_status == WIFI_IF_DOWN)
		/* loop up to 10 seconds for interface to re-associate */
		while (!ifconfig_associated_network(wifi_if) && count++ < 20)
			usleep(500000);

	wifi_if_status = ifconfig_intf_status(intf);

	return wifi_if_status;
}

/*
** build list of networks from existing config file and ifconfig scan
*/
struct wifi_net *
build_network_list(char * wifi_if) {

	read_networks_file(NETWORKS_FILE);
	ifconfig_network_scan(wifi_if);

	return nets;
}

/*
** open setuid backend
*/
int
wifimgrsu_init()
{
	char *			pass;
	char			resp[256];

	if (gui_message(gettext("Enter Administrator password:"), MSG_INPUT) != MSG_RESPONSE_OK)
		exit(1);

	pass = gui_response;

	if ((sucmd = popen(PATH_WIFIMGRSU, "r+")) == NULL) {
		char		buf[256];
		sprintf(buf, gettext("Cannot exec %s."), PATH_IFCONFIG);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	/* set line buffering */
	setvbuf(sucmd, (char *) NULL, _IOLBF, 0);

	fprintf(sucmd, "password %s\n", pass);
	fgets(resp, sizeof(resp), sucmd);
	chop(resp);
	if (strcmp(resp, "OK") != 0) {
		char		buf[256];
		char *		err;

		err = index(resp, ' ') + 1;
		sprintf(buf, gettext("Password error - %s."), err);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	/* clear gui_changes flag (reading password sets it) */
	gui_changes = 0;

	return 1;
}

int
main(int argc, char ** argv) {

#ifndef WITHOUT_NLS
	/* set output character set */
	bind_textdomain_codeset("wifimgr", "UTF-8");

	/* set language catalog domain */
	textdomain("wifimgr");
#endif

	/* open display */
	if (!gui_init(&argc, &argv)) {
		exit(1);
	}

	/* open channel to setuid backend */
	wifimgrsu_init();

	/* find WiFi interface */
	if ((wifi_if = find_wifi_if(RC_CONF_LOCAL_FILE)) == NULL &&
	    (wifi_if = find_wifi_if(RC_CONF_FILE)) == NULL) {
		char		buf[256];
		sprintf(buf, gettext("No WiFi interface is configured in <b>%s</b>."), RC_CONF_FILE);
		gui_message(buf, MSG_ERROR);
		exit(1);
	}

	/* find interface status */
	wifi_if_status = ifconfig_intf_status(wifi_if);

	gui_loop();

	exit(0);
}
