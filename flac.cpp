#include "flac.h"
#include <iostream>

namespace FLACStream {
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::FunctionTemplate;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::Persistent;
using v8::String;
using v8::Value;
using v8::HandleScope;
using v8::Context;

Persistent<Function> FLACNodeWrapper::Constructor;

FLACNodeWrapper::FLACNodeWrapper() : streamer(*this) {}

FLACNodeWrapper::~FLACNodeWrapper() {}

void FLACNodeWrapper::Init(Local<Object> exports) {
  Isolate * isolate = exports->GetIsolate();
  HandleScope scope(isolate);
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8(isolate, "FLACStream"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tpl, "_transform", _Transform);
  NODE_SET_PROTOTYPE_METHOD(tpl, "_flush", _Flush);
  Constructor.Reset(isolate, tpl->GetFunction());
  exports->Set(String::NewFromUtf8(isolate, "FLACStream"), tpl->GetFunction());
}

void FLACNodeWrapper::New(const FunctionCallbackInfo<Value> & args) {
  Isolate * isolate = args.GetIsolate();
  HandleScope scope(isolate);
  if (args.IsConstructCall()) {
    FLACNodeWrapper * obj = new FLACNodeWrapper();
    obj->Wrap(args.This());
    args.GetReturnValue().Set(args.This());
  } else {
    Local<Function> cons = Local<Function>::New(isolate, Constructor);
    args.GetReturnValue().Set(cons->NewInstance(0, {}));
  }
}

Local<Value> FLACNodeWrapper::doEmit(Local<Object> _this,
                                     Isolate * isolate,
                                     const char * event,
                                     Local<Value> obj) {
  auto emitFun =
      _this->GetRealNamedProperty(String::NewFromUtf8(isolate, "emit"));
  Local<Value> argv[] = {String::NewFromUtf8(isolate, event), obj};
  return emitFun->ToObject().As<Function>()->Call(_this, 2, argv);
}

void FLACNodeWrapper::_Transform(const FunctionCallbackInfo<Value> &) {
  std::cerr << "TRANSFORM" << std::endl;
}

void FLACNodeWrapper::_Flush(const FunctionCallbackInfo<Value> & args) {
  Isolate * isolate = args.GetIsolate();
  HandleScope scope(isolate);
  auto _this = args.Holder();
  args.GetReturnValue().Set(
      doEmit(_this, isolate, "bam", Number::New(isolate, 3)));
}

FLACStreamer::FLACStreamer(FLACNodeWrapper & arg)
    : FLAC::Decoder::Stream(), input(arg) {}

FLACStreamer::~FLACStreamer() {}

void FLACStreamer::metadata_callback(const FLAC__StreamMetadata * metadata) {
  /* std::cerr << "metadata callback called!" << std::endl; */
  if (FLAC__METADATA_TYPE_STREAMINFO == metadata->type) {
    /* std::cerr << "streaminfo found!" << std::endl; */

    sample_rate     = metadata->data.stream_info.sample_rate;
    channels        = metadata->data.stream_info.channels;
    bits_per_sample = metadata->data.stream_info.bits_per_sample;
  }
}

FLAC__StreamDecoderReadStatus FLACStreamer::read_callback(FLAC__byte * buffer,
                                                          size_t * nbytes) {
  if (nbytes and *nbytes > 0) {
    /* input.read(reinterpret_cast<char *>(buffer), *nbytes); */
    /* nbytes = input.gcount(); */
    /* if (input.fail()) { */
    /*   return FLAC__STREAM_DECODER_READ_STATUS_ABORT; */
    /* } else if (input.eof()) { */
    /*   return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM; */
    /* } else { */
    /*   return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE; */
    /* } */
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
  } else {
    return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
  }
}

FLAC__StreamDecoderWriteStatus
    FLACStreamer::write_callback(const FLAC__Frame * frame,
                                 const FLAC__int32 * const * buffer) {
  /* for (uint16_t block = 0; block < frame->header.blocksize; ++block) {
     for (uint16_t channel = 0; channel < channels; ++channel) {
     auto res = buffer[channel][block];
     if (!(fputc(res, stdout) != EOF and fputc(res >> 8, stdout) != EOF)) {
     std::perror("writing");
     std::cerr << "WRITE ERROR" << std::endl;
     return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
     }
     }
     } */
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACStreamer::error_callback(FLAC__StreamDecoderErrorStatus status) {
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
  /* std::cerr << msg << std::endl; */
}
}
