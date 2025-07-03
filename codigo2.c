/* ds_pipeline.c — selecciona uno de dos pipelines con argv[1] (1|2)      */
#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>

static int
tutorial_main (int argc, char *argv[])
{
    if (argc < 2) {
        g_printerr ("Uso:\n"
          "  %s 1 <uri_mp4> <ip_dest>\n"
          "  %s 2 <ip_dest> <archivo_mp4>\n", argv[0], argv[0]);
        return -1;
    }

    int choice = atoi (argv[1]);

    /*------------- plantillas --------------------------------------*/
    const char tpl1[] =   /* archivo → IA → display + grab + UDP RTP */
      "uridecodebin uri=%s name=srcbin "
      "srcbin. ! queue ! nvvideoconvert ! "
      "video/x-raw(memory:NVMM),format=NV12,block-linear=false ! "
      "queue ! mux.sink_0 "
      "nvstreammux name=mux batch-size=1 width=1920 height=1080 "
      "live-source=0 ! queue ! "
      "nvinfer name=primary-infer unique-id=1 "
      "config-file-path=/opt/nvidia/deepstream/deepstream-6.0/samples/"
      "configs/deepstream-app/config_infer_primary.txt ! queue ! "
      "nvtracker tracker-width=640 tracker-height=368 "
      "ll-lib-file=/opt/nvidia/deepstream/deepstream-6.0/lib/"
      "libnvds_nvmultiobjecttracker.so "
      "ll-config-file=/opt/nvidia/deepstream/deepstream-6.0/samples/"
      "configs/deepstream-app/config_tracker_IOU.yml "
      "enable-batch-process=1 ! queue ! "
      "nvinfer name=secondary-infer unique-id=2 process-mode=2 "
      "infer-on-gie-id=1 infer-on-class-ids=\"0:\" batch-size=16 "
      "config-file-path=/opt/nvidia/deepstream/deepstream-6.0/samples/"
      "configs/deepstream-app/config_infer_secondary_carmake.txt ! queue ! "
      "nvvideoconvert ! nvdsosd process-mode=HW_MODE ! nvvideoconvert ! "
      "video/x-raw(memory:NVMM),format=NV12 ! tee name=t "
      "t. ! queue ! nvoverlaysink sync=false "
      "t. ! queue ! nvv4l2h264enc bitrate=4000000 insert-sps-pps=1 "
      "iframeinterval=30 ! h264parse ! tee name=e "
      "e. ! queue ! mp4mux ! filesink location=output.mp4 "
      "e. ! queue ! rtph264pay config-interval=1 pt=96 ! "
      "udpsink host=%s port=8001 sync=false async=false";

    const char tpl2[] =   /* cámara CSI → IA → display + grab + UDP RTP */
      "nvarguscamerasrc num-buffers=500 sensor-id=0 bufapi-version=1 ! "
      "video/x-raw(memory:NVMM),width=1920,height=1080,framerate=30/1,"
      "format=NV12 ! nvvidconv flip-method=0 ! "
      "video/x-raw(memory:NVMM),format=NV12 ! queue ! mux.sink_0 "
      "nvstreammux name=mux batch-size=1 width=1920 height=1080 "
      "live-source=true ! queue ! "
      "video/x-raw(memory:NVMM),format=NV12,width=1920,height=1080 ! "
      "nvinfer name=primary-infer unique-id=1 batch-size=1 "
      "config-file-path=/opt/nvidia/deepstream/deepstream-6.0/samples/"
      "configs/deepstream-app/config_infer_primary.txt ! queue ! "
      "nvtracker tracker-width=640 tracker-height=368 "
      "ll-lib-file=/opt/nvidia/deepstream/deepstream-6.0/lib/"
      "libnvds_nvmultiobjecttracker.so "
      "ll-config-file=/opt/nvidia/deepstream/deepstream-6.0/samples/"
      "configs/deepstream-app/config_tracker_IOU.yml "
      "enable-batch-process=1 ! queue ! "
      "nvinfer name=secondary-infer unique-id=2 process-mode=2 "
      "infer-on-gie-id=1 infer-on-class-ids=\"0:\" batch-size=16 "
      "config-file-path=/opt/nvidia/deepstream/deepstream-6.0/samples/"
      "configs/deepstream-app/config_infer_secondary_carmake.txt ! queue ! "
      "nvvideoconvert ! nvdsosd process-mode=HW_MODE ! nvvideoconvert ! "
      "video/x-raw(memory:NVMM),format=NV12 ! tee name=dis "
      "dis. ! queue ! nvoverlaysink sync=false "
      "dis. ! queue ! nvv4l2h264enc insert-sps-pps=true bitrate=4000000 "
      "iframeinterval=30 ! h264parse ! tee name=ex "
      "ex. ! queue ! mp4mux ! filesink location=%s async=false "
      "ex. ! queue ! rtph264pay config-interval=1 pt=96 ! "
      "udpsink host=%s port=8001 sync=false async=false";

    char desc[8192];

    switch (choice) {
    case 1:
        if (argc != 4) {
            g_printerr ("Modo 1 requiere: <uri_mp4> <ip_dest>\n");
            return -1;
        }
        snprintf (desc, sizeof (desc), tpl1, argv[2], argv[3]);
        break;

    case 2:
        if (argc != 4) {
            g_printerr ("Modo 2 requiere: <ip_dest> <archivo_mp4>\n");
            return -1;
        }
        snprintf (desc, sizeof (desc), tpl2, argv[3], argv[2]);
        break;

    default:
        g_printerr ("Primer argumento debe ser 1 o 2\n");
        return -1;
    }

    /*------------ GStreamer ----------------------------------------*/
    gst_init (&argc, &argv);
    GError *err = NULL;
    GstElement *pipeline = gst_parse_launch (desc, &err);
    if (!pipeline || err) {
        g_printerr ("gst_parse_launch error: %s\n",
                    err ? err->message : "desconocido");
        if (err) g_error_free (err);
        return -1;
    }

    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_print ("Pipeline %d corriendo…\n", choice);

    GstBus *bus = gst_element_get_bus (pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered (
        bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR) {
        GError *gerr; gchar *dbg;
        gst_message_parse_error (msg, &gerr, &dbg);
        g_printerr ("Error: %s\n", gerr->message);
        g_error_free (gerr); g_free (dbg);
    }

    gst_message_unref (msg);
    gst_object_unref (bus);
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
    return 0;
}

int
main (int argc, char *argv[])
{
#if defined(__APPLE__) && TARGET_OS_MAC && !TARGET_OS_IPHONE
    return gst_macos_main ((GstMainFunc) tutorial_main, argc, argv, NULL);
#else
    return tutorial_main (argc, argv);
#endif
}

