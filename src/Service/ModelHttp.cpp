#include "ModelHttp.h"
#include "AP_Web.h"
#include "Config.h"
#include "Model/ModelStore.h"
#include "HAL/HAL.h"
#include <ESPAsyncWebServer.h>
#include <esp_heap_caps.h>
#include <string.h>

namespace {
struct Upload {uint8_t *data;size_t size,received;uint32_t ticket;int error;bool reserved;};
String jsonString(const char *p){String s="\"";for(;*p;++p){if(*p=='"'||*p=='\\')s+='\\';s+=*p;}return s+'"';}
void reply(AsyncWebServerRequest *request,int code,const String &json){auto *response=request->beginResponse(code,"application/json; charset=utf-8",json);response->addHeader("Cache-Control","no-store");request->send(response);}
void release(AsyncWebServerRequest *request){auto *ctx=static_cast<Upload*>(request->_tempObject);if(!ctx)return;if(ctx->reserved)ModelStore::cancel(request);if(ctx->data)heap_caps_free(ctx->data);free(ctx);request->_tempObject=nullptr;}
String info(){auto job=ModelStore::status();String s="{\"firmware\":\"" FIRMWARE_VER "\",\"protocol\":1,\"contract\":1,\"max_classes\":16,\"max_model_bytes\":"+String(ModelStore::MaxModelBytes);
    s+=",\"storage_ready\":";s+=job.storage?"true":"false";s+=",\"updating\":";s+=job.busy?"true":"false";
    s+=",\"active_hash\":"+jsonString(ModelStore::active_hash())+",\"source\":"+jsonString(ModelStore::from_storage()?"stored":"builtin")+",\"gestures\":[";
    for(int i=0;i<gesture_model_count();++i){if(i)s+=',';s+=jsonString(gesture_model_label(i));}
    s+="],\"job\":{\"ticket\":"+String(job.ticket)+",\"phase\":"+jsonString(job.phase)+",\"percent\":"+String(job.percent)+",\"message\":"+jsonString(job.message)+",\"next_hash\":"+jsonString(job.nextHash)+"}}";return s;
}
}
void model_http_routes(AsyncWebServer &server){
    server.on("/api/model",HTTP_GET,[](AsyncWebServerRequest *request){reply(request,200,info());}).setFilter(ap_web_local);
    server.on("/api/model",HTTP_POST,[](AsyncWebServerRequest *request){
        auto *ctx=static_cast<Upload*>(request->_tempObject);
        if(!ctx){reply(request,400,"{\"error\":\"请选择完整的手势模型包\"}");return;}
        if(ctx->error||ctx->received!=ctx->size){int code=ctx->error?ctx->error:400;release(request);reply(request,code,code==409?"{\"error\":\"请先返回手势识别，并等待其他更新任务结束\"}":"{\"error\":\"模型包不完整、过大或暂时无法接收，请重试\"}");return;}
        uint32_t ticket=ctx->ticket;
        if(!ModelStore::submit(request,ctx->data,ctx->size)){release(request);reply(request,503,"{\"error\":\"更新任务无法启动，原模型未改动\"}");return;}
        ctx->data=nullptr;ctx->reserved=false;release(request);reply(request,202,"{\"accepted\":true,\"ticket\":"+String(ticket)+"}");
    },nullptr,[](AsyncWebServerRequest *request,uint8_t *data,size_t len,size_t index,size_t total){
        if(index==0){
            request->client()->setRxTimeout(15);
            auto *ctx=static_cast<Upload*>(calloc(1,sizeof(Upload)));if(!ctx)return;request->_tempObject=ctx;ctx->size=total;
            request->onDisconnect([request](){release(request);});
            if(request->header("X-Wand-Model")!="1")ctx->error=403;
            else if(total<sizeof(ModelPackageHeader)+40||total>ModelStore::MaxPackageBytes)ctx->error=413;
            else if(HAL::ICM42670P_getDataMux()!=2)ctx->error=409;
            else if(!ModelStore::reserve(request,ctx->ticket))ctx->error=409;
            else {ctx->reserved=true;ctx->data=ModelStore::allocate(total);if(!ctx->data)ctx->error=503;}
        }
        auto *ctx=static_cast<Upload*>(request->_tempObject);if(!ctx||ctx->error)return;
        if(index!=ctx->received||len>ctx->size-ctx->received){ctx->error=400;return;}
        memcpy(ctx->data+index,data,len);ctx->received+=len;
    }).setFilter(ap_web_local);
}
