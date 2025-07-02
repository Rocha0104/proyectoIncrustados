DISPLAY=:0.0 \
gst-launch-1.0 -e \
  nvarguscamerasrc sensor-id=0 bufapi-version=1 ! \
    'video/x-raw(memory:NVMM),width=1920,height=1080,framerate=30/1,format=NV12' ! \
  nvvidconv flip-method=0 ! \
    'video/x-raw(memory:NVMM),format=NV12' ! queue ! mux.sink_0 \
  nvstreammux name=mux batch-size=1 width=1920 height=1080 live-source=1 ! \
  queue ! nvinfer \
    config-file-path=/opt/nvidia/deepstream/deepstream-6.0/\
samples/configs/deepstream-app/config_infer_primary.txt ! \
  queue ! nvvideoconvert ! nvdsosd process-mode=HW_MODE ! \
  nvvideoconvert ! \
    'video/x-raw(memory:NVMM),format=NV12' ! tee name=t \
  t. ! queue ! nvoverlaysink sync=false \
  t. ! queue ! nvv4l2h264enc bitrate=4000000 \
        insert-sps-pps=1 iframeinterval=30 ! \
      h264parse ! tee name=e \
  e. ! queue ! mp4mux ! filesink location=output.mp4 \
  e. ! queue ! rtph264pay config-interval=1 pt=96 ! \
        udpsink host=192.168.1.149 port=8001 \
                sync=false async=false

