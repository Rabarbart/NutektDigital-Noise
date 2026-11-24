// NOISELAB 2.0 experimental: gesture-driven noise instrument for NTS-3 genericfx.
// Copyright (c) 2026 RABARBART. Derived from the user's NOISELAB concept;
// independent implementation using the official Korg genericfx API.
#include "unit_genericfx.h"
#include <stdint.h>
#include <math.h>

namespace {
constexpr float NL_PI=3.14159265358979323846f;
constexpr unsigned NVOICES=4;
constexpr unsigned NP=8;
static float sr=48000.f;
// 0 TYPE, 1 ATTACK, 2 DECAY, 3 RELEASE, 4 TONE,
// 5 RESONANCE, 6 TEXTURE, 7 LEVEL.
static int32_t params[NP]={0,0,300,350,512,140,0,650};
static uint32_t rng=0x9E3779B9u;
static bool touching=false;
static uint32_t touchSamples=0;
static float sustain=0.f;
static float toneSmooth=0.5f, resonanceSmooth=0.137f, textureSmooth=0.f, levelSmooth=0.f;
static float typeSmooth=0.f;
static float attackStep=0.1f, hitDecayCoef=0.99f, releaseCoef=0.99f;
static unsigned ctrlTicks=0;
static float pink[7]={0.f};
static float brown=0.f,brownDc=0.f,bandFast=0.f,bandSlow=0.f;
static float heldNoise=0.f,pulseEnv=0.f,crackleEnv=0.f;
static int holdCounter=0;
struct Hit { float env; uint8_t phase; };
static Hit hits[NVOICES];

static inline float clip(float x,float lo,float hi){return x<lo?lo:(x>hi?hi:x);}
static inline uint32_t randomU32(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static inline float random01(){return (randomU32()>>8)*(1.f/16777216.f);}
static inline float randomBipolar(){return 2.f*random01()-1.f;}
static inline float mapLog(float minval,float maxval,int32_t value){return minval*powf(maxval/minval,clip(float(value)/1023.f,0.f,1.f));}
struct SVF {
  float ic1,ic2,a1,a2,a3,k;
  void reset(){ic1=ic2=0.f;a1=1.f;a2=a3=0.f;k=1.4142135f;}
  void configure(float hz,float q){
    const float g=tanf(NL_PI*clip(hz,20.f,15500.f)/sr);
    k=1.f/clip(q,0.55f,3.5f);
    a1=1.f/(1.f+g*(g+k));a2=g*a1;a3=g*a2;
  }
  inline void run(float x,float &lp,float &hp){
    const float v3=x-ic2;
    const float v1=a1*ic1+a2*v3;
    const float v2=ic2+a2*ic1+a3*v3;
    ic1=2.f*v1-ic1;ic2=2.f*v2-ic2;
    lp=v2;hp=x-k*v1-v2;
  }
};
static SVF lowpass,highpass;

static void recalc(){
  const float a=mapLog(0.0003f,0.150f,params[1]);
  const float d=mapLog(0.008f,3.000f,params[2]);
  const float r=mapLog(0.007f,2.500f,params[3]);
  attackStep=1.f/(sr*a);
  hitDecayCoef=expf(-6.907755f/(sr*d));
  releaseCoef=expf(-6.907755f/(sr*r));
  const float t=clip(toneSmooth,0.f,1.f);
  const float lpNorm=clip(2.f*t,0.f,1.f),hpNorm=clip(2.f*t-1.f,0.f,1.f);
  const float lpHz=120.f*powf(125.f,lpNorm); // 120 -> 15 kHz
  const float hpHz=30.f*powf(300.f,hpNorm); // 30 -> 9 kHz
  const float q=0.7071068f+2.8f*clip(resonanceSmooth,0.f,1.f);
  lowpass.configure(lpHz,q);
  highpass.configure(hpHz,q);
}
static void resetDSP(){
 touching=false;touchSamples=0;sustain=0.f;ctrlTicks=0;
 toneSmooth=float(params[4])/1023.f;
 resonanceSmooth=float(params[5])/1023.f;
 textureSmooth=float(params[6])/1023.f;
 levelSmooth=float(params[7])/1023.f;
 typeSmooth=float(params[0]);
 for(unsigned i=0;i<7;i++)pink[i]=0.f;
 brown=brownDc=bandFast=bandSlow=0.f;
 heldNoise=pulseEnv=crackleEnv=0.f;holdCounter=0;
 for(unsigned i=0;i<NVOICES;i++){hits[i].env=0.f;hits[i].phase=0;}
 lowpass.reset();highpass.reset();recalc();
}
static void triggerHit(){
 unsigned chosen=0;float least=100.f;
 for(unsigned i=0;i<NVOICES;i++){
   if(hits[i].phase==0){chosen=i;break;}
   if(hits[i].env<least){least=hits[i].env;chosen=i;}
 }
 hits[chosen].env=0.f;hits[chosen].phase=1;
 // Percussive sources always have an initial event, even with low density.
 pulseEnv=1.f;
 crackleEnv=(random01()>0.5f?1.f:-1.f);
}
static float processOne(){
 // Smooth the remappable parameters; never infer XY/Y/depth from raw touch coordinates.
 const float smooth=0.0040f;
 toneSmooth+=smooth*(float(params[4])/1023.f-toneSmooth);
 resonanceSmooth+=smooth*(float(params[5])/1023.f-resonanceSmooth);
 textureSmooth+=smooth*(float(params[6])/1023.f-textureSmooth);
 levelSmooth+=smooth*(float(params[7])/1023.f-levelSmooth);
 typeSmooth+=0.009f*(float(params[0])-typeSmooth);
 if((ctrlTicks++&31U)==0U)recalc();
 // Short percussive hits have full attack followed by decay (no decay during attack).
 float hitsGain=0.f;
 for(unsigned i=0;i<NVOICES;i++){
   Hit &v=hits[i];
   if(v.phase==1){v.env+=attackStep;if(v.env>=1.f){v.env=1.f;v.phase=2;}}
   else if(v.phase==2){v.env*=hitDecayCoef;if(v.env<0.00005f){v.env=0.f;v.phase=0;}}
   hitsGain+=v.env;
 }
 // Sustained gesture engages after 80ms; taps only launch autonomous HIT voices.
 // No explicit MODE and no requirement to move a finger.
 if(touching){
   if(touchSamples<0x7fffffffu)++touchSamples;
   if(touchSamples>=uint32_t(0.080f*sr))sustain+=clip(1.f/(sr*0.035f),0.f,1.f);
   if(sustain>1.f)sustain=1.f;
 }else{
   sustain*=releaseCoef;
   if(sustain<0.00002f)sustain=0.f;
 }
 const float env=hitsGain>sustain?hitsGain:sustain;
 if(env<0.000001f)return 0.f;
 const float w=randomBipolar();
 // Well-established 7-tap pink-noise approximation (Kellet-style coefficients).
 pink[0]=0.99886f*pink[0]+w*0.0555179f;
 pink[1]=0.99332f*pink[1]+w*0.0750759f;
 pink[2]=0.96900f*pink[2]+w*0.1538520f;
 pink[3]=0.86650f*pink[3]+w*0.3104856f;
 pink[4]=0.55000f*pink[4]+w*0.5329522f;
 pink[5]=-0.7616f*pink[5]-w*0.0168980f;
 const float pinkN=(pink[0]+pink[1]+pink[2]+pink[3]+pink[4]+pink[5]+pink[6]+w*0.5362f)*0.16f;
 pink[6]=w*0.115926f;
 brown=0.99905f*brown+0.0285f*w;
 brownDc=0.999975f*brownDc+0.000025f*brown;
 const float brownN=(brown-brownDc)*2.0f;
 bandFast+=0.18f*(w-bandFast);
 bandSlow+=0.016f*(w-bandSlow);
 const float filteredN=(bandFast-bandSlow)*2.f;
 const float t=clip(textureSmooth,0.f,1.f);
 const int period=1+int(96.f*t*t);
 if(holdCounter<=0){
   holdCounter=period;
   const float qlevels=256.f-240.f*t;
   heldNoise=floorf(w*qlevels+0.5f)/qlevels;
 }
 --holdCounter;
 // PULSE/CRACKLE rates respond to TEXTURE, not to an autonomous LFO.
 const float pulseRate=8.f+420.f*t*t;
 if(random01()<pulseRate/sr)pulseEnv=1.f;
 pulseEnv*=0.980f-0.015f*t;
 const float crackleRate=2.f+360.f*t*t;
 if(random01()<crackleRate/sr)crackleEnv=(random01()>0.5f?1.f:-1.f);
 crackleEnv*=0.94f;
 const float rough=t*t;
 float source[8];
 source[0]=w;
 source[1]=pinkN*1.65f;
 source[2]=brownN*0.69f;
 source[3]=filteredN*1.75f;
 source[4]=0.46f*w+0.49f*pinkN+0.57f*brownN;
 source[5]=w*pulseEnv*2.7f;
 source[6]=crackleEnv*1.40f+w*0.060f;
 source[7]=heldNoise*0.92f;
 // TEXTURE progressively introduces sample/hold texture to continuous sources.
 for(unsigned j=0;j<5;j++)source[j]=source[j]*(1.f-0.35f*rough)+heldNoise*(0.35f*rough);
 int lower=int(typeSmooth);if(lower<0)lower=0;if(lower>7)lower=7;
 int upper=lower<7?lower+1:7;
 const float blend=clip(typeSmooth-float(lower),0.f,1.f);
 const float src=source[lower]+blend*(source[upper]-source[lower]);
 float lp,hp,unused1,unused2;
 lowpass.run(src,lp,unused1);
 highpass.run(src,unused2,hp);
 // Centre position ~ dry; left=LP, right=HP, with continuous transitions.
 const float tone=clip(toneSmooth,0.f,1.f);
 float shaped;
 if(tone<0.5f){
   const float dryMix=clip((tone-0.45f)/0.05f,0.f,1.f);
   shaped=lp*(1.f-dryMix)+src*dryMix;
 }else{
   const float hpMix=clip((tone-0.5f)/0.05f,0.f,1.f);
   shaped=src*(1.f-hpMix)+hp*hpMix;
 }
 // Mild resonance compensation and soft output limiting.
 const float q=0.7071f+2.8f*resonanceSmooth;
 const float gainComp=1.f/(1.f+0.16f*(q-0.7071f));
 const float output=shaped*env*(0.77f*levelSmooth*gainComp);
 return output/(1.f+fabsf(output));
}
} // namespace

extern "C" {
__unit_callback int8_t unit_init(const unit_runtime_desc_t *d){
 if(!d)return k_unit_err_undef;
 if(d->target!=unit_header.common.target)return k_unit_err_target;
 if(!UNIT_API_IS_COMPAT(d->api))return k_unit_err_api_version;
 if(d->samplerate!=48000U)return k_unit_err_samplerate;
 if(d->input_channels!=2||d->output_channels!=2)return k_unit_err_geometry;
 sr=float(d->samplerate);
 for(unsigned i=0;i<NP;i++)params[i]=unit_header.common.params[i].init;
 resetDSP();return k_unit_err_none;
}
__unit_callback void unit_teardown(void){}
__unit_callback void unit_reset(void){resetDSP();}
__unit_callback void unit_resume(void){}
__unit_callback void unit_suspend(void){}
__unit_callback void unit_render(const float *in,float *out,uint32_t frames){
 (void)in;
 for(uint32_t i=0;i<frames;i++){
   const float sample=processOne();out[2*i]=sample;out[2*i+1]=sample;
 }
}
__unit_callback void unit_set_param_value(uint8_t id,int32_t v){
 if(id>=NP)return;
 if(v<unit_header.common.params[id].min)v=unit_header.common.params[id].min;
 if(v>unit_header.common.params[id].max)v=unit_header.common.params[id].max;
 params[id]=v;
}
__unit_callback int32_t unit_get_param_value(uint8_t id){return id<NP?params[id]:0;}
__unit_callback const char *unit_get_param_str_value(uint8_t id,int32_t v){
 if(id==0){
   static const char *names[]={"WHITE","PINK","BROWN","FILTERED","MIX","PULSE","CRACKLE","DIGITAL"};
   return (v>=0&&v<8)?names[v]:"WHITE";
 }
 return "";
}
__unit_callback void unit_touch_event(uint8_t id,uint8_t phase,uint32_t x,uint32_t y){
 (void)id;(void)x;(void)y;  // XY parameter mapping is owned by the NTS-3 host/Editor.
 if(phase==k_unit_touch_phase_began){touching=true;touchSamples=0;triggerHit();}
 else if(phase==k_unit_touch_phase_moved||phase==k_unit_touch_phase_stationary){touching=true;}
 else if(phase==k_unit_touch_phase_ended||phase==k_unit_touch_phase_cancelled){touching=false;touchSamples=0;}
}
__unit_callback void unit_set_tempo(uint32_t tempo){(void)tempo;}
__unit_callback void unit_tempo_4ppqn_tick(uint32_t tick){(void)tick;}
}