#include "jrCompressor.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,

  kGainX = 100,
  kGainY = 100,
  kKnobFrames = 60
};

jrCompressor::jrCompressor(IPlugInstanceInfo instanceInfo)
  :	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(2.);

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  
  /* == GUI STUFF == */
  
  // background image
  IBitmap Bg = pGraphics->LoadIBitmap(BG_ID, BG_FN);
  pGraphics->AttachControl(new IBitmapControl(this, 0, 0, -1, &Bg));
  
  IBitmap big_knob = pGraphics->LoadIBitmap(BIG_KNOB_ID, BIG_KNOB_FN, kKnobFrames);
  pGraphics->AttachControl(new IKnobMultiControl(this, 200, 120, kGain, &big_knob));
  
  /* =============== */
  

  AttachGraphics(pGraphics);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

jrCompressor::~jrCompressor() {}

void jrCompressor::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    // process audio signals
    *out1 = *in1 * mGain;
    *out2 = *in2 * mGain;
  }
}

void jrCompressor::Reset()
{
  TRACE;
  IMutexLock lock(this);
}

void jrCompressor::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  switch (paramIdx)
  {
    case kGain:
      mGain = GetParam(kGain)->Value() / 100.;
      break;

    default:
      break;
  }
}


void compress
(
 float*  wav_in,     // signal
 int     n,          // N samples
 double  threshold,  // threshold (percents)
 double  slope,      // slope angle (percents)
 int     sr,         // sample rate (smp/sec)
 double  tla,        // lookahead  (ms)
 double  twnd,       // window time (ms)
 double  tatt,       // attack time  (ms)
 double  trel        // release time (ms)
)
{
  typedef float   stereodata[2];
  stereodata*     wav = (stereodata*) wav_in; // our stereo signal
  threshold *= 0.01;          // threshold to unity (0...1)
  slope *= 0.01;              // slope to unity
  tla *= 1e-3;                // lookahead time to seconds
  twnd *= 1e-3;               // window time to seconds
  tatt *= 1e-3;               // attack time to seconds
  trel *= 1e-3;               // release time to seconds
  
  // attack and release "per sample decay"
  double  att = (tatt == 0.0) ? (0.0) : exp (-1.0 / (sr * tatt));
  double  rel = (trel == 0.0) ? (0.0) : exp (-1.0 / (sr * trel));
  
  // envelope
  double  env = 0.0;
  
  // sample offset to lookahead wnd start
  int     lhsmp = (int) (sr * tla);
  
  // samples count in lookahead window
  int     nrms = (int) (sr * twnd);
  
  // for each sample...
  for (int i = 0; i < n; ++i)
  {
    // now compute RMS
    double  summ = 0;
    
    // for each sample in window
    for (int j = 0; j < nrms; ++j)
    {
      int     lki = i + j + lhsmp;
      double  smp;
      
      // if we in bounds of signal?
      // if so, convert to mono
      if (lki < n)
        smp = 0.5 * wav[lki][0] + 0.5 * wav[lki][1];
      else
        smp = 0.0;      // if we out of bounds we just get zero in smp
      
      summ += smp * smp;  // square em..
    }
    
    double  rms = sqrt (summ / nrms);   // root-mean-square
    
    // dynamic selection: attack or release?
    double  theta = rms > env ? att : rel;
    
    // smoothing with capacitor, envelope extraction...
    // here be aware of pIV denormal numbers glitch
    env = (1.0 - theta) * rms + theta * env;
    
    // the very easy hard knee 1:N compressor
    double  gain = 1.0;
    if (env > threshold)
      gain = gain - (env - threshold) * slope;
    
    // result - two hard kneed compressed channels...
    float  leftchannel = wav[i][0] * gain;
    float  rightchannel = wav[i][1] * gain;
  }
}
