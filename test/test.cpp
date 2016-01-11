#include <iostream>
#include <fstream>
#include <string>
#include "../deps/flac/include/FLAC++/decoder.h"

class FLACStreamerFromFile : public FLAC::Decoder::Stream {
private:
  std::fstream & input;

public:
  ~FLACStreamerFromFile();
  FLACStreamerFromFile(std::fstream & arg)
      : FLAC::Decoder::Stream(), input(arg) {}
  virtual FLAC__StreamDecoderReadStatus read_callback(FLAC__byte *, size_t *);
  virtual FLAC__StreamDecoderWriteStatus
      write_callback(const FLAC__Frame *, const FLAC__int32 * const *);
  virtual void error_callback(FLAC__StreamDecoderErrorStatus);
};

FLACStreamerFromFile::~FLACStreamerFromFile() {
  input.close();
}
FLAC__StreamDecoderReadStatus
    FLACStreamerFromFile::read_callback(FLAC__byte * buffer, size_t * nbytes) {}
FLAC__StreamDecoderWriteStatus
    FLACStreamerFromFile::write_callback(const FLAC__Frame *,
                                         const FLAC__int32 * const * buffer) {}
void FLACStreamerFromFile::error_callback(
    FLAC__StreamDecoderErrorStatus status) {}

const char * flac_filename = "/home/cosmicexplorer/Music/CHVRCHES - Every Open "
                             "Eye [Deluxe] (2015) [FLAC]/01 - Never Ending "
                             "Circles.flac";

int main() {
  std::cout << "hey" << std::endl;
}
