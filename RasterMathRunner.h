/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef RASTERMATHRUNNER_H
#define RASTERMATHRUNNER_H

#include <string>

class ProcessStack;
class RasterElement;
class Signature;

class RasterMathRunner
{
public:
   enum DisplayType
   {
      DISPLAY_NONE,
      DISPLAY_RASTER_OVERLAY,
      DISPLAY_THRESHOLD_OVERLAY,
      DISPLAY_RASTER_WINDOW
   };

   RasterMathRunner(Progress* pProgress, bool& aborted);
   void execute(const std::string& formula);
   void setDisplayType(DisplayType type);
   void setBaseResultName(const std::string& baseName);
   void setResultEncoding(EncodingType type) { mResultEncoding = type; }
   void setFailureMode(bool failOnError, double defaultValue=0.0) { mFailOnError = failOnError; mDefaultValue = defaultValue; }
   void setRadians(bool radians);
   void setResultLocation(const ProcessingLocation& location) 
   { 
      mResultLocation = location; 
   }
   RasterElement* getRasterResult() const { return mpRasterResult; }
   Signature* getSignatureResult() const { return mpSignatureResult; }
   double getScalarResult() const { return mScalarResult; }

private:
   void executeFull(ProcessStack& stack);
   double executeScalar(ProcessStack& stack);
   void displayRaster(RasterElement* pElement);
   void displaySignature(Signature* pElement);

   DisplayType mDisplayType;
   std::string mBaseResultName;
   EncodingType mResultEncoding;
   ProcessingLocation mResultLocation;
   Progress* mpProgress;
   double mDefaultValue;
   bool mFailOnError;
   bool mRadians;
   RasterElement* mpRasterResult;
   Signature* mpSignatureResult;
   double mScalarResult;
   bool& mAborted;
};

#endif
