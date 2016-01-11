// -*- mode: c++; -*-
#ifndef __FLAC_H__
#define __FLAC_H__

#include <node.h>
#include <node_object_wrap.h>
#include "deps/flac/include/FLAC++/decoder.h"

static_assert(sizeof(char) == sizeof(FLAC__byte), "invalid char size");

namespace FLACStream {
class FLACNodeWrapper;
class FLACStreamer : public FLAC::Decoder::Stream {
private:
  FLACNodeWrapper & input;
  uint16_t sample_rate;
  uint16_t channels;
  uint16_t bits_per_sample;

public:
  ~FLACStreamer();
  FLACStreamer(FLACNodeWrapper &);

  uint16_t getSampleRate() { return sample_rate; }
  uint16_t getChannels() { return channels; }
  uint16_t getBitsPerSample() { return bits_per_sample; }

  virtual void metadata_callback(const FLAC__StreamMetadata *);
  virtual FLAC__StreamDecoderReadStatus read_callback(FLAC__byte *, size_t *);
  virtual FLAC__StreamDecoderWriteStatus
      write_callback(const FLAC__Frame *, const FLAC__int32 * const *);
  virtual void error_callback(FLAC__StreamDecoderErrorStatus);
};

class FLACNodeWrapper : public node::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object>);

private:
  FLACStreamer streamer;

  explicit FLACNodeWrapper();
  ~FLACNodeWrapper();
  static v8::Local<v8::Value> doEmit(v8::Local<v8::Object>,
                                     v8::Isolate *,
                                     const char *,
                                     v8::Local<v8::Value>);
  static void New(const v8::FunctionCallbackInfo<v8::Value> &);
  static void _Transform(const v8::FunctionCallbackInfo<v8::Value> &);
  static void _Flush(const v8::FunctionCallbackInfo<v8::Value> &);
  static v8::Persistent<v8::Function> Constructor;
};
}

#endif /* __FLAC_H__ */
