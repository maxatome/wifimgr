/*
** wifimgr-gtk.c
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
** $Id: wifimgr-gtk.c 102 2014-05-10 20:53:58Z jr $
*/

#include "wifimgr.h"
#include "version.h"
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>

#ifndef WITHOUT_NLS
#include <libintl.h>
#else
#define gettext(x)	(x)
#endif

#define	ALIGN_LEFT	0
#define	ALIGN_CENTER	0.5
#define	ALIGN_RIGHT	1
#define	ALIGN_TOP	0
#define	ALIGN_MIDDLE	0.5
#define	ALIGN_BOTTOM	1

char			gui_response[1024];
int			gui_changes;
GtkWidget *		gui_up_down_icon;
struct wifi_net *	gui_new_net;
gpointer *		gui_new_net_table;

static GtkWidget *	gui_fill_network_table(GtkWidget * x, gpointer * gp);

/*
** initialize GUI
*/
int
gui_init(int * ac, char *** av) {
	return gtk_init_check(ac, av);
}

/*
** handle check_button input by user
*/
static void
gui_process_check_button(GtkWidget * w, gpointer * gp) {

	*((int *) gp) = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	gui_changes++;
}

/*
** handle check_button for show password input by user
*/
static void
gui_process_check_button_showpass(GtkWidget * w, gpointer * gp) {
	struct wifi_net *	net;

	net = (struct wifi_net *) gp;
	net->wn_show_password = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	if (net->w_wn_key)
		gtk_entry_set_visibility(GTK_ENTRY(net->w_wn_key), net->wn_show_password);
	if (net->w_sup_password)
		gtk_entry_set_visibility(GTK_ENTRY(net->w_sup_password), net->wn_show_password);
	if (net->w_sup_private_key_passwd)
		gtk_entry_set_visibility(GTK_ENTRY(net->w_sup_private_key_passwd), net->wn_show_password);
	if (net->w_sup_private_key2_passwd)
		gtk_entry_set_visibility(GTK_ENTRY(net->w_sup_private_key2_passwd), net->wn_show_password);
}

/*
** handle spin button input by user
*/
static void
gui_process_spin_button(GtkWidget * w, gpointer * gp) {

	*((int *) gp) = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(w));
	gui_changes++;
}

/*
** handle text input by user
*/
static void
gui_process_text_input(GtkWidget * w, gpointer * gp) {

	strcpy((char *) gp, (char *) gtk_entry_get_text(GTK_ENTRY(w)));
	gui_changes++;
}

/*
** handle filename selection by user
*/
static void
gui_process_file_selection(GtkWidget * w, gpointer * gp) {
	char *			filename;

	filename = (char *) gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(w));
	if (filename && strcmp((char *) gp, filename) != 0) {
		strcpy((char *) gp, filename);
		g_free(filename);
		gui_changes++;
	}
}

/*
** handle combo_box selection by user
*/
static void
gui_process_combo_box_selection(GtkWidget * w, gpointer * gp) {
	char *			text;

	text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	if (text) {
		if (strcmp(text, gettext("(Unset)")) == 0)
			*((char *) gp) = '\0';
		else
			strcpy((char *) gp, text);
		g_free(text);
		gui_changes++;
	}
}

/*
** handle combo_box security type selection by user
*/
static void
gui_process_security_type_selection(GtkWidget * w, gpointer * gp) {
	char *			text;
	struct wifi_net *	net;

	net = (struct wifi_net *) gp;

	text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	if (text) {
		if (strcmp(text, gettext("Open")) == 0) {
			net->wn_security = WN_SEC_NONE;
			net->wn_km = WN_KM_NONE;
		}
		if (strcmp(text, gettext("WEP")) == 0) {
			net->wn_security = WN_SEC_WEP;
			net->wn_km = WN_KM_NONE;
		}
		if (strcmp(text, gettext("WPA PSK")) == 0) {
			net->wn_security = WN_SEC_WPA;
			net->wn_km = WN_KM_PSK;
		}
		if (strcmp(text, gettext("RSN PSK")) == 0) {
			net->wn_security = WN_SEC_RSN;
			net->wn_km = WN_KM_PSK;
		}
		if (strcmp(text, gettext("WPA EAP")) == 0) {
			net->wn_security = WN_SEC_WPA;
			net->wn_km = WN_KM_EAP;
		}
		if (strcmp(text, gettext("RSN EAP")) == 0) {
			net->wn_security = WN_SEC_RSN;
			net->wn_km = WN_KM_EAP;
		}
		g_free(text);
		gui_changes++;
	}
}

