/* бррр скипиди доп доп доп йес йес */
#include <vector>

struct MP4_Sosite_Chlen {
  AVFormatContext* pFormatContext = NULL;
  const AVCodec *pCodec = NULL;
  AVCodecParameters *pCodecParameters = NULL;
  int video_stream_index = -1;
  AVCodecContext *pCodecContext = NULL;
  AVPacket *pPacket = NULL;
  int frame_counter = 0;
  double time_base;

  int width() const {
    return pCodecParameters->width;
  }

  int height() const {
    return pCodecParameters->height;
  }

  ~MP4_Sosite_Chlen() {
    avformat_close_input(&pFormatContext);
    av_packet_free(&pPacket);
    avcodec_free_context(&pCodecContext);
  }
};

struct Frame_Info {
  void* pData;
  double timestamp;
  double feature1;
  char type;
};

int load_mp4(MP4_Sosite_Chlen& hui, const char* path)
{
  hui.pFormatContext = avformat_alloc_context();

  if (avformat_open_input(&hui.pFormatContext, path, NULL, NULL) != 0) {
    printf("ERROR: could not open the file %s\n", path);
    return -1;
  }

  printf("format %s, duration %lld us, bit_rate %lld\n", hui.pFormatContext->iformat->name, hui.pFormatContext->duration, hui.pFormatContext->bit_rate);

  if (avformat_find_stream_info(hui.pFormatContext,  NULL) < 0) {
    printf("ERROR could not get the stream info\n");
    return -1;
  }

  printf("number of streams: %d\n", hui.pFormatContext->nb_streams);

  for (int i = 0; i < hui.pFormatContext->nb_streams; i++) {
    AVCodecParameters *pLocalCodecParameters =  NULL;
    pLocalCodecParameters = hui.pFormatContext->streams[i]->codecpar;
    printf("AVStream->time_base before open coded %d/%d\n", hui.pFormatContext->streams[i]->time_base.num, hui.pFormatContext->streams[i]->time_base.den);
    printf("AVStream->r_frame_rate before open coded %d/%d\n", hui.pFormatContext->streams[i]->r_frame_rate.num, hui.pFormatContext->streams[i]->r_frame_rate.den);
    printf("AVStream->start_time %d\n", hui.pFormatContext->streams[i]->start_time);
    printf("AVStream->duration %d\n", hui.pFormatContext->streams[i]->duration);

    printf("finding the proper decoder (CODEC)\n");

    const AVCodec *pLocalCodec = NULL;

    // finds the registered decoder for a codec ID
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
    pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

    if (pLocalCodec==NULL) {
      printf("ERROR unsupported codec!\n");
      // In this example if the codec is not found we just skip it
      continue;
    }

    // when the stream is a video we store its index, codec parameters and codec
    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (hui.video_stream_index == -1) {
	hui.video_stream_index = i;
	hui.pCodec = pLocalCodec;
	hui.pCodecParameters = pLocalCodecParameters;
      }

      printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
    } else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
      printf("Audio Codec: %d channels, sample rate %d\n", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
    }

    // print its name, id and bitrate
    printf("\tCodec %s ID %d bit_rate %lld\n", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
  }

  if (hui.video_stream_index == -1) {
    printf("File %s does not contain a video stream!\n", path);
    return -1;
  }

  // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
  AVCodecContext *pCodecContext = avcodec_alloc_context3(hui.pCodec);
  if (!pCodecContext)
    {
      printf("failed to allocated memory for AVCodecContext\n");
      return -1;
    }

  // Fill the codec context based on the values from the supplied codec parameters
  // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
  if (avcodec_parameters_to_context(pCodecContext, hui.pCodecParameters) < 0)
    {
      printf("failed to copy codec params to codec context\n");
      return -1;
    }

  // Initialize the AVCodecContext to use the given AVCodec.
  // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
  if (avcodec_open2(pCodecContext, hui.pCodec, NULL) < 0)
    {
      printf("failed to open codec through avcodec_open2\n");
      return -1;
    }

  hui.pPacket = av_packet_alloc();

  printf("time_base=%d/%d\n", hui.pFormatContext->streams[hui.video_stream_index]->time_base.den, hui.pFormatContext->streams[hui.video_stream_index]->time_base.num);
  hui.time_base = (double)hui.pFormatContext->streams[hui.video_stream_index]->time_base.den / hui.pFormatContext->streams[hui.video_stream_index]->time_base.num;

  return 0;
}

Frame_Info read_frame(MP4_Sosite_Chlen& hui)
{
  Frame_Info pizda = {};

  int r = av_read_frame(hui.pFormatContext, hui.pPacket);
  if (r < 0) {
    printf("ended video!\n");
    return pizda;
  }

  double n = hui.width()*hui.height();

  pizda.pData = hui.pPacket->data;
  pizda.feature1 = hui.pPacket->size/n;
  pizda.type = (hui.pPacket->flags & AV_PKT_FLAG_KEY) ? 'I' : 'P';
  pizda.timestamp = hui.pPacket->pts/hui.time_base;

  hui.frame_counter++;
  return pizda;
}

struct Ebany_Time_series {
  const char* name;
  std::vector<double> timestamps;
  std::vector<double> values;

  void add(double t, double y) {
    timestamps.push_back(t);
    values.push_back(y);
  }

  void plot() {
    ImPlot::PlotLine(name, timestamps.data(), values.data(), timestamps.size());
  }
};
