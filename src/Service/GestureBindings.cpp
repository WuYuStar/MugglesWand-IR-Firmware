#include "GestureBindings.h"
#include "GestureEffects.h"
#include "HomeAssistant.h"
#include <Preferences.h>
#include <limits.h>
#include <string.h>

namespace {
Preferences storage, previousStorage, legacyStorage;
SemaphoreHandle_t saveMutex=nullptr;
portMUX_TYPE stateMux=portMUX_INITIALIZER_UNLOCKED;
GestureBindingState state={};
bool validActions(const int8_t *values,int count=gesture_model_count(),bool legacy=false) {
    if (!values) return false;
    for (int i=0;i<count;++i) if (values[i]<-1 || values[i]>kLastGestureAction || (!legacy && gesture_action_retired(values[i]))) return false;
    return true;
}
bool validEffects(const int8_t *values,int count=gesture_model_count()) {
    if (!values) return false;
    for (int i=0;i<count;++i) if (!gesture_effect_valid(values[i])) return false;
    return true;
}
bool load(Preferences &prefs,const char *key,int count,bool oldFormat) {
    if (count<1 || count>gesture_model_count()) return false;
    uint8_t blob[4+2*kMaxGestureClasses]={};
    const size_t size=oldFormat?count:4+2*count;
    if (prefs.getBytesLength(key)!=size || prefs.getBytes(key,blob,size)!=size) return false;
    const int8_t *actions=reinterpret_cast<int8_t*>(blob+(oldFormat?0:4));
    if (!validActions(actions,count,true)) return false;
    if (!oldFormat && (blob[0]!=0x4d || blob[1]!=0x57 || blob[2]!=2 || blob[3]!=count || !validEffects(actions+count,count))) return false;
    for (int i=0;i<count;++i) state.actions[i]=gesture_action_retired(actions[i]) ? -1 : actions[i];
    if (!oldFormat) memcpy(state.effects,actions+count,count);
    return true;
}
bool prefix(Preferences &prefs,bool oldFormat) {
    for (int i=0;i<gesture_model_fallback_count();++i) {
        if (load(prefs,gesture_model_fallback_key(i),gesture_model_fallback_size(i),oldFormat)) return true;
    }
    return false;
}
bool values(const char *&p,int8_t *out,int maximum) {
    for (int i=0;i<gesture_model_count();++i) {
        if (*p=='-' && p[1]=='1') {out[i]=-1;p+=2;}
        else {
            if (*p<'0' || *p>'9') return false;
            int number=*p++-'0';
            if (*p>='0' && *p<='9') {if (!number) return false;number=number*10+*p++-'0';}
            if (number>maximum || (*p>='0' && *p<='9')) return false;
            out[i]=number;
        }
        if (i<gesture_model_count()-1 && *p++!=',') return false;
    }
    return true;
}
bool parse(const char *command,int8_t *actions,int8_t *effects,uint32_t &revision,bool allowEffects) {
    if (!command) return false;
    const char prefix[]="web:bindings ";
    if (strncmp(command,prefix,sizeof(prefix)-1)) return false;
    const char *p=command+sizeof(prefix)-1;
    if (*p<'0' || *p>'9') return false;
    uint32_t number=0;
    while (*p>='0' && *p<='9') {
        uint32_t digit=*p++-'0';if (number>(UINT32_MAX-digit)/10) return false;
        number=number*10+digit;
    }
    if (*p++!=' ') return false;
    int8_t parsedActions[kMaxGestureClasses],parsedEffects[kMaxGestureClasses];
    if (!values(p,parsedActions,kLastGestureAction) || !validActions(parsedActions)) return false;
    if (*p==' ' && allowEffects) {
        ++p;if (!values(p,parsedEffects,kGestureEffectMax) || *p) return false;
    } else {
        if (*p) return false;
        auto current=gesture_bindings_state();memcpy(parsedEffects,current.effects,gesture_model_count());
    }
    memcpy(actions,parsedActions,gesture_model_count());
    if (effects) memcpy(effects,parsedEffects,gesture_model_count());
    revision=number;return true;
}
}

void gesture_bindings_init() {
    memset(state.actions,-1,sizeof(state.actions));
    for(int i=0;i<gesture_model_count();++i)state.actions[i]=gesture_model_default_action(i);
    memset(state.effects,-1,sizeof(state.effects));
    state.revision=1;
    saveMutex=xSemaphoreCreateMutex();state.writable=saveMutex && storage.begin("wand-map3",false);
    if (!state.writable) return;
    // New configuration is one atomic blob; the previous namespace is kept for rollback.
    if(load(storage,gesture_model_key(),gesture_model_count(),false))return;
    if (prefix(storage,false)) return;
    if (previousStorage.begin("wand-map2",true) &&
        (load(previousStorage,gesture_model_key(),gesture_model_count(),false) || prefix(previousStorage,false))) return;
    if (!legacyStorage.begin("wand-map",true)) return;
    if(!load(legacyStorage,gesture_model_key(),gesture_model_count(),true))prefix(legacyStorage,true);
}

GestureBindingState gesture_bindings_state() {
    portENTER_CRITICAL(&stateMux);GestureBindingState copy=state;portEXIT_CRITICAL(&stateMux);return copy;
}
int gesture_binding_action(int id) {
    if (id<0 || id>=gesture_model_count()) return -1;
    auto current=gesture_bindings_state();return current.actions[id];
}
BindingSave gesture_bindings_save(const int8_t actions[kMaxGestureClasses],const int8_t effects[kMaxGestureClasses],uint32_t expectedRevision) {
    if (!validActions(actions) || !validEffects(effects)) return BindingSave::Invalid;
    if (!saveMutex || xSemaphoreTake(saveMutex,pdMS_TO_TICKS(100))!=pdTRUE) return BindingSave::StorageError;
    auto current=gesture_bindings_state();BindingSave result=BindingSave::Saved;
    if (expectedRevision!=current.revision) result=BindingSave::Conflict;
    else if (!current.writable) result=BindingSave::StorageError;
    else if (memcmp(current.actions,actions,gesture_model_count()) || memcmp(current.effects,effects,gesture_model_count())) {
        uint8_t blob[4+2*kMaxGestureClasses]={0x4d,0x57,2,static_cast<uint8_t>(gesture_model_count())};
        memcpy(blob+4,actions,gesture_model_count());memcpy(blob+4+gesture_model_count(),effects,gesture_model_count());
        if (storage.putBytes(gesture_model_key(),blob,4+2*gesture_model_count())!=static_cast<size_t>(4+2*gesture_model_count())) result=BindingSave::StorageError;
        else {portENTER_CRITICAL(&stateMux);memcpy(state.actions,actions,gesture_model_count());memcpy(state.effects,effects,gesture_model_count());++state.revision;portEXIT_CRITICAL(&stateMux);}
    }
    xSemaphoreGive(saveMutex);return result;
}
BindingSave gesture_bindings_save(const int8_t actions[kMaxGestureClasses],uint32_t revision) {
    auto current=gesture_bindings_state();return gesture_bindings_save(actions,current.effects,revision);
}
bool gesture_bindings_parse(const char *command,int8_t actions[kMaxGestureClasses],uint32_t &revision) {
    return parse(command,actions,nullptr,revision,false);
}
bool gesture_bindings_parse(const char *command,int8_t actions[kMaxGestureClasses],int8_t effects[kMaxGestureClasses],uint32_t &revision) {
    return parse(command,actions,effects,revision,true);
}
