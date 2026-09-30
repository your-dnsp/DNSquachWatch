#include "test_util.h"
#include <cstdlib>
#include "ambient_light.h"
int main(){
 suite("Ambient brightness bounds and transitions");
 ck("simulated medium-low input is 200",AmbientLight::SIMULATED_READING==200);
 ck("reference has usable intermediate brightness",AmbientLight::target(200,255)>100&&AmbientLight::target(200,255)<180);
 bool bounded=true,monotonic=true;
 for(unsigned cap=32;cap<=255;++cap){unsigned previous=255;for(unsigned raw=0;raw<=4095;++raw){unsigned v=AmbientLight::target(raw,cap);bounded&=v>=32&&v<=cap;monotonic&=v<=previous;previous=v;}}
 ck("all ADC inputs respect brightness limits",bounded);ck("more darkness never brightens output",monotonic);
 bool safe=true;for(unsigned start=0;start<256;++start)for(unsigned target=0;target<256;++target){unsigned v=AmbientLight::approach(start,target);safe&=abs(int(v)-int(start))<=4&&abs(int(v)-int(target))<=abs(int(start)-int(target));}
 ck("brightness changes bounded without overshoot",safe);
 return report();
}
