// -*- mode: c++; -*-
#ifndef __FLAC_H__
#define __FLAC_H__

#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <string>

#include "deps/gRin/gRin.hpp"

#include <node.h>
#include <node_object_wrap.h>
#include "deps/flac/include/FLAC++/decoder.h"

namespace FLACStream {
class finished_decoding_event : public std::exception {};

class FLACStreamer : public FLAC::Decoder::Stream, public node::ObjectWrap {
private:
  unsigned int sample_rate;
  unsigned int channels;
  unsigned int bits_per_sample;

  std::atomic<bool> metadata_has_been_read;
  bool metadata_sent;
  gRin::ring_queue<FLAC__byte> input_queue;
  gRin::ring_queue<FLAC__byte> output_queue;

  std::atomic<bool> is_done_processing_read;
  std::atomic<bool> is_done_processing_write;
  std::thread decoder_thread;
  std::mutex in_queue_lock;
  std::mutex out_queue_lock;
  std::condition_variable in_queue_cv;
  std::condition_variable out_queue_cv;

  /* tells thread to cancel immediately */
  std::atomic<bool> do_quit;

  /* used if an error occurs somewhere */
  std::atomic<bool> is_err;
  std::string errstr;

  static void do_decode(FLACStreamer *);
  static void
      emit_metadata(FLACStreamer *, v8::Local<v8::Object>, v8::Isolate *);
  static void emit_error(FLACStreamer *,
                         v8::Local<v8::Object>,
                         v8::Isolate *,
                         const char *);
  static void push_in(FLACStreamer *, FLAC__byte *, size_t);
  static void
      push_single(FLACStreamer *, v8::Local<v8::Object>, v8::Isolate *, size_t);
  static void push_out(FLACStreamer *,
                       v8::Local<v8::Object>,
                       v8::Isolate *,
                       v8::Local<v8::Function>);
  static void push_out_until_end(FLACStreamer *,
                                 v8::Local<v8::Object>,
                                 v8::Isolate *,
                                 v8::Local<v8::Function>);

  static v8::Persistent<v8::Function> super_call;
  static void SetSuper(const v8::FunctionCallbackInfo<v8::Value> &);

public:
  /* maximum length of buffer this will ever push */
  static const size_t MAX_PUSH_LENGTH;

  static void Init(v8::Local<v8::Object>);

  virtual ~FLACStreamer();
  explicit FLACStreamer();
  FLACStreamer(const FLACStreamer &) = delete;

  static void New(const v8::FunctionCallbackInfo<v8::Value> &);
  static void _Transform(const v8::FunctionCallbackInfo<v8::Value> &);
  static void _Flush(const v8::FunctionCallbackInfo<v8::Value> &);
  static v8::Persistent<v8::Function> Constructor;

  virtual void metadata_callback(const FLAC__StreamMetadata *);
  virtual FLAC__StreamDecoderReadStatus read_callback(FLAC__byte *, size_t *);
  virtual FLAC__StreamDecoderWriteStatus
      write_callback(const FLAC__Frame *, const FLAC__int32 * const *);
  virtual void error_callback(FLAC__StreamDecoderErrorStatus);
};
}

#endif /* __FLAC_H__ */
