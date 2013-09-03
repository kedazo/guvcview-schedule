/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#           Nobuhiro Iwamatsu <iwamatsu@nigauri.org>                            #
#                             Add UYVY color support(Macbook iSight)            #
#           David Kedves <kedazo@gmail.com>                                     #
#                             Added schedule support                            #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/
#include <glib.h>
#include <glib/gprintf.h>
/* support for internationalization - i18n */
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "globals.h"
#include "callbacks.h"
#include "../config.h"

static gboolean
schedule_timer (gpointer data)
{
    struct ALL_DATA *all_data = (struct ALL_DATA*) data;
    struct GLOBAL *global = all_data->global;
    struct GWIDGET *gwidget = all_data->gwidget;
    char *message = NULL;

    if (global->schedTime <= 0) {
        // scheduling is disabled then
        return FALSE;
    }

    __LOCK_MUTEX(&all_data->videoIn->mutex);
    gboolean cap_Vid = all_data->videoIn->capVid;
    __UNLOCK_MUTEX(&all_data->videoIn->mutex);

    if (--global->schedTimeout <= 0) {
        if (!cap_Vid) {
            // start the capture
            capture_vid (NULL, all_data);
            // schedule the capture end timeout
            global->schedTimeout = global->Capture_time;
        } else {
            // shutdown the capture
            capture_vid (NULL, all_data);
            // lets schedule the next capture
            global->schedTimeout = (global->schedTime - global->Capture_time) + 1;
            if (global->schedTimeout <= 0) {
                global->schedTimeout = 1; // for safety
            }
        }
    }

    if (cap_Vid) {
        message = g_strdup_printf(_("scheduled capture in progress (left %d seconds)"),
                                  (global->schedTimeout));
    } else {
        message = g_strdup_printf(_("next scheduled capture in %d seconds"),
                                  (global->schedTimeout));
    }

    if(message && !global->no_display)
    {
        gtk_statusbar_pop (GTK_STATUSBAR(gwidget->status_bar), gwidget->status_warning_id);
        gtk_statusbar_push (GTK_STATUSBAR(gwidget->status_bar), gwidget->status_warning_id, message);
        g_free (message);
    }

    return TRUE;
}

/**
 * nextRecording: schedule a next recording event from now in seconds
 * videLength: the scheduled video recording length
 */
static void
schedule_start (
    struct ALL_DATA *all_data,
    int              nextRecording,
    int              videoLength)
{
    struct GLOBAL *global = all_data->global;
    global->schedTime = nextRecording;
    global->schedTimeout = nextRecording;
    global->Capture_time = videoLength;
    // destroy the previous timer if any
    if (global->sched_timer_id > 0) {
        g_source_remove (global->sched_timer_id);
        global->sched_timer_id = 0;
    }
    // lets start it is a schedule request
    if (global->schedTime > 0) {
        global->sched_timer_id =
            g_timeout_add_seconds (1, schedule_timer, all_data);
    }
}

static void
schedule_btn_toggled (
    GtkToggleButton *toggle,
    struct ALL_DATA *all_data)
{
    g_debug ("*** TOGGLED");
    struct GWIDGET *gwidget = all_data->gwidget;

    // 0 means disable the schedule, this is the default action
    int nextRecording = 0;

    int videoLength = gtk_spin_button_get_value (
        GTK_SPIN_BUTTON (gwidget->schedLengthIn));

    if (gtk_toggle_button_get_active (toggle)) {
        nextRecording = gtk_spin_button_get_value (
            GTK_SPIN_BUTTON (gwidget->schedTimeIn)) * 60; 
        g_debug ("*** NEXT REC: %d", nextRecording);
    }

    // enable&disable the time-editing widgets
    gtk_widget_set_sensitive (gwidget->schedTimeIn, (nextRecording <= 0));
    gtk_widget_set_sensitive (gwidget->schedLengthIn, (nextRecording <= 0));

    // and enable&disable the main controls too
    gtk_widget_set_sensitive (gwidget->quitButton, (nextRecording <= 0));
    gtk_widget_set_sensitive (gwidget->CapImageButt, (nextRecording <= 0));
    gtk_widget_set_sensitive (gwidget->CapVidButt, (nextRecording <= 0));

    schedule_start (all_data, nextRecording, videoLength);
}

