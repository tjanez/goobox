/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  Goo
 *
 *  Copyright (C) 2004 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include "dlg-extract.h"
#include "dlg-ripper.h"
#include "goo-player.h"
#include "gtk-utils.h"
#include "track-info.h"
#include "typedefs.h"


#define GET_WIDGET(x) _gtk_builder_get_widget (data->builder, (x))


typedef struct {
	GooWindow   *window;
	GooPlayer   *player;
	GList       *tracks;
	GList       *selected_tracks;
	GtkBuilder  *builder;
	GtkWidget   *dialog;
} DialogData;


static void
dialog_destroy_cb (GtkWidget  *widget,
		   DialogData *data)
{
	track_list_free (data->selected_tracks);
	track_list_free (data->tracks);
	g_object_unref (data->player);
	g_object_unref (data->builder);
	g_free (data);
}


static void
ok_button_clicked_cb (GtkWidget  *widget,
		      DialogData *data)
{
	GList *tracks_to_rip;

	gtk_window_set_modal (GTK_WINDOW (data->dialog), FALSE);
	gtk_widget_hide (data->dialog);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("selected_tracks_radiobutton"))))
		tracks_to_rip = track_list_dup (data->selected_tracks);
	else
		tracks_to_rip = track_list_dup (goo_player_get_album (data->player)->tracks);

	dlg_ripper (data->window, tracks_to_rip);

	track_list_free (tracks_to_rip);
	gtk_widget_destroy (data->dialog);
}


void
dlg_extract_ask (GooWindow *window)
{
	GstElement *encoder;
	gboolean    ogg_encoder;
	gboolean    flac_encoder;
	gboolean    wave_encoder;
	DialogData *data;
	int         selected;

	encoder = gst_element_factory_make (OGG_ENCODER, "encoder");
	ogg_encoder = encoder != NULL;
	if (encoder != NULL)
		gst_object_unref (GST_OBJECT (encoder));

	encoder = gst_element_factory_make (FLAC_ENCODER, "encoder");
	flac_encoder = encoder != NULL;
	if (encoder != NULL)
		gst_object_unref (GST_OBJECT (encoder));

	encoder = gst_element_factory_make (WAVE_ENCODER, "encoder");
	wave_encoder = encoder != NULL;
	if (encoder != NULL)
		gst_object_unref (GST_OBJECT (encoder));

	if (! ogg_encoder && ! flac_encoder && ! wave_encoder) {
		GtkWidget *d;
		char      *msg;

		msg = g_strdup_printf (_("You need at least one of the following GStreamer plugins in order to extract CD tracks:\n\n"
					 "\342\200\242 %s \342\206\222 Ogg Vorbis\n"
					 "\342\200\242 %s \342\206\222 FLAC\n"
					 "\342\200\242 %s \342\206\222 Waveform PCM"),
				       OGG_ENCODER,
				       FLAC_ENCODER,
				       WAVE_ENCODER);

		d = _gtk_message_dialog_new (GTK_WINDOW (window),
					     GTK_DIALOG_MODAL,
					     _GTK_ICON_NAME_DIALOG_ERROR,
					     _("No encoder available."),
					     msg,
					     _GTK_LABEL_OK, GTK_RESPONSE_OK,
					     NULL);
		g_free (msg);

		g_signal_connect (G_OBJECT (d), "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);
		gtk_widget_show (d);

		return;
	}

	/**/

	data = g_new0 (DialogData, 1);
	data->window = window;
	data->builder = _gtk_builder_new_from_resource ("extract.ui");
	data->tracks = goo_window_get_tracks (window, FALSE);
	data->selected_tracks = goo_window_get_tracks (window, TRUE);
	data->player = g_object_ref (goo_window_get_player (window));

	/* Get the widgets. */

	data->dialog = GET_WIDGET ("extract_dialog");

	/* Set widgets data. */

	selected = g_list_length (data->selected_tracks);
	gtk_widget_set_sensitive (GET_WIDGET ("selected_tracks_radiobutton"), selected > 0);
	if (selected <= 1)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("all_tracks_radiobutton")), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (GET_WIDGET ("selected_tracks_radiobutton")), TRUE);

	/* Set the signals handlers. */

	g_signal_connect (data->dialog,
			  "destroy",
			  G_CALLBACK (dialog_destroy_cb),
			  data);
	g_signal_connect_swapped (GET_WIDGET ("cancel_button"),
				  "clicked",
				  G_CALLBACK (gtk_widget_destroy),
				  G_OBJECT (data->dialog));
	g_signal_connect (GET_WIDGET ("ok_button"),
			  "clicked",
			  G_CALLBACK (ok_button_clicked_cb),
			  data);

	/* run dialog. */

	gtk_window_set_transient_for (GTK_WINDOW (data->dialog), GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (data->dialog), TRUE);
	gtk_widget_show (data->dialog);
}


void
dlg_extract_selected (GooWindow *window)
{
	GList *tracks_to_rip;

	tracks_to_rip = goo_window_get_tracks (window, TRUE);
	dlg_ripper (window, tracks_to_rip);

	track_list_free (tracks_to_rip);
}


void
dlg_extract (GooWindow *window)
{
	GList *selected_tracks;

	selected_tracks = goo_window_get_tracks (window, TRUE);
	if (g_list_length (selected_tracks) < 1)
		dlg_ripper (window, NULL);
	else
		dlg_extract_ask (window);

	track_list_free (selected_tracks);
}