/*
** display GUI message dialog with OK button
*/
int
gui_message(const char * s, int msg_type) {
	GtkWidget *		dialog;
	GtkWidget *		w;
	int			resp;

	switch(msg_type) {
	case MSG_INFO:
		dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_INFO, GTK_BUTTONS_OK, s);
		break;
	case MSG_WARNING:
		dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, s);
		break;
	case MSG_QUESTION:
		dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, s);
		break;
	case MSG_ERROR:
		dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, s);
		break;
	case MSG_INPUT:
		dialog = gtk_message_dialog_new_with_markup(NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
		    GTK_MESSAGE_QUESTION, GTK_BUTTONS_OK, s);
		w = gtk_entry_new();
		gtk_entry_set_visibility(GTK_ENTRY(w), FALSE);
		gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
		gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE); /* allow CR to end input */
		g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
		    (gpointer) gui_response);
		gtk_container_add(GTK_CONTAINER(GTK_MESSAGE_DIALOG(dialog)->label->parent), w);
		gtk_widget_show_all(dialog);
		break;
	}
	gtk_window_set_title(GTK_WINDOW(dialog), gettext("WiFi Networks Manager"));

	resp = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	switch(resp) {
	case GTK_RESPONSE_OK:	return MSG_RESPONSE_OK;
	case GTK_RESPONSE_YES:	return MSG_RESPONSE_YES;
	case GTK_RESPONSE_NO:	return MSG_RESPONSE_NO;
	default:		return MSG_RESPONSE_FAIL;
	}
}

/*
** edit entry for new network
*/
static void
gui_edit_new_network_done(GtkWidget * w, gpointer * gp) {

	gtk_widget_destroy((GtkWidget *) gp);

	/* insert new network */
	if (*gui_new_net->sup_ssid && gui_new_net->wn_security != WN_SEC_UNKNOWN) {
		gui_new_net->wn_enabled = 1;

		/* use directed probe request for cloaked networks */
		gui_new_net->sup_scan_ssid = 1;

		if (!nl_insert(&nets, gui_new_net)) {
			char            buf[256];

			sprintf(buf, gettext("Error adding SSID <b>%s</b>"), gui_new_net->sup_ssid);
			gui_message(buf, MSG_ERROR);
			return;
		}

		/* refresh main display */
		gui_fill_network_table(NULL, gui_new_net_table);
	}
}

static void
gui_edit_new_network(GtkWidget * x, gpointer * gp) {
	GtkWidget *		window;
	GtkWidget *		vbox;
	GtkWidget *		table_scrolled_window;
	GtkWidget *		bottom_buttons;
	GtkWidget *		table;
	GtkWidget *		w;
	int			row;
	int			col;
	char			buf[256];

	gui_new_net_table = gp;

	if ((gui_new_net = (struct wifi_net *) malloc(sizeof(struct wifi_net))) == NULL) {
		char		buf[256];
		sprintf(buf, gettext("Out of memory."));
		gui_message(buf, MSG_ERROR);
		return;
	}
	memset(gui_new_net, 0, sizeof(*gui_new_net));

	gui_new_net->wn_enabled = 0;
	gui_new_net->wn_next = NULL;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 142);
	gtk_window_set_title(GTK_WINDOW(window), gettext("WiFi Networks Manager"));
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	g_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(gtk_widget_destroy),
	    NULL);

	vbox = gtk_vbox_new(FALSE, 5);

	table_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(table_scrolled_window), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(table_scrolled_window),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	table = gtk_table_new(1, 1, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 2);

	row = 0; col = 0;

	sprintf(buf, "<span size=\"large\"><b>%s</b></span>", gettext("Add Cloaked Network"));
	w = gtk_label_new(buf);
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

	w = gtk_label_new(gettext("SSID:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 5);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), gui_new_net->sup_ssid);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) gui_new_net->sup_ssid);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 5);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Security Type:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(w), gettext("Open"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(w), gettext("WEP"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(w), gettext("WPA PSK"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(w), gettext("RSN PSK"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(w), gettext("WPA EAP"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(w), gettext("RSN EAP"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(w), 0);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_security_type_selection),
	    (gpointer) gui_new_net);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(table_scrolled_window), table);
	gtk_box_pack_start(GTK_BOX(vbox), table_scrolled_window, TRUE, TRUE, 0);

	bottom_buttons = gtk_hbox_new(FALSE, 0);
	w = gtk_button_new_with_label(gettext("Done"));
	g_signal_connect(GTK_OBJECT(w), "clicked", GTK_SIGNAL_FUNC(gui_edit_new_network_done), 
	    (gpointer) window);
	gtk_box_pack_end(GTK_BOX(bottom_buttons), w, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), bottom_buttons, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show_all(window);
}

