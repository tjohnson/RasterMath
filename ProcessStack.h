/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PROCESSSTACK_H
#define PROCESSSTACK_H

#include "ObjectResource.h"
#include "RasterElement.h"
#include "Signature.h"

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

class ProcessStep;
class RasterMathProgress;

class ProcessStack
{
public:
   ProcessStack();
   ProcessStack(const ProcessStack& rhs);
   void clear() { mSteps.clear(); }
   void add(boost::shared_ptr<ProcessStep> step);
   void addResultStep(const std::string& baseName, EncodingType type, ProcessingLocation location);
   void pop_back();
   const std::vector<boost::shared_ptr<ProcessStep> >& getSteps() const { return mSteps; }
   void compute(std::vector<double>& workingStack, RasterMathProgress& progress);
   void setDegrees(bool asDegrees);
   RasterElement* releaseRaster();
   Signature* releaseSignature();
   void execute(RasterMathProgress& progress);
   void setFailureMode(bool failOnError, double defaultValue=0.0) { mFailOnError = failOnError; mDefaultValue = defaultValue; }
   int64_t totalWork() const;

private:
   void storeErrorValue();
   ProcessStep& previousStep(std::vector<boost::shared_ptr<ProcessStep> >::iterator ppStep, int dist) const;
   ProcessingLocation computeLocation(int rowCount, int columnCount, int bandCount, EncodingType type) const;
   void initializeSteps();
   void nextBand();
   void nextRow();
   void optimize();

   ModelResource<RasterElement> mpResultRaster;
   std::vector<boost::shared_ptr<ProcessStep> > mSteps; // after mpResultRaster so destroyed before mpResultRaster
   ModelResource<Signature> mpResultSignature;
   double mDefaultValue;
   double mToRadians;
   bool mFailOnError;
};

#endif
