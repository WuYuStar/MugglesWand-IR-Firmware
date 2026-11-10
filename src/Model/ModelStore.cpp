#include "ModelStore.h"
#include "GestureProfile.h"
#include "BuiltinModelProfile.h"
#include "gesture_model.h"
#include <Arduino.h>
#include <esp_partition.h>
#include <esp_heap_caps.h>
#include <mbedtls/sha256.h>
#include <math.h>
#include <string.h>

namespace {
constexpr size_t BankBytes=0x10000;
struct BankHeader {char magic[8];uint32_t sequence,size;uint8_t digest[32];uint32_t checksum;uint8_t reserved[8];uint32_t commit;};
static_assert(sizeof(BankHeader)==64,"Commit header");
const esp_partition_t *partition=nullptr;
ModelPackageHeader profile={};
uint8_t *activePayload=nullptr;
const uint8_t *activeData=nullptr;
size_t activeSize=0;
char activeHash[65]={},profileKey[16]={},profileTitle[64]={},fallbackKeys[14][16]={};
int fallbackSizes[14]={},fallbackCount=0,activeBank=-1;
uint32_t activeSequence=0;
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
ModelStore::Status job={};
void *receivingOwner=nullptr;
uint8_t *workPayload=nullptr;size_t workSize=0;
void hex(const uint8_t *data,char *out){for(int i=0;i<32;++i)sprintf(out+2*i,"%02x",data[i]);out[64]=0;}
void digest(const void *data,size_t size,uint8_t out[32]){mbedtls_sha256_ret(static_cast<const uint8_t*>(data),size,out,0);}
void digestParts(const void *a,size_t an,const void *b,size_t bn,uint8_t out[32]){mbedtls_sha256_context c;mbedtls_sha256_init(&c);mbedtls_sha256_starts_ret(&c,0);mbedtls_sha256_update_ret(&c,static_cast<const uint8_t*>(a),an);mbedtls_sha256_update_ret(&c,static_cast<const uint8_t*>(b),bn);mbedtls_sha256_finish_ret(&c,out);mbedtls_sha256_free(&c);}
void keyFor(int count,char *out){
    bool builtin=count==kGestureCount;
    for(int i=0;builtin&&i<count;++i)builtin=strcmp(profile.labels[i],kGestureLabels[i])==0;
    if(builtin){strlcpy(out,kGestureProfileKey,16);return;}
    const char *defaults[]={"a","e","n","o","p","s","8","∞"};bool letters=count==8;
    for(int i=0;letters&&i<8;++i)letters=strcmp(profile.labels[i],defaults[i])==0;
    if(letters){strlcpy(out,"letters-v1",16);return;}
    String json="[";
    for(int i=0;i<count;++i){if(i)json+=", ";json+='"';for(const char *p=profile.labels[i];*p;++p){if(*p=='"'||*p=='\\')json+='\\';json+=*p;}json+='"';}json+=']';
    uint8_t sha[32];digest(json.c_str(),json.length(),sha);char full[65];hex(sha,full);snprintf(out,16,"g%.13s",full);
}
void catalog(){keyFor(profile.classes,profileKey);fallbackCount=0;for(int n=profile.classes-1;n>=2;--n){keyFor(n,fallbackKeys[fallbackCount]);fallbackSizes[fallbackCount++]=n;}snprintf(profileTitle,sizeof(profileTitle),"%s · %u个手势",activePayload?"独立模型":"内置模型",profile.classes);}
bool labelValid(const char *p){
    const char *end=static_cast<const char*>(memchr(p,0,64));if(!end||end==p)return false;
    for(const unsigned char *c=reinterpret_cast<const unsigned char*>(p);*c;){
        if(*c<32)return false;if(*c<128){++c;continue;}
        int n=*c>=0xc2&&*c<=0xdf?1:*c>=0xe0&&*c<=0xef?2:*c>=0xf0&&*c<=0xf4?3:-1;
        if(n<0||c+n>=reinterpret_cast<const unsigned char*>(end))return false;
        if((*c==0xe0&&c[1]<0xa0)||(*c==0xed&&c[1]>=0xa0)||(*c==0xf0&&c[1]<0x90)||(*c==0xf4&&c[1]>=0x90))return false;
        ++c;while(n--){if((*c&0xc0)!=0x80)return false;++c;}
    }return true;
}
void phase(const char *name,int percent,const char *message){portENTER_CRITICAL(&mux);strlcpy(job.phase,name,sizeof(job.phase));strlcpy(job.message,message,sizeof(job.message));job.percent=percent;portEXIT_CRITICAL(&mux);}
uint32_t bankChecksum(const BankHeader &h){uint32_t value=2166136261u;auto p=reinterpret_cast<const uint8_t*>(&h);for(size_t i=0;i<48;++i)value=(value^p[i])*16777619u;return value;}
bool readBankHeader(int index,BankHeader &header){return partition&&esp_partition_read(partition,index*BankBytes,&header,sizeof(header))==ESP_OK&&!memcmp(header.magic,"MWBANK01",8)&&header.commit==0xc0de0001&&header.checksum==bankChecksum(header)&&header.size>=sizeof(ModelPackageHeader)+40&&header.size<=ModelStore::MaxPackageBytes;}
bool readPayload(int bank,uint8_t *out,size_t size){uint8_t chunk[1024];for(size_t i=0;i<size;i+=sizeof(chunk)){size_t n=min(sizeof(chunk),size-i);if(esp_partition_read(partition,bank*BankBytes+sizeof(BankHeader)+i,chunk,n)!=ESP_OK)return false;memcpy(out+i,chunk,n);}return true;}
void fail(const char *message){phase("error",0,message);portENTER_CRITICAL(&mux);job.busy=false;receivingOwner=nullptr;portEXIT_CRITICAL(&mux);}
void updateTask(void*){
    uint8_t *payload=workPayload;size_t size=workSize;char error[180]={};
    phase("validating",5,"正在检查模型兼容性");
    if(!ModelStore::validate(payload,size,error,sizeof(error))){free(payload);fail(error);vTaskDelete(nullptr);return;}
    const int target=activeBank==0?1:0;const size_t offset=target*BankBytes;
    phase("writing",20,"正在保存模型，原模型仍保留");
    bool ok=esp_partition_erase_range(partition,offset,BankBytes)==ESP_OK;
    uint8_t check[1024];
    for(size_t i=0;ok&&i<size;i+=sizeof(check)){size_t n=min(sizeof(check),size-i);memcpy(check,payload+i,n);ok=esp_partition_write(partition,offset+sizeof(BankHeader)+i,check,n)==ESP_OK;phase("writing",20+int(60*(i+n)/size),"正在保存模型，原模型仍保留");vTaskDelay(1);}
    for(size_t i=0;ok&&i<size;i+=sizeof(check)){size_t n=min(sizeof(check),size-i);ok=esp_partition_read(partition,offset+sizeof(BankHeader)+i,check,n)==ESP_OK&&!memcmp(check,payload+i,n);}
    BankHeader header={};memcpy(header.magic,"MWBANK01",8);header.sequence=activeSequence+1;header.size=size;header.commit=0xc0de0001;memcpy(header.digest,payload+size-32,32);header.checksum=bankChecksum(header);
    if(ok)ok=esp_partition_write(partition,offset,&header,offsetof(BankHeader,commit))==ESP_OK;
    if(ok)ok=esp_partition_write(partition,offset+offsetof(BankHeader,commit),&header.commit,sizeof(header.commit))==ESP_OK;
    if(!ok){free(payload);fail("模型保存未完成，原模型未改动，请重试");vTaskDelete(nullptr);return;}
    portENTER_CRITICAL(&mux);hex(payload+size-32,job.nextHash);portEXIT_CRITICAL(&mux);free(payload);
    phase("complete",100,"模型已保存，魔杖正在重启");
    // Keep busy until reboot: no second upload may overwrite the pending bank.
    vTaskDelay(pdMS_TO_TICKS(3000));ESP.restart();vTaskDelete(nullptr);
}
}

