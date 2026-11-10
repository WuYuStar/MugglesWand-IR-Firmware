#pragma once
#include <stddef.h>
#include <stdint.h>
#include "ModelCatalog.h"

struct ModelPackageHeader {
    char magic[8];
    uint32_t version,classes,modelBytes,contract;
    float mean[2],std[2];
    char labels[kMaxGestureClasses][64];
    uint8_t reserved[24];
};
static_assert(sizeof(ModelPackageHeader)==1088,"Stable model-only wire format");
namespace ModelStore {
constexpr size_t MaxModelBytes=48*1024;
constexpr size_t MaxPackageBytes=sizeof(ModelPackageHeader)+MaxModelBytes+32;
struct Status {bool busy,storage;uint32_t ticket;int percent;char phase[20],message[180],nextHash[65];};
uint8_t *allocate(size_t size);
void init();
void use_builtin();
const uint8_t *model_data();
size_t model_size();
const float *mean();
const float *stddev();
const char *active_hash();
bool from_storage();
bool busy();
Status status();
bool reserve(void *owner,uint32_t &ticket);
void cancel(void *owner);
bool submit(void *owner,uint8_t *payload,size_t size); // takes ownership only on success
bool validate(const uint8_t *data,size_t size,char *error,size_t capacity,bool runModel=true);
}
// Validation allocates a separate temporary arena; live inference is never modified.
bool inference_validate_model(const uint8_t *data,size_t size,int classes,char *error,size_t capacity);