/*
** extended EAP fields editor
*/
static void
gui_edit_eap_done(GtkWidget * w, gpointer * gp) {

	gtk_widget_destroy((GtkWidget *) gp);
}

static void
gui_edit_eap(GtkWidget * x, gpointer * gp) {
	GtkWidget *		window;
	GtkWidget *		vbox;
	GtkWidget *		show_pass_box;
	GtkWidget *		table_scrolled_window;
	GtkWidget *		table;
	GtkWidget *		bottom_buttons;
	GtkWidget *		w;
	int			row;
	int			col;
	char			buf[256];
	int			i;
	int			j;
	struct wifi_net *	net;

	net = (struct wifi_net *) gp;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);
	gtk_window_set_title(GTK_WINDOW(window), gettext("WiFi Networks Manager"));
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	g_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(gtk_widget_destroy),
	    NULL);

	vbox = gtk_vbox_new(FALSE, 5);

	sprintf(buf, "<span size=\"large\"><b>%s</b></span>", gettext("Edit EAP Parameters"));
	w = gtk_label_new(buf);
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

	sprintf(buf, "<b>%s</b> %s (%s)", gettext("Network:"), net->sup_ssid, net->sup_bssid);
	w = gtk_label_new(buf);
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

	show_pass_box = gtk_hbox_new(FALSE, 0);

	w = gtk_check_button_new();
	if (net->wn_show_password)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	g_signal_connect(G_OBJECT(w), "toggled", G_CALLBACK(gui_process_check_button_showpass),
	    (gpointer) net);
	gtk_box_pack_end(GTK_BOX(show_pass_box), w, FALSE, FALSE, 5);

	w = gtk_image_new_from_file(ICON_PATH "/eye.png");
	gtk_box_pack_end(GTK_BOX(show_pass_box), w, FALSE, FALSE, 5);

	gtk_box_pack_start(GTK_BOX(vbox), show_pass_box, FALSE, FALSE, 0);

	table_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(table_scrolled_window), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(table_scrolled_window),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	table = gtk_table_new(1, 1, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 2);

	row = 0; col = 0;

	w = gtk_label_new(gettext("<b>EAP Parameter</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	w = gtk_label_new(gettext("<b>Value</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	row++; col=0;

	if (net->wn_km & WN_KM_PSK) {
		w = gtk_label_new(gettext("PSK:"));
		gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
		col++;

		w = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(w), net->wn_key);
		g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
		    (gpointer) net->wn_key);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
		col++;

		row++; col=0;
	}

	w = gtk_label_new(gettext("EAP:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(w), gettext("(Unset)"));
	j = 1;
	for (i = 0; conf_eap[i].cl_name; i++)
		if (conf_eap[i].cl_value > 0) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(w), conf_eap[i].cl_name);
			if (strstr(net->sup_eap, conf_eap[i].cl_name))
				gtk_combo_box_set_active(GTK_COMBO_BOX(w), j);
			j++;
		}
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_combo_box_selection),
	    (gpointer) net->sup_eap);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Pairwise:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(w), gettext("(Unset)"));
	j = 1;
	for (i = 0; conf_pairwise[i].cl_name; i++)
		if (conf_pairwise[i].cl_value > 0) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(w), conf_pairwise[i].cl_name);
			if (strstr(net->sup_pairwise, conf_pairwise[i].cl_name))
				gtk_combo_box_set_active(GTK_COMBO_BOX(w), j);
			j++;
		}
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_combo_box_selection),
	    (gpointer) net->sup_pairwise);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Group:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(w), gettext("(Unset)"));
	j = 1;
	for (i = 0; conf_group[i].cl_name; i++)
		if (conf_group[i].cl_value > 0) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(w), conf_group[i].cl_name);
			if (strstr(net->sup_group, conf_group[i].cl_name))
				gtk_combo_box_set_active(GTK_COMBO_BOX(w), j);
			j++;
		}
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_combo_box_selection),
	    (gpointer) net->sup_group);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Identity:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_identity);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_identity);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Anonymous Identity:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_anonymous_identity);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_anonymous_identity);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Mixed Cell:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), (gdouble) net->sup_mixed_cell);
	g_signal_connect(G_OBJECT(w), "value-changed", G_CALLBACK(gui_process_spin_button),
	    (gpointer) &net->sup_mixed_cell);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Password:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_password);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_password);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	gtk_entry_set_visibility(GTK_ENTRY(w), FALSE);
	net->w_sup_password = w;
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("CA Certificate:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_file_chooser_button_new(net->sup_ca_cert, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(w), gettext("CA Certificate:"));
	if (*net->sup_ca_cert)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), net->sup_ca_cert);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), ETCDIR);
	g_signal_connect(G_OBJECT(w), "selection-changed", G_CALLBACK(gui_process_file_selection),
	    (gpointer) net->sup_ca_cert);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Client Certificate:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_file_chooser_button_new(net->sup_client_cert, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(w), gettext("Client Certificate:"));
	if (*net->sup_client_cert)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), net->sup_client_cert);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), ETCDIR);
	g_signal_connect(G_OBJECT(w), "selection-changed", G_CALLBACK(gui_process_file_selection),
	    (gpointer) net->sup_client_cert);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Private Key:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_file_chooser_button_new(net->sup_private_key, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(w), gettext("Private Key:"));
	if (*net->sup_private_key)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), net->sup_private_key);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), ETCDIR);
	g_signal_connect(G_OBJECT(w), "selection-changed", G_CALLBACK(gui_process_file_selection),
	    (gpointer) net->sup_private_key);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Private Key Password:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_private_key_passwd);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_private_key_passwd);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	gtk_entry_set_visibility(GTK_ENTRY(w), FALSE);
	net->w_sup_private_key_passwd = w;
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("DH/DSA Parameters File:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_file_chooser_button_new(net->sup_dh_file, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(w), gettext("DH/DSA Parameters File:"));
	if (*net->sup_dh_file)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), net->sup_dh_file);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), ETCDIR);
	g_signal_connect(G_OBJECT(w), "selection-changed", G_CALLBACK(gui_process_file_selection),
	    (gpointer) net->sup_dh_file);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Subject Match:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_subject_match);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_subject_match);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Phase1 Parameters:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_phase1);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_phase1);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("CA Certificate 2:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_file_chooser_button_new(net->sup_ca_cert2, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(w), gettext("CA Certificate 2:"));
	if (*net->sup_ca_cert2)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), net->sup_ca_cert2);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), ETCDIR);
	g_signal_connect(G_OBJECT(w), "selection-changed", G_CALLBACK(gui_process_file_selection),
	    (gpointer) net->sup_ca_cert2);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Client Certificate 2:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_file_chooser_button_new(net->sup_client_cert2, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(w), gettext("Client Certificate 2:"));
	if (*net->sup_client_cert2)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), net->sup_client_cert2);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), ETCDIR);
	g_signal_connect(G_OBJECT(w), "selection-changed", G_CALLBACK(gui_process_file_selection),
	    (gpointer) net->sup_client_cert2);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Private Key 2:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_file_chooser_button_new(net->sup_private_key2, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(w), gettext("Private Key 2:"));
	if (*net->sup_private_key2)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), net->sup_private_key2);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), ETCDIR);
	g_signal_connect(G_OBJECT(w), "selection-changed", G_CALLBACK(gui_process_file_selection),
	    (gpointer) net->sup_private_key2);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Private Key 2 Password:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_private_key2_passwd);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_private_key2_passwd);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	gtk_entry_set_visibility(GTK_ENTRY(w), FALSE);
	net->w_sup_private_key2_passwd = w;
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("DH/DSA Parameters File 2:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_file_chooser_button_new(net->sup_dh_file2, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(w), gettext("DH/DSA Parameters File 2:"));
	if (*net->sup_dh_file2)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), net->sup_dh_file2);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), ETCDIR);
	g_signal_connect(G_OBJECT(w), "selection-changed", G_CALLBACK(gui_process_file_selection),
	    (gpointer) net->sup_dh_file2);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Subject Match 2:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_subject_match2);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_subject_match2);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Phase2 Parameters:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_phase2);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_phase2);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("EAP PSK (16-byte hex value):"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_eappsk);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_eappsk);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("User NAI for EAP PSK:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_nai);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_nai);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("Server NAI for EAP PSK:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(w), net->sup_server_nai);
	g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
	    (gpointer) net->sup_server_nai);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("PAC File:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_file_chooser_button_new(net->sup_pac_file, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_title(GTK_FILE_CHOOSER_BUTTON(w), gettext("PAC File:"));
	if (*net->sup_pac_file)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(w), net->sup_pac_file);
	else
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(w), ETCDIR);
	g_signal_connect(G_OBJECT(w), "selection-changed", G_CALLBACK(gui_process_file_selection),
	    (gpointer) net->sup_pac_file);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	w = gtk_label_new(gettext("EAP Workaround:"));
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	w = gtk_spin_button_new_with_range(0.0, 1.0, 1.0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), (gdouble) net->sup_eap_workaround);
	g_signal_connect(G_OBJECT(w), "value-changed", G_CALLBACK(gui_process_spin_button),
	    (gpointer) &net->sup_eap_workaround);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 5, 0);
	col++;

	row++; col=0;

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(table_scrolled_window), table);
	gtk_box_pack_start(GTK_BOX(vbox), table_scrolled_window, TRUE, TRUE, 0);

	bottom_buttons = gtk_hbox_new(FALSE, 0);
	w = gtk_button_new_with_label(gettext("Done"));
	g_signal_connect(GTK_OBJECT(w), "clicked", GTK_SIGNAL_FUNC(gui_edit_eap_done), 
	    (gpointer) window);
	gtk_box_pack_end(GTK_BOX(bottom_buttons), w, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), bottom_buttons, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_widget_show_all(window);
}

