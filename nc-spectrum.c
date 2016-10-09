#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <curses.h>
#include <stdlib.h>

#define TIME_OUT  20

void display_spectrum(WINDOW *win, int xpos, int ypos, float valores[]);
void display_spectrum_bar(WINDOW *win, int x, int h);

static guint spect_bands = 20;

#define AUDIOFREQ 32000

/* receive spectral data from element message */
static gboolean
message_handler (GstBus * bus, GstMessage * message, gpointer data)
{
    if (message->type == GST_MESSAGE_ELEMENT) {
        const GstStructure *s = gst_message_get_structure (message);
        const gchar *name = gst_structure_get_name (s);
        GstClockTime endtime;

        if (strcmp (name, "spectrum") == 0) {
            const GValue *magnitudes;
            const GValue *phases;
            const GValue *mag, *phase;
            gdouble freq;
            guint i;

            if (!gst_structure_get_clock_time (s, "endtime", &endtime))
                endtime = GST_CLOCK_TIME_NONE;

            /*g_print ("New spectrum message, endtime %" GST_TIME_FORMAT "\n",*/
            /*GST_TIME_ARGS (endtime));*/

            magnitudes = gst_structure_get_value (s, "magnitude");
            phases = gst_structure_get_value (s, "phase");

            float valor = 0.0;
            float valores[spect_bands];
            for (i = 0; i < spect_bands; ++i) {
                freq = (gdouble) ((AUDIOFREQ / 2) * i + AUDIOFREQ / 4) / spect_bands;
                mag = gst_value_list_get_value (magnitudes, i);
                phase = gst_value_list_get_value (phases, i);

                if (mag != NULL && phase != NULL) {
                    /*g_print ("band %d (freq %g): magnitude %f dB phase %f\n", i, freq,*/
                    /*g_value_get_float (mag), g_value_get_float (phase));*/
                    g_print ("%2.0f;", 80.0 + g_value_get_float (mag));

                    /*if(g_value_get_float(mag) < valor) {*/
                        /*valor = g_value_get_float(mag);*/
                    /*}*/
                    valores[i] = (80.0 + g_value_get_float(mag)) / 4;
                }
            }
            g_print ("\n");

            // ncurses
            /*display_spectrum(stdscr, 10, 10, (int)((80 + valor)));*/
            display_spectrum(stdscr, 10, 10, valores);// (int)((80 + valor)));
        }
    }

    return TRUE;
}

static void
on_pad_added (GstElement *element,
        GstPad     *pad,
        gpointer    data)
{
    GstPad *sinkpad;
    GstElement *decoder = (GstElement *) data;

    /* We can now link this pad with the vorbis-decoder sink pad */
    g_print ("Dynamic pad created, linking demuxer/decoder\n");

    sinkpad = gst_element_get_static_pad (decoder, "sink");

    gst_pad_link (pad, sinkpad);

    gst_object_unref (sinkpad);
}

void display_spectrum(WINDOW *win, int xpos, int ypos, float valores[])
{
    int x;
    int h = 5;
    wclear(win);
    for(x = 0; x <= 20; ++x) {
        display_spectrum_bar(win, x, (int)valores[x]);
    }
    wrefresh(win);
}

void display_spectrum_bar(WINDOW *win, int x, int h)
{
    int y;
    for(y = 0; y <= h; ++y) {
        mvwaddch(win, 20 - y, x, ' '|A_REVERSE);
    }
}

///////////////////////////////////////////////////////////////////////////////

    int
main (int argc, char *argv[])
{
    GstElement *bin;
    GstElement *src, *audioconvert, *spectrum, *sink;
    GstElement *demuxer, *decoder;
    GstBus *bus;
    GstCaps *caps;
    GMainLoop *loop;

    gst_init (&argc, &argv);

    bin = gst_pipeline_new ("bin");

    /*src = gst_element_factory_make ("audiotestsrc", "src");*/
    /*g_object_set (G_OBJECT (src), "wave", 0, "freq", 6000.0, NULL);*/
    /*audioconvert = gst_element_factory_make ("audioconvert", NULL);*/
    /*g_assert (audioconvert);*/

    src = gst_element_factory_make("filesrc", "file-source");
    g_object_set(G_OBJECT(src), "location", "/home/edu/proyectos/nc-spectrum/vocal2.ogg",  NULL);

    audioconvert = gst_element_factory_make ("audioconvert", "converter");
    g_assert (audioconvert);

    demuxer  = gst_element_factory_make ("oggdemux",      "ogg-demuxer");
    decoder  = gst_element_factory_make ("vorbisdec",     "vorbis-decoder");

    spectrum = gst_element_factory_make ("spectrum", "spectrum");
    g_object_set (G_OBJECT (spectrum), "bands", spect_bands, "threshold", -80,
            "post-messages", TRUE, "message-phase", TRUE, NULL);

    /*sink = gst_element_factory_make ("fakesink", "sink");*/
    sink = gst_element_factory_make ("autoaudiosink", "audio-output");


    gst_bin_add_many (GST_BIN (bin), src, demuxer, decoder, audioconvert, spectrum, sink, NULL);

    if (!src || !demuxer || !decoder || !audioconvert || !sink) {
        g_printerr ("One element could not be created. Exiting.\n");
        return -1;
    }

    caps = gst_caps_new_simple ("audio/x-raw",
            "rate", G_TYPE_INT, AUDIOFREQ, NULL);

    /*if (!gst_element_link (src, audioconvert) ||*/
    /*!gst_element_link_filtered (audioconvert, spectrum, caps) ||*/
    /*!gst_element_link (spectrum, sink)) {*/
    /*fprintf (stderr, "can't link elements\n");*/
    /*exit (1);*/
    /*}*/


    if(!gst_element_link (src, demuxer)) {
        /*if (!gst_element_link (src, audioconvert)) {*/
        fprintf (stderr, "can't link elements 1\n");
        exit (1);
    }
    if(!gst_element_link (decoder, audioconvert)) {
        /*if (!gst_element_link (src, audioconvert)) {*/
        fprintf (stderr, "can't link elements 1c\n");
        exit (1);
    }
    /*if (!gst_element_link_filtered (audioconvert, spectrum, caps)) {*/
    /*fprintf (stderr, "can't link elements 2\n");*/
    /*exit (1);*/
    /*}*/
    if (!gst_element_link (audioconvert, spectrum)) {
        fprintf (stderr, "can't link elements 2\n");
        exit (1);
    }
    if (!gst_element_link (spectrum, sink)) {
        fprintf (stderr, "can't link elements 3\n");
        exit (1);
    }
    g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), decoder);

    gst_caps_unref (caps);

    bus = gst_element_get_bus (bin);
    gst_bus_add_watch (bus, message_handler, NULL);
    gst_object_unref (bus);

    gst_element_set_state (bin, GST_STATE_PLAYING);


    /////////////////////////////////////////////////////////////////////////////
    // ncurses
    //
    initscr();
    cbreak();
    timeout(TIME_OUT);
    keypad(stdscr, TRUE);
    curs_set(0);

    /////////////////////////////////////////////////////////////////////////////

    printf("main loop...\n");
    /* we need to run a GLib main loop to get the messages */
    loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (loop);

    printf("post main loop...\n");

    gst_element_set_state (bin, GST_STATE_NULL);

    gst_object_unref (bin);

    // ncurses
    endwin();

    return 0;
}
