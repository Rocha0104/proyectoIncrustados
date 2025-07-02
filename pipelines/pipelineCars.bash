DISPLAY=:0.0 GST_DEBUG=2 gst-launch-1.0 filesrc location=/opt/nvidia/deepstream/deepstream/samples/streams/sample_1080p_h264.mp4 ! qtdemux ! h264parse ! nvv4l2decoder ! queue ! mux.sink_0 nvstreammux name=mux width=1920 height=1080 batch-size=1 ! queue ! nvvideoconvert ! nvinfer config-file-path=/opt/nvidia/deepstream/deepstream-6.0/samples/configs/deepstream-app/config_infer_primary.txt model-engine-file=/opt/nvidia/deepstream/deepstream-6.0/samples/models/Primary_Detector/resnet10.caffemodel_b30_gpu0_fp16.engine ! queue ! nvdsosd process-mode=HW_MODE ! nvoverlaysink sync=false


