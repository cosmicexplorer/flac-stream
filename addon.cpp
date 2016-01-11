#include "flac.h"

namespace FLACStream {
using v8::Local;
using v8::Object;

void InitAll(Local<Object> exports) { FLACNodeWrapper::Init(exports); }

NODE_MODULE(addon, InitAll);
}
