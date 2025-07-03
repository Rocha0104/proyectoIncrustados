DISPLAY=:0.0 gst-launch-1.0 -e \
    uridecodebin uri=file:///carros.mp4 name=srcbin \
    srcbin. ! queue ! nvvideoconvert ! 'video/x-raw(memory:NVMM),format=NV12' ! queue ! mux.sink_0 \
    nvstreammux name=mux batch-size=1 width=1920 height=1080 live-source=0 ! queue \
    ! nvinfer name=primary-infer unique-id=1 \
      config-file-path=/opt/nvidia/deepstream/deepstream-6.0/samples/configs/deepstream-app/config_infer_primary.txt ! queue \
    ! nvtracker tracker-width=640 tracker-height=368 \
      ll-lib-file=/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_nvmultiobjecttracker.so \
      ll-config-file=/opt/nvidia/deepstream/deepstream-6.0/samples/configs/deepstream-app/config_tracker_IOU.yml \
      enable-batch-process=1 ! queue \
    ! nvinfer name=secondary-infer unique-id=2 process-mode=2 \
      infer-on-gie-id=1 infer-on-class-ids="0:" batch-size=16 \
      config-file-path=/opt/nvidia/deepstream/deepstream-6.0/samples/configs/deepstream-app/config_infer_secondary_carmake.txt ! queue \
    ! nvvideoconvert ! nvdsosd process-mode=HW_MODE ! nvvideoconvert ! 'video/x-raw(memory:NVMM),format=NV12' \
    ! tee name=t \
    t. ! queue ! nvoverlaysink sync=false \
    t. ! queue ! nvv4l2h264enc bitrate=4000000 insert-sps-pps=1 iframeinterval=30 ! h264parse \
    ! tee name=e \
    e. ! queue ! mp4mux ! filesink location=output.mp4 \
    e. ! queue ! rtph264pay config-interval=1 pt=96 ! udpsink host=192.168.1.149 port=8001 sync=false async=false

