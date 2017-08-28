#ifndef __JRCOMPRESSOR__
#define __JRCOMPRESSOR__

#include "IPlug_include_in_plug_hdr.h"

class jrCompressor : public IPlug
{
public:
  jrCompressor(IPlugInstanceInfo instanceInfo);
  ~jrCompressor();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  double mGain;
};

#endif
