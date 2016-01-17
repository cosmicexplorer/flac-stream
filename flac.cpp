#include <cstring>
#include <algorithm>
#include <node_buffer.h>
#include "flac.h"

/* TODO: remove this! */
#include <iostream>

static_assert(sizeof(char) == sizeof(FLAC__byte), "invalid char size");
static_assert(sizeof(FLAC__byte) == 1, "invalid byte size");

namespace FLACStream {
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::MaybeLocal;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;
using v8::HandleScope;
using v8::Context;
using v8::Exception;

Persistent<Function> FLACStreamer::Constructor;

const size_t FLACStreamer::MAX_PUSH_LENGTH = 10e4;

Persistent<Function> FLACStreamer::super_call;

FLACStreamer::FLACStreamer()
    : FLAC::Decoder::Stream(), metadata_has_been_read(false),
      metadata_sent(false), is_done_processing_read(false),
      is_done_processing_write(false), in_queue_cv(), out_queue_cv(),
      do_quit(false), is_err(false) {
  decoder_thread = std::thread(do_decode, this);
}

FLACStreamer::~FLACStreamer() {
  do_quit.store(true);
  decoder_thread.join();
}

void FLACStreamer::do_decode(FLACStreamer * _this) {
  try {
    auto init_status = _this->init();
    if (FLAC__STREAM_DECODER_INIT_STATUS_OK != init_status) {
      throw std::runtime_error(
          FLAC__StreamDecoderErrorStatusString[init_status]);
    }
    _this->process_until_end_of_stream();
  } catch (finished_decoding_event &) {
  } catch (const std::exception & ex) {
    _this->errstr = ex.what();
    _this->is_err.store(true);
  } catch (...) {
    _this->errstr = "Unknown stream processing error";
    _this->is_err.store(true);
  }
  _this->is_done_processing_write.store(true);
  _this->out_queue_cv.notify_one();
}

void FLACStreamer::SetSuper(const FunctionCallbackInfo<Value> & args) {
  Isolate * isolate = args.GetIsolate();
  HandleScope scope(isolate);
  Local<Function> super(args[0]->ToObject().As<Function>());
  super_call.Reset(isolate, super);

  Local<FunctionTemplate> class_template = FunctionTemplate::New(isolate, New);
  class_template->SetClassName(String::NewFromUtf8(isolate, "FLACStream"));
  class_template->InstanceTemplate()->SetInternalFieldCount(17);
  NODE_SET_PROTOTYPE_METHOD(class_template, "_transform", _Transform);
  NODE_SET_PROTOTYPE_METHOD(class_template, "_flush", _Flush);
  Constructor.Reset(isolate, class_template->GetFunction());

  args.GetReturnValue().Set(class_template->GetFunction());
}

void FLACStreamer::Init(Local<Object> exports) {
  Isolate * isolate = exports->GetIsolate();
  HandleScope scope(isolate);

  Local<FunctionTemplate> setup_template =
      FunctionTemplate::New(isolate, SetSuper);
  Local<Function> output = setup_template->GetFunction();
  Local<String> name = String::NewFromUtf8(isolate, "FLACStreamerCreator");
  output->SetName(name);
  exports->Set(name, output);
}

void FLACStreamer::New(const FunctionCallbackInfo<Value> & args) {
  Isolate * isolate = args.GetIsolate();
  HandleScope scope(isolate);
  if (args.IsConstructCall()) {
    FLACStreamer * obj = new FLACStreamer();
    obj->Wrap(args.This());
    auto opts           = args[0];
    Local<Value> argv[] = {opts};
    Local<Function>::New(isolate, super_call)->Call(args.This(), 1, argv);
    args.GetReturnValue().Set(args.This());
  } else {
    Local<Function> cons = Local<Function>::New(isolate, Constructor);
    args.GetReturnValue().Set(cons->NewInstance(0, {}));
  }
}

void FLACStreamer::emit_metadata(FLACStreamer * _this,
                                 Local<Object> _thisObj,
                                 Isolate * isolate) {
  auto emitFun = _thisObj->Get(String::NewFromUtf8(isolate, "emit"))
                     ->ToObject()
                     .As<Function>();
  auto metadataObj = Object::New(isolate);
  metadataObj->Set(String::NewFromUtf8(isolate, "sample_rate"),
                   Number::New(isolate, _this->sample_rate));
  metadataObj->Set(String::NewFromUtf8(isolate, "channels"),
                   Number::New(isolate, _this->channels));
  metadataObj->Set(String::NewFromUtf8(isolate, "bits_per_sample"),
                   Number::New(isolate, _this->bits_per_sample));
  Local<Value> argv[] = {String::NewFromUtf8(isolate, "metadata"), metadataObj};
  emitFun->Call(_thisObj, 2, argv);
}

void FLACStreamer::emit_error(FLACStreamer * _this,
                              Local<Object> _thisObj,
                              Isolate * isolate,
                              const char * errstr) {
  auto emitFun = _thisObj->Get(String::NewFromUtf8(isolate, "emit"))
                     ->ToObject()
                     .As<Function>();
  Local<Value> argv[] = {
      String::NewFromUtf8(isolate, "error"),
      Exception::Error(String::NewFromUtf8(isolate, errstr))};
  emitFun->Call(_thisObj, 2, argv);
}

void FLACStreamer::push_in(FLACStreamer * _this,
                           FLAC__byte * data,
                           size_t len) {
  std::unique_lock<std::mutex> in_guard(_this->in_queue_lock);
  std::cerr << "input_queue size() before insert: " << _this->input_queue.size()
            << std::endl;
  _this->input_queue.push_range(data, len);
  std::cerr << "input_queue size() after insert: " << _this->input_queue.size()
            << std::endl;
  _this->in_queue_cv.notify_one();
}

/* MUST have lock while doing this */
void FLACStreamer::push_single(FLACStreamer * _this,
                               Local<Object> _thisObj,
                               Isolate * isolate,
                               size_t len) {
  size_t num_to_write               = std::min(len, MAX_PUSH_LENGTH);
  MaybeLocal<Object> maybe_push_buf = node::Buffer::New(isolate, num_to_write);
  Local<Object> push_buf;
  if (!maybe_push_buf.ToLocal(&push_buf)) {
    throw std::runtime_error("buffer could not be allocated");
  }
  size_t num_written = _this->output_queue.pull_range(
      node::Buffer::Data(push_buf), num_to_write);
  if (num_to_write != num_written) {
    throw std::runtime_error(
        "number of bytes written to output buffer does not match expected");
  }
  auto pushFun = _thisObj->Get(String::NewFromUtf8(isolate, "push"))
                     ->ToObject()
                     .As<Function>();
  Local<Value> argv[] = {push_buf};
  pushFun->Call(_thisObj, 1, argv);
}

void FLACStreamer::push_out(FLACStreamer * _this,
                            Local<Object> _thisObj,
                            Isolate * isolate,
                            Local<Function> cb) {
  size_t out_queue_len;
  std::unique_lock<std::mutex> out_guard(_this->out_queue_lock);
  while ((out_queue_len = _this->output_queue.size()) > 0) {
    push_single(_this, _thisObj, isolate, out_queue_len);
  }
  cb->Call(_thisObj, 0, {});
}

void FLACStreamer::push_out_until_end(FLACStreamer * _this,
                                      Local<Object> _thisObj,
                                      Isolate * isolate,
                                      Local<Function> cb) {
  size_t out_queue_len;
  std::unique_lock<std::mutex> cond_lock(_this->out_queue_lock);
  while (!_this->is_done_processing_write.load()) {
    _this->out_queue_cv.wait(cond_lock);
  }
  while ((out_queue_len = _this->output_queue.size()) > 0) {
    push_single(_this, _thisObj, isolate, out_queue_len);
  }
  cb->Call(_thisObj, 0, {});
}

void FLACStreamer::_Transform(const FunctionCallbackInfo<Value> & args) {
  Isolate * isolate(args.GetIsolate());
  HandleScope scope(isolate);
  Local<Object> _thisObj(args.Holder());
  FLACStreamer * _this(ObjectWrap::Unwrap<FLACStreamer>(_thisObj));
  Local<Value> chunk(args[0]);
  Local<Function> cb(args[2]->ToObject().As<Function>());

  size_t len        = node::Buffer::Length(chunk);
  FLAC__byte * data = (FLAC__byte *) node::Buffer::Data(chunk);
  std::cerr << "len: " << len << std::endl;

  if (!_this->metadata_sent and _this->metadata_has_been_read.load()) {
    _this->emit_metadata(_this, _thisObj, isolate);
    _this->metadata_sent = true;
  }

  if (_this->is_err.load()) {
    emit_error(_this, _thisObj, isolate, _this->errstr.c_str());
  }

  push_in(_this, data, len);
  push_out(_this, _thisObj, isolate, cb);
}

void FLACStreamer::_Flush(const FunctionCallbackInfo<Value> & args) {
  Isolate * isolate(args.GetIsolate());
  HandleScope scope(isolate);
  Local<Object> _thisObj(args.Holder());
  FLACStreamer * _this(ObjectWrap::Unwrap<FLACStreamer>(_thisObj));
  Local<Function> cb(args[0]->ToObject().As<Function>());

  if (_this->is_err.load()) {
    emit_error(_this, _thisObj, isolate, _this->errstr.c_str());
  }

  std::unique_lock<std::mutex> cond_lock(_this->in_queue_lock);
  _this->is_done_processing_read.store(true);
  _this->in_queue_cv.notify_one();
  cond_lock.unlock();

  push_out_until_end(_this, _thisObj, isolate, cb);
}

void FLACStreamer::metadata_callback(const FLAC__StreamMetadata * metadata) {
  if (metadata and (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)) {
    sample_rate     = metadata->data.stream_info.sample_rate;
    channels        = metadata->data.stream_info.channels;
    bits_per_sample = metadata->data.stream_info.bits_per_sample;
    /* we don't need to use a mutex here, but only because the default memory
       order of this store is std::memory_order_seq_cst, so it happens after
       the above */
    metadata_has_been_read.store(true);
  }
}

FLAC__StreamDecoderReadStatus FLACStreamer::read_callback(FLAC__byte * buffer,
                                                          size_t * nbytes) {
  if (do_quit.load()) { throw finished_decoding_event(); }
  /* consider returning abort on exception (with catch(...)) */
  if (buffer and nbytes and (*nbytes > 0)) {
    std::unique_lock<std::mutex> cond_lock(in_queue_lock);
    bool done_proc;
    while (input_queue.empty() and
           !(done_proc = is_done_processing_read.load())) {
      in_queue_cv.wait(cond_lock);
    }
    std::cerr << "input_queue size() before: " << input_queue.size()
              << std::endl;
    *nbytes = input_queue.pull_range(buffer, *nbytes);
    std::cerr << "nbytes: " << *nbytes << std::endl;
    std::cerr << "input_queue size() after: " << input_queue.size()
              << std::endl;
    if (done_proc) {
      std::cerr << "here?" << std::endl;
      return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    } else {
      return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
  } else {
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
  }
}

FLAC__StreamDecoderWriteStatus
    FLACStreamer::write_callback(const FLAC__Frame * frame,
                                 const FLAC__int32 * const * buffer) {
  /* consider returning abort on exception (with catch(...)) */
  if (frame and buffer) {
    /* need to know number of channels to write */
    if (!metadata_has_been_read.load()) {
      throw std::runtime_error(
          "metadata not read before write_callback called");
    }
    std::unique_lock<std::mutex> out_guard(out_queue_lock);
    size_t blocksize         = frame->header.blocksize;
    size_t num_bytes_to_push = bits_per_sample * 8;
    std::cerr << "blocksize: " << blocksize << std::endl;
    /* FIXME: much slower than it needs to be because you have to interleave
       samples from each channel, so we can't just use memcpy */
    for (uint16_t block = 0; block < blocksize; ++block) {
      for (uint16_t channel = 0; channel < channels; ++channel) {
        output_queue.push_range(buffer[channel] + block, num_bytes_to_push);
      }
    }
  }
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACStreamer::error_callback(FLAC__StreamDecoderErrorStatus status) {
  throw std::runtime_error(FLAC__StreamDecoderErrorStatusString[status]);
}
}
