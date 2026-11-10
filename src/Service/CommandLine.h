#pragma once
#include <stddef.h>

// TCP packets are not command boundaries. Keep one bounded line per client.
class CommandLine {
    char text[64]{};
    size_t used=0;
    bool discard=false;
public:
    void reset(){used=0;discard=false;}
    template<class Handler> void feed(const void *bytes,size_t size,Handler handle){
        const auto *data=static_cast<const char*>(bytes);
        for(size_t i=0;i<size;++i){
            char c=data[i];
            if(c=='\n'||c=='\r'){
                if(used&&!discard){text[used]=0;handle(text);}
                reset();
            }else if(!c||used==sizeof(text)-1)discard=true;
            else if(!discard)text[used++]=c;
        }
    }
};