int gesture_model_count(){return profile.classes;}
const char *gesture_model_label(int i){return i>=0&&i<gesture_model_count()?profile.labels[i]:"";}
const char *gesture_model_key(){return profileKey;}
const char *gesture_model_title(){return profileTitle;}
int gesture_model_default_action(int i){return !strcmp(profileKey,kGestureProfileKey)&&i>=0&&i<kGestureCount?kDefaultGestureActions[i]:-1;}
int gesture_model_fallback_count(){return fallbackCount;}
const char *gesture_model_fallback_key(int i){return i>=0&&i<fallbackCount?fallbackKeys[i]:"";}
int gesture_model_fallback_size(int i){return i>=0&&i<fallbackCount?fallbackSizes[i]:0;}

namespace ModelStore {
uint8_t *allocate(size_t size){auto *p=static_cast<uint8_t*>(heap_caps_aligned_alloc(16,size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return p?p:static_cast<uint8_t*>(heap_caps_aligned_alloc(16,size,MALLOC_CAP_8BIT));}
void use_builtin(){
    if(activePayload){free(activePayload);activePayload=nullptr;}activeBank=-1;activeSequence=0;
    memset(&profile,0,sizeof(profile));memcpy(profile.magic,"MWMODEL1",8);profile.version=1;profile.contract=1;profile.classes=kGestureCount;profile.modelBytes=gesture_model_tflite_len;
    memcpy(profile.mean,kBuiltinMean,sizeof(profile.mean));memcpy(profile.std,kBuiltinStd,sizeof(profile.std));for(int i=0;i<kGestureCount;++i)strlcpy(profile.labels[i],kGestureLabels[i],64);
    activeData=gesture_model_tflite;activeSize=gesture_model_tflite_len;uint8_t sha[32];digestParts(&profile,sizeof(profile),activeData,activeSize,sha);hex(sha,activeHash);catalog();
}
bool validate(const uint8_t *data,size_t size,char *error,size_t capacity,bool runModel){
    auto reject=[&](const char *message){strlcpy(error,message,capacity);return false;};
    if(!data||size<sizeof(ModelPackageHeader)+40||size>MaxPackageBytes)return reject("模型包不完整或过大");
    ModelPackageHeader h;memcpy(&h,data,sizeof(h));
    if(memcmp(h.magic,"MWMODEL1",8)||h.version!=1||h.contract!=1||h.classes<2||h.classes>16||h.modelBytes<8||h.modelBytes>MaxModelBytes||size!=sizeof(h)+h.modelBytes+32)return reject("模型包格式或识别协议不兼容");
    uint8_t sha[32];digest(data,size-32,sha);if(memcmp(sha,data+size-32,32))return reject("模型包校验失败，未修改原模型");
    for(int i=0;i<2;++i)if(!isfinite(h.mean[i])||!isfinite(h.std[i])||h.std[i]<=0)return reject("归一化参数无效");
    for(unsigned i=0;i<h.classes;++i){if(!labelValid(h.labels[i]))return reject("手势名称无效");for(unsigned j=0;j<i;++j)if(!strcmp(h.labels[i],h.labels[j]))return reject("手势名称重复");}
    if(runModel&&!inference_validate_model(data+sizeof(h),h.modelBytes,h.classes,error,capacity))return false;
    return true;
}
void init(){
    memset(&job,0,sizeof(job));strlcpy(job.phase,"idle",sizeof(job.phase));
    partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"spiffs");if(partition&&partition->size<2*BankBytes)partition=nullptr;job.storage=partition!=nullptr;use_builtin();
    BankHeader banks[2];bool present[2]={readBankHeader(0,banks[0]),readBankHeader(1,banks[1])};int first=present[1]&&(!present[0]||static_cast<int32_t>(banks[1].sequence-banks[0].sequence)>0)?1:0;
    for(int step=0;step<2;++step){int index=step?1-first:first;if(!present[index])continue;auto h=banks[index];auto *buffer=allocate(h.size);if(!buffer)continue;char error[180];
        if(readPayload(index,buffer,h.size)&&!memcmp(h.digest,buffer+h.size-32,32)&&validate(buffer,h.size,error,sizeof(error))){activePayload=buffer;memcpy(&profile,buffer,sizeof(profile));activeData=buffer+sizeof(profile);activeSize=profile.modelBytes;activeBank=index;activeSequence=h.sequence;hex(buffer+h.size-32,activeHash);catalog();return;}free(buffer);
    }
}
const uint8_t *model_data(){return activeData;}size_t model_size(){return activeSize;}
const float *mean(){return profile.mean;}const float *stddev(){return profile.std;}
const char *active_hash(){return activeHash;}bool from_storage(){return activePayload!=nullptr;}
bool busy(){portENTER_CRITICAL(&mux);bool value=job.busy;portEXIT_CRITICAL(&mux);return value;}
Status status(){portENTER_CRITICAL(&mux);Status copy=job;portEXIT_CRITICAL(&mux);return copy;}
bool reserve(void *owner,uint32_t &ticket){portENTER_CRITICAL(&mux);if(job.busy||!job.storage){portEXIT_CRITICAL(&mux);return false;}job.busy=true;job.ticket=esp_random();receivingOwner=owner;ticket=job.ticket;job.nextHash[0]=0;portEXIT_CRITICAL(&mux);phase("receiving",0,"正在接收模型");return true;}
void cancel(void *owner){portENTER_CRITICAL(&mux);bool match=receivingOwner==owner&&!strcmp(job.phase,"receiving");if(match){job.busy=false;receivingOwner=nullptr;}portEXIT_CRITICAL(&mux);}
bool submit(void *owner,uint8_t *payload,size_t size){portENTER_CRITICAL(&mux);bool match=receivingOwner==owner&&!strcmp(job.phase,"receiving");if(match){receivingOwner=nullptr;workPayload=payload;workSize=size;strlcpy(job.phase,"validating",sizeof(job.phase));}portEXIT_CRITICAL(&mux);if(!match)return false;
    if(xTaskCreatePinnedToCore(updateTask,"ModelUpdate",8192,nullptr,1,nullptr,0)!=pdPASS){fail("更新任务无法启动，原模型未改动");return false;}return true;
}
}
