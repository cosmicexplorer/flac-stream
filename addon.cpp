#include "flac.h"

using v8::Local;
using v8::Object;

void InitAll(Local<Object> exports) { FLACStream::FLACStreamer::Init(exports); }

NODE_MODULE(addon, InitAll);
