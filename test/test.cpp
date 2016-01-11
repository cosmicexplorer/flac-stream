#include <iostream>
#include <fstream>
#include <string>
#include "../deps/flac/include/FLAC++/decoder.h"

class FLACStreamerFromFile : public FLAC::Decoder::Stream {
private:
  std::ifstream & input;
  uint16_t sample_rate;
  uint16_t channels;
  uint16_t bits_per_sample;

public:
  ~FLACStreamerFromFile();
  FLACStreamerFromFile(std::ifstream & arg)
      : FLAC::Decoder::Stream(), input(arg) {}

  uint16_t getSampleRate() { return sample_rate; }
  uint16_t getChannels() { return channels; }
  uint16_t getBitsPerSample() { return bits_per_sample; }

  virtual void metadata_callback(const FLAC__StreamMetadata *);
  virtual FLAC__StreamDecoderReadStatus read_callback(FLAC__byte *, size_t *);
  virtual FLAC__StreamDecoderWriteStatus
      write_callback(const FLAC__Frame *, const FLAC__int32 * const *);
  virtual void error_callback(FLAC__StreamDecoderErrorStatus);
};

FLACStreamerFromFile::~FLACStreamerFromFile() { input.close(); }
void FLACStreamerFromFile::metadata_callback(
    const FLAC__StreamMetadata * metadata) {
  std::cerr << "metadata callback called!" << std::endl;
  if (FLAC__METADATA_TYPE_STREAMINFO == metadata->type) {
    std::cerr << "streaminfo found!" << std::endl;
    sample_rate     = metadata->data.stream_info.sample_rate;
    channels        = metadata->data.stream_info.channels;
    bits_per_sample = metadata->data.stream_info.bits_per_sample;
  }
}

static_assert(sizeof(char) == sizeof(FLAC__byte), "invalid char size");

FLAC__StreamDecoderReadStatus
    FLACStreamerFromFile::read_callback(FLAC__byte * buffer, size_t * nbytes) {
  if (nbytes and *nbytes > 0) {
    /* FIXME: cast to signed char ok? */
    input.read(reinterpret_cast<char *>(buffer), *nbytes);
    *nbytes = input.gcount();
    if (!input) {
      return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    } else if (input.eof()) {
      return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    } else {
      return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
  } else {
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
  }
}

FLAC__StreamDecoderWriteStatus
    FLACStreamerFromFile::write_callback(const FLAC__Frame * frame,
                                         const FLAC__int32 * const * buffer) {
  for (uint16_t block = 0; block < frame->header.blocksize; ++block) {
    for (uint16_t channel = 0; channel < channels; ++channel) {
      auto res = buffer[channel][block];
      if (!(fputc(res, stdout) != EOF and fputc(res >> 8, stdout) != EOF)) {
        std::perror("writing");
        std::cerr << "WRITE ERROR" << std::endl;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
      }
    }
  }
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACStreamerFromFile::error_callback(
    FLAC__StreamDecoderErrorStatus status) {
  std::string msg;
  switch (status) {
  case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
    msg = "BAD HEADER";
    break;
  case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
    msg = "LOST SYNC";
    break;
  case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
    msg = "FRAME CRC MISMATCH";
    break;
  case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM:
    msg = "UNPARSEABLE STREAM";
    break;
  default:
    msg = "ERROR UNKNOWN";
    break;
  }
  std::cerr << msg << std::endl;
}

const char * flac_filename = "/home/cosmicexplorer/Music/CHVRCHES - Every Open "
                             "Eye [Deluxe] (2015) [FLAC]/01 - Never Ending "
                             "Circles.flac";

int main() {
  std::ifstream stream(flac_filename, std::ifstream::binary);
  FLACStreamerFromFile streamer(stream);
  auto initStatus = streamer.init();
  if (FLAC__STREAM_DECODER_INIT_STATUS_OK != initStatus) {
    std::cerr << "ERROR INITIALIZING" << std::endl;
  } else {
    if (!streamer.process_until_end_of_stream()) {
      std::cerr << "FAILED PROCESSING" << std::endl;
    } else {
      std::cerr << "SUCCEEDED PROCESSING" << std::endl;
    }
  }
}
