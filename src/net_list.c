/*
** net_list.c
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
** $Id: net_list.c 88 2011-11-15 15:48:26Z jr $
*/

#include "wifimgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int (*_nl_sort_specific_compar)(struct wifi_net *, struct wifi_net *);

static int
_nl_sort_generic_compar(const void * a, const void * b) {
	return _nl_sort_specific_compar(*(struct wifi_net **)a, *(struct wifi_net **)b);
}

static void
_nl_sort(struct wifi_net ** a, int (*compar)(struct wifi_net *, struct wifi_net *)) {
	struct wifi_net *	p;
	struct wifi_net **	list;
	size_t			num;
	int			i;

	for (p = *a, num = 0; p; p = p->wn_next)
		num++;

	if (num <= 1)
		return;

	list = malloc(num * sizeof(struct wifi_net *));
	if (list == NULL)
		return;

	for (p = *a, i = 0; p; p = p->wn_next, i++)
		list[i] = p;

	_nl_sort_specific_compar = compar;
	qsort(list, num, sizeof(struct wifi_net *), _nl_sort_generic_compar);

	*a = p = list[0];
	for (i = 1; i < num; i++) {
		p->wn_next = list[i];
		p = list[i];
	}
	p->wn_next = NULL;

	free(list);
}

static int
_nl_order_by_ssid_compar(struct wifi_net * a, struct wifi_net * b) {
	int cmp = strcasecmp(a->sup_ssid, b->sup_ssid);
	if (cmp != 0)
		return cmp;

	return strcasecmp(a->sup_bssid, b->sup_bssid);
}

void
nl_order_by_ssid(struct wifi_net ** a) {
	_nl_sort(a, _nl_order_by_ssid_compar);
}

static int
_nl_order_by_bars_compar(struct wifi_net * a, struct wifi_net * b) {
	if (a->wn_bars == b->wn_bars) {
		int cmp = strcasecmp(a->sup_ssid, b->sup_ssid);
		if (cmp != 0)
			return cmp;

		return strcasecmp(a->sup_bssid, b->sup_bssid);
	}

	return b->wn_bars - a->wn_bars;
}

void
nl_order_by_bars(struct wifi_net ** a) {
	_nl_sort(a, _nl_order_by_bars_compar);
}


static int
_nl_order_by_channel_compar(struct wifi_net * a, struct wifi_net * b) {
	if (a->wn_chan == b->wn_chan) {
		int cmp = strcasecmp(a->sup_ssid, b->sup_ssid);
		if (cmp != 0)
			return cmp;

		return strcasecmp(a->sup_bssid, b->sup_bssid);
	}

	/* 0 channel is always last */
	if (a->wn_chan == 0)
		return 1;
	if (b->wn_chan == 0)
		return -1;

	return a->wn_chan - b->wn_chan;
}

void
nl_order_by_channel(struct wifi_net ** a) {
	_nl_sort(a, _nl_order_by_channel_compar);
}

/*
** copy and insert element into list
*/
int
nl_insert(struct wifi_net ** a, struct wifi_net * b) {
	struct wifi_net *	p;
	struct wifi_net *	pp;
	int			security_changed;

	if (!a || !b)
		return NL_INSERT_ERROR;

	/* search for this or next alphabetically higher SSID */
	for (p = *a; p; pp = p, p = p->wn_next)
		if (strcasecmp(p->sup_ssid, b->sup_ssid) >= 0)
			break;

	/* search within SSID for this or empty or next alphabetically higher BSSID */
	for ( ; p; pp = p, p = p->wn_next) {
		if (strcasecmp(p->sup_ssid, b->sup_ssid) != 0)
			break;
		if (strcasecmp(p->sup_bssid, b->sup_bssid) >= 0 || strcmp(p->sup_bssid, "") == 0)
			break;
	}

	security_changed = 0;
	if (p && strcasecmp(p->sup_ssid, b->sup_ssid) == 0 &&
	    (strcasecmp(p->sup_bssid, b->sup_bssid) == 0 ||
	    strcmp(p->sup_bssid, "") == 0 ||
	    p->wn_any_bssid)) {
		/* net is already in list, so merge */
		if (strcmp(p->sup_bssid, "") == 0 && !p->wn_any_bssid)
			strcpy(p->sup_bssid, b->sup_bssid);
		p->wn_chan = b->wn_chan;
		p->wn_rate = b->wn_rate;
		p->wn_bars = b->wn_bars;
		if (! p->wn_security || ! *p->sup_proto) {
			p->wn_security = b->wn_security;
			strcpy(p->sup_proto, b->sup_proto);
		}
		else if (p->wn_security & b->wn_security) {
			/* b is a subset of p, so just reduce p */
			p->wn_security = b->wn_security;
			strcpy(p->sup_proto, b->sup_proto);
		}
		else if (p->wn_security != b->wn_security) {
			p->wn_security = b->wn_security;
			strcpy(p->sup_proto, b->sup_proto);
			security_changed++;
		}
		if (! p->wn_km)
			p->wn_km = b->wn_km;
		else if (p->wn_km != b->wn_km) {
			p->wn_km = b->wn_km;
			security_changed++;
		}
	}
	else {
		/* insert */
		if (p == *a) {
			b->wn_next = *a;
			*a = b;
		}
		else {
			b->wn_next = p;
			pp->wn_next = b;
		}
	}

	return (security_changed) ? NL_INSERT_SEC_CHG : NL_INSERT_OK;
}


/*
** delete all items in list
*/
int
nl_delete_list(struct wifi_net ** a) {
	struct wifi_net *	p;
	struct wifi_net *	pp = NULL;

	if (!a)
		return 0;

	for (p = *a; p; pp = p, p = p->wn_next)
		if (pp)
			free(pp);
	if (p)
		free(p);

	*a = NULL;
	return 1;
}