/*
** delete existing table data, if any
** fill table with network data
*/
static GtkWidget *
gui_fill_network_table(GtkWidget * x, gpointer * gp) {
	GtkWidget *		table;
	GtkWidget *		table_viewport = NULL;
	GtkWidget *		w;
	int			row;
	int			col;
	struct wifi_net *	net;
	char *			associated_net;
	char *			icon;
	char			buf[256];

	if (gp) {
		table = *((GtkWidget **) gp);
		table_viewport = table->parent;
		gtk_widget_destroy(table);
	}

	table = gtk_table_new(1, 1, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 2);

	row = 0;
	col = 0;

	w = gtk_label_new(gettext("<b>Enabled</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	w = gtk_label_new(gettext("<b>SSID</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	/* encryption */
	col++;

	w = gtk_label_new(gettext("<b>Encryption Key</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	w = gtk_image_new_from_file(ICON_PATH "/eye.png");
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	w = gtk_label_new(gettext("<b>Comment</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	w = gtk_label_new(gettext("<b>Priority</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	w = gtk_label_new(gettext("<b>BSSID</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_CENTER, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	w = gtk_label_new(gettext("<b>Any\nBSSID</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_CENTER, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	w = gtk_label_new(gettext("<b>Chan</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_RIGHT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	w = gtk_label_new(gettext("<b>Mbps</b>"));
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_misc_set_alignment(GTK_MISC(w), ALIGN_RIGHT, ALIGN_MIDDLE);
	gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
	    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
	col++;

	row++;
	col = 0;

	if (!nets)
		nets = build_network_list(wifi_if);

	if (wifi_if_status == WIFI_IF_UP)
		associated_net = ifconfig_associated_network(wifi_if);
	else
		associated_net = NULL;

	for (net = nets; net; net = net->wn_next) {
		col = 0;

		w = gtk_check_button_new();
		if (net->wn_enabled)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
		g_signal_connect(G_OBJECT(w), "toggled", G_CALLBACK(gui_process_check_button),
		    (gpointer) &(net->wn_enabled));
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		if (associated_net &&
			((net->wn_any_bssid && strcmp(net->sup_ssid, associated_net) == 0) ||
			(!net->wn_any_bssid && strcmp(net->sup_bssid, associated_net) == 0))) {
			w = gtk_image_new_from_file(ICON_PATH "/wifimgr24green.png");
			gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
			    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		}
		col++;

		w = gtk_label_new(net->sup_ssid);
		gtk_misc_set_alignment(GTK_MISC(w), ALIGN_LEFT, ALIGN_MIDDLE);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		col++;

		icon = NULL;
		if (net->wn_security & WN_SEC_RSN)
			icon = ICON_PATH "/padlock-rsn.png";
		else if (net->wn_security & WN_SEC_WPA)
			icon = ICON_PATH "/padlock-wpa.png";
		else if (net->wn_security & WN_SEC_WEP)
			icon = ICON_PATH "/padlock-wep.png";
		else if (net->wn_security & WN_SEC_NONE)
			icon = ICON_PATH "/padlock-open.png";
		else
			icon = ICON_PATH "/padlock-unk.png";
		if (icon) {
			w = gtk_image_new_from_file(icon);
			gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
			    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		}
		col++;

		/* if net has both EAP and PSK, gui_edit_eap() will allow editing of PSK field */
		if (net->wn_km & WN_KM_EAP) {
			w = gtk_button_new_with_label(gettext("Edit EAP Parameters"));
			g_signal_connect(GTK_OBJECT(w), "clicked", GTK_SIGNAL_FUNC(gui_edit_eap),
			    (gpointer) net);
			gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
			    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		}
		else if (net->wn_km & WN_KM_PSK || net->wn_security & WN_SEC_WEP) {
			w = gtk_entry_new();
			gtk_entry_set_text(GTK_ENTRY(w), net->wn_key);
			g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
			    (gpointer) net->wn_key);
			gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
			    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
			gtk_entry_set_visibility(GTK_ENTRY(w), FALSE);
			net->w_wn_key = w;
		}
		col++;

		if (net->wn_km & WN_KM_PSK || net->wn_security & WN_SEC_WEP) {
			w = gtk_check_button_new();
			if (net->wn_show_password)
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
			g_signal_connect(G_OBJECT(w), "toggled", G_CALLBACK(gui_process_check_button_showpass),
			    (gpointer) net);
			gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
			    GTK_EXPAND|GTK_FILL, GTK_FILL, 10, 0);
		}
		col++;

		w = gtk_entry_new();
		gtk_entry_set_text(GTK_ENTRY(w), net->wn_comment);
		g_signal_connect(G_OBJECT(w), "changed", G_CALLBACK(gui_process_text_input),
		    (gpointer) net->wn_comment);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		col++;

		w = gtk_spin_button_new_with_range(-1.0, 255.0, 1.0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), (gdouble) net->sup_priority);
		g_signal_connect(G_OBJECT(w), "value-changed", G_CALLBACK(gui_process_spin_button),
		    (gpointer) &net->sup_priority);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		col++;

		w = gtk_label_new(net->sup_bssid);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		col++;

		w = gtk_check_button_new();
		if (net->wn_any_bssid)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
		g_signal_connect(G_OBJECT(w), "toggled", G_CALLBACK(gui_process_check_button),
		    (gpointer) &net->wn_any_bssid);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 10, 0);
		col++;

		sprintf(buf, "%d", net->wn_chan);
		w = gtk_label_new(buf);
		gtk_misc_set_alignment(GTK_MISC(w), ALIGN_RIGHT, ALIGN_MIDDLE);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		col++;

		sprintf(buf, "%d", net->wn_rate);
		w = gtk_label_new(buf);
		gtk_misc_set_alignment(GTK_MISC(w), ALIGN_RIGHT, ALIGN_MIDDLE);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		col++;

		icon = NULL;
		if (net->wn_bars > 80)      icon = ICON_PATH "/signal_bars_5.png";
		else if (net->wn_bars > 60) icon = ICON_PATH "/signal_bars_4.png";
		else if (net->wn_bars > 40) icon = ICON_PATH "/signal_bars_3.png";
		else if (net->wn_bars > 20) icon = ICON_PATH "/signal_bars_2.png";
		else if (net->wn_bars > 0)  icon = ICON_PATH "/signal_bars_1.png";
		else			    icon = ICON_PATH "/signal_bars_0.png";
		w = gtk_image_new_from_file(icon);
		gtk_table_attach(GTK_TABLE(table), w, col, col+1, row, row+1,
		    GTK_EXPAND|GTK_FILL, GTK_FILL, 0, 0);
		col++;

		row++;
		col = 0;
	}

	if (gp) {
		gtk_container_add(GTK_CONTAINER(table_viewport), table);
		gtk_widget_show_all(table);
		*gp = table;
	}

	return table;
}

/*
** turn WiFi interface on or off
*/
static GtkWidget *
gui_interface_up_down(GtkWidget * x, gpointer * gp) {
	char *			icon;

	if (gui_changes && gui_message(gettext("Discard changes?"), MSG_QUESTION) != MSG_RESPONSE_YES)
		return NULL;

	gui_changes = 0;

	toggle_intf_up_down(wifi_if);

	/* change the main icon */
	icon = (wifi_if_status == WIFI_IF_UP) ? ICON_PATH "/wifimgr.png" : ICON_PATH "/wifimgr-grey.png";
	gtk_image_set_from_file(GTK_IMAGE(gui_up_down_icon), icon);

	/* zero existing network list */
	nl_delete_list(&nets);

	/* rescan */
	gui_fill_network_table(NULL, gp);

	return *gp;
}

/*
** restart interface and rescan
*/
static GtkWidget *
gui_rescan(GtkWidget * x, gpointer * gp) {
	char *			icon;

	if (gui_changes && gui_message(gettext("Discard changes?"), MSG_QUESTION) != MSG_RESPONSE_YES)
		return NULL;

	gui_changes = 0;

	/* restart interface */
	if (! restart_intf())
		return NULL;

	/* change the main icon */
	if (wifi_if_status == WIFI_IF_DOWN) {
		wifi_if_status = WIFI_IF_UP;
		icon = ICON_PATH "/wifimgr.png";
		gtk_image_set_from_file(GTK_IMAGE(gui_up_down_icon), icon);
	}

	/* zero existing network list */
	nl_delete_list(&nets);

	/* rescan */
	gui_fill_network_table(NULL, gp);

	return *gp;
}

/*
** save networks file, restart interface and rescan
*/
static GtkWidget *
gui_save_and_rescan(GtkWidget * x, gpointer * gp) {
	char *			icon;

	/* save updates */
	if (! save_networks_file(NETWORKS_FILE))
		return NULL;

	gui_changes = 0;

	/* restart interface */
	if (! restart_intf())
		return NULL;

	/* change the main icon */
	if (wifi_if_status == WIFI_IF_DOWN) {
		wifi_if_status = WIFI_IF_UP;
		icon = ICON_PATH "/wifimgr.png";
		gtk_image_set_from_file(GTK_IMAGE(gui_up_down_icon), icon);
	}

	/* zero existing network list */
	nl_delete_list(&nets);

	/* rescan */
	gui_fill_network_table(NULL, gp);

	return *gp;
}

/*
** user has clicked exit button
*/
static void
gui_exit(GtkWidget * w, gpointer * gp) {
	if (gui_changes && gui_message(gettext("Discard changes?"), MSG_QUESTION) != MSG_RESPONSE_YES)
		return;

	gtk_main_quit();
}

/*
** main GUI loop
*/
void
gui_loop() {
	GtkWidget *		window;
	GtkWidget *		main_vbox;
	GtkWidget *		top_graphic;
	GtkWidget *		table_scrolled_window;
	GtkWidget *		bottom_buttons;
	GtkWidget *		table;
	GtkWidget *		w;
	char *			icon;
	char			buf[256];

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 1000, 480);
	gtk_window_set_title(GTK_WINDOW(window), gettext("WiFi Networks Manager"));
	gtk_container_set_border_width(GTK_CONTAINER(window), 5);
	g_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

	main_vbox = gtk_vbox_new(FALSE, 5);

	top_graphic = gtk_hbox_new(FALSE, 0);
	w = gtk_image_new_from_file(ICON_PATH "/freebsd-surf.png");
	gtk_box_pack_start(GTK_BOX(top_graphic), w, FALSE, FALSE, 0);
	icon = (wifi_if_status == WIFI_IF_UP) ? ICON_PATH "/wifimgr.png" : ICON_PATH "/wifimgr-grey.png";
	gui_up_down_icon = gtk_image_new_from_file(icon);
	gtk_box_pack_end(GTK_BOX(top_graphic), gui_up_down_icon, FALSE, FALSE, 0);
	sprintf(buf, "<span size=\"x-large\"><b>%s</b></span>\n%s", gettext("WiFi Networks Manager"), VERSION);
	w = gtk_label_new(buf);
	gtk_label_set_use_markup(GTK_LABEL(w), TRUE);
	gtk_box_pack_end(GTK_BOX(top_graphic), w, FALSE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(main_vbox), top_graphic, FALSE, FALSE, 0);

	table_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(table_scrolled_window), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(table_scrolled_window),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	table = gui_fill_network_table(NULL, NULL);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(table_scrolled_window), table);
	gtk_box_pack_start(GTK_BOX(main_vbox), table_scrolled_window, TRUE, TRUE, 0);

	bottom_buttons = gtk_hbox_new(FALSE, 0);
	w = gtk_button_new_with_label(gettext("WiFi Up/Down"));
	gtk_button_set_image(GTK_BUTTON(w), gtk_image_new_from_file(ICON_PATH "/on-off.png"));
	g_signal_connect(GTK_OBJECT(w), "clicked", GTK_SIGNAL_FUNC(gui_interface_up_down), 
	    (gpointer) &table);
	gtk_box_pack_start(GTK_BOX(bottom_buttons), w, FALSE, FALSE, 5);
	w = gtk_button_new_with_label(gettext("Add Cloaked Network"));
	g_signal_connect(GTK_OBJECT(w), "clicked", GTK_SIGNAL_FUNC(gui_edit_new_network),
	    (gpointer) &table);
	gtk_box_pack_start(GTK_BOX(bottom_buttons), w, FALSE, FALSE, 5);
	w = gtk_button_new_with_label(gettext("Rescan Networks"));
	g_signal_connect(GTK_OBJECT(w), "clicked", GTK_SIGNAL_FUNC(gui_rescan),
	    (gpointer) &table);
	gtk_box_pack_start(GTK_BOX(bottom_buttons), w, FALSE, FALSE, 5);
	w = gtk_button_new_with_label(gettext("Close"));
	g_signal_connect(GTK_OBJECT(w), "clicked", GTK_SIGNAL_FUNC(gui_exit), NULL);
	gtk_box_pack_end(GTK_BOX(bottom_buttons), w, FALSE, FALSE, 5);
	w = gtk_button_new_with_label(gettext("Save and Reconnect"));
	g_signal_connect(GTK_OBJECT(w), "clicked", GTK_SIGNAL_FUNC(gui_save_and_rescan),
	    (gpointer) &table);
	gtk_box_pack_end(GTK_BOX(bottom_buttons), w, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(main_vbox), bottom_buttons, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(window), main_vbox);

	gtk_widget_show_all(window);

	gtk_main();
}
