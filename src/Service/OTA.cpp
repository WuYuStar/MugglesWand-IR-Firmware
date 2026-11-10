// System maintenance only. Models use /api/model on the normal device server.
#include <Arduino.h>
#include "OTA.h"
#include "Config.h"
#include "HAL/HAL.h"
#include "HAL/WS2812_Animation/AnimProgress.hpp"
#include "HAL/WS2812_Animation/AnimBreathe.hpp"
#include "Service/AP.h"
#include "Service/DualPrint.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <atomic>

static WebServer server(80);
static IPAddress ota_ip;
static uint8_t progressPct=0;
static bool receiving=false,complete=false,failed=false,bodyEnded=false,exiting=false;
static uint32_t restartAt=0;
static std::atomic<bool> started{false};
static const char UPDATE_PAGE[]=R"page(
<!doctype html>
<html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>更新魔杖系统</title>
<style>
*{box-sizing:border-box}[hidden]{display:none!important}
body{font:14px/1.55 system-ui,sans-serif;color:#25231e;background:#f5f3ef;max-width:620px;margin:24px auto;padding:0 16px}
header{border-bottom:1px solid #e5e0d7;padding-bottom:12px;font-weight:650}
h1{font-size:26px;line-height:1.25;margin:22px 0 16px}
section{background:#fffdfa;padding:16px;border:1px solid #e5e0d7;border-radius:12px}
label{display:block;font-weight:600;margin-bottom:6px}
button,input{font:inherit;color:inherit;max-width:100%;min-height:44px;padding:8px 12px;border:1px solid #b9b1a4;border-radius:8px;background:#fffdfa}
input{width:100%;overflow:hidden}button{background:#dbb76a;border-color:#dbb76a;margin-top:14px;font-weight:600;cursor:pointer}
button:disabled{background:#eeebe4;border-color:#e5e0d7;color:#625c51}
progress{display:block;width:100%;height:12px;accent-color:#795613;margin-top:12px}
p{color:#625c51;margin:10px 0 0}a{display:inline-flex;align-items:center;min-height:44px;color:#795613;text-underline-offset:3px;margin-top:8px}
:focus-visible{outline:2px solid #795613;outline-offset:3px}
</style>
<header>我的魔杖</header><h1>更新魔杖系统</h1>
<section>
<label for="file">选择应用固件 .bin</label><input id="file" type="file" accept=".bin">
<button id="send" hidden>上传并更新</button>
<progress id="progress" max="100" hidden aria-label="上传进度"></progress>
<p id="message" role="status">更新期间请保持魔杖供电。</p>
</section><a id="exit" href="/exit">退出升级并重启</a>
<script>
const $=id=>document.getElementById(id);
function fail(message){
 $('message').textContent=message;
 $('send').textContent='重试上传';
 $('send').disabled=$('file').disabled=$('exit').hidden=false;
}
$('file').onchange=()=>{
 const file=$('file').files[0];
 $('send').hidden=!file||!file.name.toLowerCase().endsWith('.bin');
 $('send').textContent='上传并更新';
 $('progress').hidden=true;
 $('message').textContent=file&&$('send').hidden?'请选择应用固件 .bin。':'更新期间请保持魔杖供电。';
};
$('send').onclick=async()=>{
 const file=$('file').files[0];
 if(!file||$('send').disabled)return;
 $('send').disabled=$('file').disabled=$('exit').hidden=true;
 $('progress').hidden=false;$('progress').value=0;
 $('message').textContent='准备更新…';
 try{
  const response=await fetch('/ota/start?mode=firmware');
  if(!response.ok)throw Error(await response.text());
  const body=new FormData();body.append('firmware',file);
  const upload=new XMLHttpRequest();
  upload.open('POST','/ota/upload');
  upload.upload.onprogress=event=>{
   if(!event.lengthComputable)return;
   $('progress').value=100*event.loaded/event.total;
   $('message').textContent=event.loaded===event.total?'正在校验固件…':'正在上传固件…';
  };
  upload.onload=()=>{
   if(upload.status!==200||upload.responseText.trim()!=='OK'){fail(upload.responseText||'更新失败，请重试。');return}
   $('progress').value=100;
   $('message').textContent='更新完成。请重连魔杖 Wi-Fi，再打开后台。';
  };
  upload.onerror=()=>fail('连接已断开。请重连魔杖后检查版本。');
  upload.send(body);
 }catch(error){fail(error.message)}
};
</script></html>
)page";

IPAddress OTA_get_ip(){return ota_ip;}
static void loopOTA(void*){for(;;){server.handleClient();if((complete||exiting)&&static_cast<int32_t>(millis()-restartAt)>=0)ESP.restart();vTaskDelay(10);}}
void OTA_begin(){
    if(started)return;started=true;
    if(ap_is_running())ap_toggle();if(espnow_is_running())espnow_toggle();
    WiFi.mode(WIFI_AP);WiFi.softAP(AP_SSID,AP_PASS);ota_ip=WiFi.softAPIP();
    auto page=[](){server.send(200,"text/html; charset=utf-8",UPDATE_PAGE);};server.on("/",HTTP_GET,page);server.on("/update",HTTP_GET,page);
    server.on("/exit",HTTP_GET,[](){
        if(receiving||complete){server.send(409,"text/plain; charset=utf-8","更新处理中，请等待完成");return;}
        if(Update.isRunning())Update.abort();
        server.send(200,"text/plain; charset=utf-8","正在退出升级，请重连魔杖热点");
        exiting=true;restartAt=millis()+1000;
    });
    server.on("/ota/start",HTTP_GET,[](){
        if(server.arg("mode")!="firmware"){server.send(400,"text/plain; charset=utf-8","仅支持应用固件；模型请使用独立模型接口");return;}
        if(complete||exiting){server.send(409,"text/plain","Reboot pending");return;}
        String hash=server.arg("hash");if(hash.length()&&hash.length()!=32){server.send(400,"text/plain","Invalid hash");return;}
        for(size_t i=0;i<hash.length();++i)if(!isxdigit(hash[i])){server.send(400,"text/plain","Invalid hash");return;}
        if(Update.isRunning())Update.abort();failed=false;receiving=false;bodyEnded=false;progressPct=0;
        if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH)){server.send(500,"text/plain; charset=utf-8","无法准备固件存储空间");return;}
        if(hash.length()&&!Update.setMD5(hash.c_str())){Update.abort();server.send(400,"text/plain","Invalid hash");return;}
        server.send(200,"text/plain","OK");
    });
    server.on("/ota/upload",HTTP_POST,[](){
        complete=receiving&&bodyEnded&&!failed&&Update.end(true);
        receiving=false;failed=!complete;
        if(failed&&Update.isRunning())Update.abort();
        const bool ok=complete;
        server.send(ok?200:400,"text/plain; charset=utf-8",ok?"OK":"固件校验失败，未启用新固件");
        if(ok)restartAt=millis()+2000;
    },[](){
        HTTPUpload &upload=server.upload();
        if(upload.status==UPLOAD_FILE_START){if(receiving){failed=true;return;}receiving=Update.isRunning();failed=!receiving;bodyEnded=false;progressPct=0;if(receiving)HAL::ws2812_set_overlay(new AnimProgress(progressPct));}
        else if(upload.status==UPLOAD_FILE_WRITE){
            if(!receiving||failed)return;
            if(Update.write(upload.buf,upload.currentSize)!=upload.currentSize)failed=true;
            size_t total=server.clientContentLength();progressPct=total?min(static_cast<size_t>(99),(upload.totalSize+upload.currentSize)*100/total):0;
        }else if(upload.status==UPLOAD_FILE_END){
            bodyEnded=receiving&&!failed;
        }else if(upload.status==UPLOAD_FILE_ABORTED){Update.abort();receiving=false;failed=true;complete=false;}
    });
    server.begin();HAL::ws2812_set_overlay(new AnimBreathe());xTaskCreatePinnedToCore(loopOTA,"SystemUpdate",4096,nullptr,4,nullptr,0);
    DualSerial.println("[OTA] Firmware-only maintenance at http://192.168.4.1/update");
}

bool OTA_active(){return started;}