void schedule_tab(struct ALL_DATA *all_data)
{
    struct GLOBAL *global = all_data->global;
    struct GWIDGET *gwidget = all_data->gwidget;

    GtkWidget *table4;
    GtkWidget *Tab4;
    GtkWidget *Tab4Label;
    GtkWidget *Tab4Icon;

    int line = 0;
    //TABLE
    table4 = gtk_grid_new();
    gtk_grid_set_column_homogeneous (GTK_GRID(table4), FALSE);
    gtk_widget_set_hexpand (table4, TRUE);
    gtk_widget_set_halign (table4, GTK_ALIGN_FILL);

    gtk_grid_set_row_spacing (GTK_GRID(table4), 4);
    gtk_grid_set_column_spacing (GTK_GRID (table4), 4);
    gtk_container_set_border_width (GTK_CONTAINER (table4), 2);
    gtk_widget_show (table4);

    //new grid for tab label and icon
    Tab4 = gtk_grid_new();
    Tab4Label = gtk_label_new(_("Schedule"));
    gtk_widget_show (Tab4Label);
    //check for files
    gchar* Tab4IconPath = g_strconcat (PACKAGE_DATA_DIR,"/pixmaps/guvcview/clock.png",NULL);
    //don't test for file - use default empty image if load fails
    //get icon image
    Tab4Icon = gtk_image_new_from_file(Tab4IconPath);
    g_free(Tab4IconPath);
    gtk_widget_show (Tab4Icon);
    gtk_grid_attach (GTK_GRID(Tab4), Tab4Icon, 0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID(Tab4), Tab4Label, 1, 0, 1, 1);
    gtk_widget_show (Tab4);

    //ADD TABLE to NOTEBOOK (TAB)
    gtk_notebook_append_page(GTK_NOTEBOOK(gwidget->boxh),table4,Tab4);
    //--------------------- scheduler controls --------------------------
    //enable scheduled recording
    line++;
    GtkWidget *schedRecBtn=gtk_check_button_new_with_label (_("Enable scheduled recording"));
    gtk_grid_attach(GTK_GRID(table4), schedRecBtn, 1, line, 1, 1);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(schedRecBtn), (global->schedTime > 0));
    gtk_widget_show (schedRecBtn);
    g_signal_connect (GTK_CHECK_BUTTON(schedRecBtn), "toggled",
               G_CALLBACK (schedule_btn_toggled), all_data);

    // Schedule timing
    line++;

    GtkWidget *label_SchedTime = gtk_label_new(_("Schedule time (mins):"));
    gtk_misc_set_alignment (GTK_MISC (label_SchedTime), 1, 0.5);

    gtk_grid_attach (GTK_GRID(table4), label_SchedTime, 0, line, 1, 1);
    gtk_widget_show (label_SchedTime);

    gwidget->schedTimeIn = gtk_spin_button_new_with_range (5, 3600, 5);
    gtk_spin_button_set_increments (
        GTK_SPIN_BUTTON (gwidget->schedTimeIn), 5, 30);
    // default is the minimum
    gtk_spin_button_set_value (
        GTK_SPIN_BUTTON (gwidget->schedTimeIn), 5);

    gtk_widget_set_halign (gwidget->schedTimeIn, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand (gwidget->schedTimeIn, TRUE);
    gtk_grid_attach(GTK_GRID(table4), gwidget->schedTimeIn, 1, line, 1, 1);
    gtk_widget_show (gwidget->schedTimeIn);

    // Scheduled recording length
    line++;

    GtkWidget *label_SchedRecLength = gtk_label_new(_("Scheduled cap. length (secs):"));
    gtk_misc_set_alignment (GTK_MISC (label_SchedRecLength), 1, 0.5);

    gtk_grid_attach (GTK_GRID(table4), label_SchedRecLength, 0, line, 1, 1);
    gtk_widget_show (label_SchedRecLength);

    gwidget->schedLengthIn = gtk_spin_button_new_with_range (10, 3600, 10);
    gtk_spin_button_set_increments (
        GTK_SPIN_BUTTON (gwidget->schedLengthIn), 10, 60);
    gtk_spin_button_set_value (
        GTK_SPIN_BUTTON (gwidget->schedLengthIn), global->Capture_time);

    gtk_widget_set_halign (gwidget->schedLengthIn, GTK_ALIGN_FILL);
    gtk_widget_set_hexpand (gwidget->schedLengthIn, TRUE);
    gtk_grid_attach(GTK_GRID(table4), gwidget->schedLengthIn, 1, line, 1, 1);
    gtk_widget_show (gwidget->schedLengthIn);

}
