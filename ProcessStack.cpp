/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "DataAccessorImpl.h"
#include "DataRequest.h"
#include "DataVariant.h"
#include "DimensionDescriptor.h"
#include "ProcessStack.h"
#include "ProcessStepStatFunc.h"
#include "RasterCorrelator.h"
#include "RasterDataDescriptor.h"
#include "RasterMathException.h"
#include "RasterMathProgress.h"
#include "RasterUtilities.h"
#include "Signature.h"
#include "switchOnEncoding.h"
#include "UtilityServices.h"

#include <math.h>
#include <cmath>

#include <QtCore/QString>

using namespace std;
using namespace boost;

namespace
{
   template<typename T>
   double getRasterStepValue(T* pData)
   {
      return ModelServices::getDataValue(*pData, COMPLEX_MAGNITUDE);
   }

   template<typename T>
   double maxValue()
   {
      return numeric_limits<T>::max();
   }
   template<typename T>
   double minValue()
   {
      return numeric_limits<T>::min();
   }
   template<>
   double minValue<float>()
   {
      return -numeric_limits<float>::max();
   }
   template<>
   double minValue<double>()
   {
      return -numeric_limits<double>::max();
   }

   template<typename T>
   double clampedValue(double data)
   {
      data = max(data, minValue<T>());
      data = min(data, maxValue<T>());
      return data;
   }

   template<typename T>
   void setRasterStepValue(T* pRasterData, double data)
   {
      *pRasterData = clampedValue<T>(data);
   }

   string getAvailableName(const string& baseStd, const string& typeName)
   {
      vector<DataElement*> allElements = Service<ModelServices>()->getElements(typeName);
      vector<DataElement*>::iterator ppElement = allElements.begin();

      QString base = QString::fromStdString(baseStd);
      QString name = base;

      for (int i=0; ppElement!=allElements.end(); ++i)
      {
         if (i == 0)
         {
            name = base;
         }
         else
         {
            name = base + " - " + QString::number(i);
         }
         for (ppElement=allElements.begin(); ppElement!=allElements.end(); ++ppElement)
         {
            if (RM_NULLCHK(*ppElement)->getName() == name.toStdString())
            {
               break;
            }
         }
      }

      return name.toStdString();
   }
}

ProcessStack::ProcessStack() :
   mpResultRaster(static_cast<RasterElement*>(NULL)),
   mpResultSignature(static_cast<Signature*>(NULL)),
   mDefaultValue(0.0),
   mToRadians(1.0),
   mFailOnError(false)
{
}

ProcessStack::ProcessStack(const ProcessStack& rhs) :
   mSteps(rhs.mSteps),
   mpResultRaster(static_cast<RasterElement*>(NULL)),
   mpResultSignature(static_cast<Signature*>(NULL)),
   mDefaultValue(rhs.mDefaultValue),
   mToRadians(rhs.mToRadians),
   mFailOnError(rhs.mFailOnError)
{
}

void ProcessStack::add(shared_ptr<ProcessStep> step)
{ 
   mSteps.insert(mSteps.end(), step);
}

void ProcessStack::setDegrees(bool asDegrees)
{
   if (asDegrees)
   {
      mToRadians = atan2(1.0,0.0)/90.0;
   }
   else
   {
      mToRadians = 1.0;
   }
}

void ProcessStack::addResultStep(const string& baseName, EncodingType type, ProcessingLocation location)
{
   RM_VERIFY(!mSteps.empty());
   RM_NULLCHK(mSteps.back());
   int bandCount = mSteps.back()->bands();
   int rowCount = mSteps.back()->rows();
   int columnCount = mSteps.back()->columns();
   if (bandCount == 1 && rowCount == 1 && columnCount == 1)
   {
      add(shared_ptr<ProcessStep>(new ProcessStepNumber("result", ProcessStep::RESULT_NUMBER, 2.0)));
   }
   else if (rowCount == 1 && columnCount == 1)
   {
      mpResultSignature = ModelResource<Signature>(static_cast<Signature*>(Service<ModelServices>()->createElement(
         getAvailableName(baseName, "Signature"), "Signature", NULL)));
      add(shared_ptr<ProcessStep>(new ProcessStepSignature("ResultSignature", mpResultSignature.get(), bandCount)));
   }
   else
   {
      if (location.isValid() == false)
      {
         location = computeLocation(rowCount, columnCount, bandCount, type);
      }
      mpResultRaster = ModelResource<RasterElement>(RasterUtilities::createRasterElement(
         getAvailableName(baseName, "RasterElement"), rowCount, columnCount, bandCount, type,
         BIP, location==IN_MEMORY));
      RM_NULLCHK(RasterCorrelator::instance())->setResultElement(mpResultRaster.get());
      add(shared_ptr<ProcessStep>(new ProcessStepRasterResult(bandCount)));
   }
}

ProcessingLocation ProcessStack::computeLocation(int rowCount, int columnCount, int bandCount, EncodingType type) const
{
#ifdef WIN_API
   int64_t totalMemory = rowCount;
   totalMemory *= columnCount;
   totalMemory *= bandCount;

   int sizePerElement = 1;
   switch(type)
   {
   case INT2UBYTES:
   case INT2SBYTES:
      sizePerElement = 2;
      break;
   case INT4UBYTES:
   case INT4SBYTES:
   case FLT4BYTES:
      sizePerElement = 4;
      break;
   case FLT8BYTES:
      sizePerElement = 8;
      break;
   default:
      sizePerElement = 1;
      break;
   }

   totalMemory *= sizePerElement;

   MEMORYSTATUSEX stat;
   stat.dwLength = sizeof(stat);
   GlobalMemoryStatusEx(&stat);
   int64_t availPhys = static_cast<int64_t>(stat.ullAvailPhys);

   if (totalMemory > availPhys)
   {
      return ON_DISK;
   }
   else
   {
      return IN_MEMORY;
   }
#else
   return IN_MEMORY;
#endif
}

RasterElement* ProcessStack::releaseRaster()
{
   return mpResultRaster.release();
}

Signature* ProcessStack::releaseSignature()
{
   return mpResultSignature.release();
}

void ProcessStack::pop_back()
{
   RM_VERIFY(!mSteps.empty());
   mSteps.pop_back();
}

ProcessStep& ProcessStack::previousStep(std::vector<shared_ptr<ProcessStep> >::iterator ppStep, int dist) const
{
   return **(ppStep-dist);
}

void ProcessStack::initializeSteps()
{
   for (vector<shared_ptr<ProcessStep> >::iterator ppStep=mSteps.begin();
      ppStep!=mSteps.end(); ++ppStep)
   {
      RM_NULLCHK(*ppStep)->initialize();
   }
}

void ProcessStack::nextBand()
{
   for (vector<shared_ptr<ProcessStep> >::iterator ppStep=mSteps.begin();
      ppStep!=mSteps.end(); ++ppStep)
   {
      ProcessStep& step = *RM_NULLCHK(*ppStep);
      switch (step.mStepType)
      {
         case ProcessStep::RESULT_SIGNATURE:
         {
            ProcessStepSignature& sigStep = static_cast<ProcessStepSignature&>(step);
            if (sigStep.mValues.size() == sigStep.mBands)
            {
               vector<double> indices;
               for (unsigned int i=0; i<sigStep.mValues.size(); ++i)
               {
                  indices.push_back(i+1);
               }
               RM_NULLCHK(sigStep.mpSignature)->setData("Raster Math Values", sigStep.mValues);
               RM_NULLCHK(sigStep.mpSignature)->setData("Raster Math Indices", indices);
            }
            break;
         }
         case ProcessStep::RESULT_RASTER:
         case ProcessStep::VALUE_RASTER:
         {
            ProcessStepRaster& rasterStep = static_cast<ProcessStepRaster&>(step);
            if (rasterStep.mBands == 1)
            {
               rasterStep.mAccessor->toPixel(0,0);
                  rasterStep.mCurrentRow = 0;
                  rasterStep.mCurrentColumn = 0;
               break;
            }
            else
            {
               if (rasterStep.mCurrentBand == -1)
               {
                  if (mFailOnError)
                  {
                     throw RasterMathException ("Raster band-size mismatch");
                  }
                  break;
               }
               if (rasterStep.mCurrentBand < rasterStep.mMaxBand)
               {
                  ++rasterStep.mCurrentBand;
                  rasterStep.updateAccessor();
               }
               if (rasterStep.mAccessor.isValid())
               {
                  rasterStep.mCurrentRow = 0;
                  rasterStep.mCurrentColumn = 0;
                  if (step.mStepType == ProcessStep::VALUE_RASTER)
                  {
                     switchOnEncoding(rasterStep.mEncodingType, rasterStep.mValue = getRasterStepValue, rasterStep.mAccessor->getColumn());
                  }
               }
               else
               {
                  rasterStep.mCurrentBand = -1;
                  rasterStep.mCurrentRow = -1;
                  rasterStep.mCurrentColumn = -1;
                  rasterStep.mValue = rasterStep.mDefaultValue;
               }
            }
            break;
         }
         case ProcessStep::COMPUTED_SIGNATURE:
         {
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            if (statStep.mValues.empty() == false)
            {
               step.mValue = statStep.mValues.front();
               statStep.mValues.pop_front();
            }
            break;
         }
         case ProcessStep::BAND_MIN_ACCUM:
         {
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.mValues.push_back(statStep.mAccumulator1);
            statStep.mAccumulator1 = std::numeric_limits<double>::max();
            break;
         }
         case ProcessStep::BAND_MAX_ACCUM:
         {
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.mValues.push_back(statStep.mAccumulator1);
            statStep.mAccumulator1 = -std::numeric_limits<double>::max();
            break;
         }
         case ProcessStep::BAND_SUM_ACCUM:
         {
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.mValues.push_back(statStep.mAccumulator1);
            statStep.mAccumulator1 = 0.0;
            break;
         }
         case ProcessStep::BAND_MEAN_ACCUM:
         case ProcessStep::BAND_GEOMEAN_ACCUM:
         {
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            if (statStep.mAccumulator2 == 0.0)
            {
               if (mFailOnError)
               {
                  throw RasterMathException ("Computing mean with no data values");
               }
               statStep.mValues.push_back(mDefaultValue);
            }
            else
            {
               statStep.mValues.push_back(statStep.mAccumulator1/statStep.mAccumulator2);
            }
            statStep.mAccumulator1 = 0.0;
            statStep.mAccumulator2 = 0.0;
            break;
         }
         case ProcessStep::BAND_HARMEAN_ACCUM:
         {
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            if (statStep.mAccumulator1 == 0.0 || statStep.mAccumulator2 == 0.0)
            {
               if (mFailOnError)
               {
                  throw RasterMathException ("Error computing harmonic mean");
               }
               statStep.mValues.push_back(mDefaultValue);
            }
            else
            {
               statStep.mValues.push_back(1.0/(statStep.mAccumulator1/statStep.mAccumulator2));
            }
            statStep.mAccumulator1 = 0.0;
            statStep.mAccumulator2 = 0.0;
            break;
         }
         case ProcessStep::BAND_STDDEV_ACCUM:
         {
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            if (statStep.mAccumulator3 <= 1.0)
            {
               if (mFailOnError)
               {
                  throw RasterMathException ("Too few values for standard deviation");
               }
               statStep.mValues.push_back(mDefaultValue);
            }
            else
            {
               double numerator = fabs(statStep.mAccumulator3 * statStep.mAccumulator2 - statStep.mAccumulator1 * statStep.mAccumulator1);
               double stdDev = sqrt((numerator / statStep.mAccumulator3) / (statStep.mAccumulator3 - 1.0));
               statStep.mValues.push_back(stdDev);
            }
            statStep.mAccumulator1 = 0.0;
            statStep.mAccumulator2 = 0.0;
            statStep.mAccumulator3 = 0.0;
            break;
         }
         default:
            break;
      }
   }
}

void ProcessStack::nextRow()
{
   for (vector<shared_ptr<ProcessStep> >::iterator ppStep=mSteps.begin();
      ppStep!=mSteps.end(); ++ppStep)
   {
      ProcessStep& step = *RM_NULLCHK(*ppStep);
      if (!step.nextRow() && mFailOnError)
      {
         throw RasterMathException ("Raster row-size mismatch");
      }
   }
}

#define COMPUTE1(errorChk,func) \
   RM_VERIFY(!stack.empty()); \
   double v1 = stack.back(); \
   if (errorChk) \
   { \
      storeErrorValue(); \
      stack.back() = mDefaultValue; \
      ppStep = mSteps.end()-1; \
   } \
   else \
   { \
      stack.back() = func; \
   }

#define SAFE_COMPUTE2(func) \
   RM_VERIFY(stack.size()>=2); \
   double v1 = stack.back(); \
   stack.pop_back(); \
   double v2 = stack.back(); \
   stack.back() = func;

#define COMPUTE2(errorChk,func) \
   RM_VERIFY(stack.size()>=2); \
   double v1 = stack.back(); \
   stack.pop_back(); \
   double v2 = stack.back(); \
   if (errorChk) \
   { \
      storeErrorValue(); \
      stack.back() = mDefaultValue; \
      ppStep = mSteps.end()-1; \
   } \
   else \
   { \
      stack.back() = func; \
   }

void ProcessStack::compute(vector<double>& stack, RasterMathProgress& progress)
{
   double result = mDefaultValue;
   if (mSteps.empty())
   {
      throw RasterMathException("Computing empty process stack");
   }
   for (vector<shared_ptr<ProcessStep> >::iterator ppStep=mSteps.begin();
      ppStep!=mSteps.end(); ++ppStep)
   {
      ProcessStep& step = *RM_NULLCHK(*ppStep);
      switch (step.mStepType)
      {
         case ProcessStep::NUMBER:
            stack.push_back(step.mValue);
            continue;
         case ProcessStep::VALUE_RASTER:
            stack.push_back(step.mValue);
            if (!step.nextColumn() && mFailOnError)
            {
               throw RasterMathException ("Raster column-size mismatch");
            }
            continue;
         case ProcessStep::ADD:
         {
            SAFE_COMPUTE2(v2+v1);
            continue;
         }
         case ProcessStep::SUBTRACT:
         {
            SAFE_COMPUTE2(v2-v1);
            continue;
         }
         case ProcessStep::MULTIPLY:
         {
            SAFE_COMPUTE2(v2*v1);
            continue;
         }
         case ProcessStep::DIVIDE:
         {
            COMPUTE2(v1==0.0, v2/v1);
            continue;
         }
         case ProcessStep::RESULT_NUMBER:
            RM_VERIFY(!stack.empty());
            step.mValue = stack.back();
            stack.pop_back();
            continue;
         case ProcessStep::RESULT_SIGNATURE:
         {
            RM_VERIFY(!stack.empty());
            ProcessStepSignature& sigStep = static_cast<ProcessStepSignature&>(step);
            sigStep.mValues.push_back(stack.back());
            stack.pop_back();
            continue;
         }
         case ProcessStep::RESULT_RASTER:
         {
            RM_VERIFY(!stack.empty());
            ProcessStepRasterResult& rasterStep = static_cast<ProcessStepRasterResult&>(step);
            result = stack.back();
            switchOnEncoding(rasterStep.mEncodingType, setRasterStepValue, rasterStep.mAccessor->getColumn(), result);
            stack.pop_back();
            if (!step.nextColumn() && mFailOnError)
            {
               throw RasterMathException ("Raster column-size mismatch");
            }
            continue;
         }
         case ProcessStep::NEGATE:
            RM_VERIFY(!stack.empty());
            stack.back() = -stack.back();
            continue;
         case ProcessStep::EXPONENTIATE:
         {
            COMPUTE2(v1==0.0&&v2==0.0, pow(v2, v1));
            continue;
         }
         case ProcessStep::ABS:
            RM_VERIFY(!stack.empty());
            stack.back() = fabs(stack.back());
            continue;
         case ProcessStep::SQRT:
         {
            COMPUTE1(v1<0.0, sqrt(v1));
            continue;
         }
         case ProcessStep::ACOS:
         {
            COMPUTE1(v1<-1.0||v1>1.0, acos(v1)/mToRadians);
            continue;
         }
         case ProcessStep::COS:
            RM_VERIFY(!stack.empty());
            stack.back() = cos(stack.back()*mToRadians);
            continue;
         case ProcessStep::ASIN:
         {
            COMPUTE1(v1<-1.0||v1>1.0, asin(v1)/mToRadians);
            continue;
         }
         case ProcessStep::SIN:
            RM_VERIFY(!stack.empty());
            stack.back() = sin(stack.back()*mToRadians);
            continue;
         case ProcessStep::ATAN:
         {
            COMPUTE1(v1==0.0, atan(v1)/mToRadians);
            continue;
         }
         case ProcessStep::TAN:
            RM_VERIFY(!stack.empty());
            stack.back() = tan(stack.back()*mToRadians);
            continue;
         case ProcessStep::COSH:
            RM_VERIFY(!stack.empty());
            stack.back() = cosh(stack.back());
            continue;
         case ProcessStep::SINH:
            RM_VERIFY(!stack.empty());
            stack.back() = sinh(stack.back());
            continue;
         case ProcessStep::TANH:
            RM_VERIFY(!stack.empty());
            stack.back() = tanh(stack.back());
            continue;
         case ProcessStep::EXP:
            RM_VERIFY(!stack.empty());
            stack.back() = exp(stack.back());
            continue;
         case ProcessStep::LOG10:
         {
            COMPUTE1(v1<=0.0, log10(v1));
            continue;
         }
         case ProcessStep::LOG2:
         {
            COMPUTE1(v1<=0.0, log10(v1)/log10(2.0));
            continue;
         }
         case ProcessStep::LOG:
         {
            COMPUTE1(v1<=0.0, ::log(v1));
            continue;
         }
         case ProcessStep::ATAN2:
         {
            COMPUTE2(v1==0.0&&v2==0.0, atan2(v2, v1)/mToRadians);
            continue;
         }
         case ProcessStep::LOGN:
         {
            COMPUTE2(v1<=0.0||v2<=0.0, log10(v2)/log10(v1)); // optimize by making a special logStep with 1.0/log10(v1) precomputed
            continue;
         }
         case ProcessStep::MODULO:
         {
            COMPUTE2(v1==0.0, fmod(v2, v1));
            continue;
         }
         case ProcessStep::LESS_THAN:
         {
            SAFE_COMPUTE2(static_cast<double>(v2<v1));
            continue;
         }
         case ProcessStep::GREATER_THAN:
         {
            SAFE_COMPUTE2(static_cast<double>(v2>v1));
            continue;
         }
         case ProcessStep::LESS_OR_EQUAL:
         {
            SAFE_COMPUTE2(static_cast<double>(v2<=v1));
            continue;
         }
         case ProcessStep::GREATER_OR_EQUAL:
         {
            SAFE_COMPUTE2(static_cast<double>(v2>=v1));
            continue;
         }
         case ProcessStep::EQUALS:
         {
            SAFE_COMPUTE2(static_cast<double>(v2==v1));
            continue;
         }
         case ProcessStep::NOT_EQUALS:
         {
            SAFE_COMPUTE2(static_cast<double>(v2!=v1));
            continue;
         }
         case ProcessStep::NOT:
            RM_VERIFY(!stack.empty());
            stack.back() = static_cast<double>(stack.back()==0.0);
            continue;
         case ProcessStep::AND:
         {
            SAFE_COMPUTE2(static_cast<double>(v2!=0.0 && v1!=0.0));
            continue;
         }
         case ProcessStep::OR:
         {
            SAFE_COMPUTE2(static_cast<double>(v2!=0.0 || v1!=0.0));
            continue;
         }
         case ProcessStep::CLAMP:
         {
            RM_VERIFY(stack.size()>=3);
            double v1 = stack.back();
            stack.pop_back();
            double v2 = stack.back();
            stack.pop_back();
            double v3 = stack.back();
            v3 = max(v2, min(v3, v1));
            stack.back() = v3;
            continue;
         }
         case ProcessStep::COMPUTED_SIGNATURE:
            stack.push_back(step.mValue);
            continue;
         case ProcessStep::BAND_MIN:
         case ProcessStep::BAND_MAX:
         case ProcessStep::BAND_SUM:
         case ProcessStep::BAND_MEAN:
         case ProcessStep::BAND_GEOMEAN:
         case ProcessStep::BAND_HARMEAN:
         case ProcessStep::BAND_STDDEV:
         {
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.execute(progress);
            stack.push_back(step.mValue);
            continue;
         }
         case ProcessStep::BAND_MIN_ACCUM:
         {
            RM_VERIFY(!stack.empty());
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.mAccumulator1 = min(statStep.mAccumulator1, stack.back());
            stack.pop_back();
            continue;
         }
         case ProcessStep::BAND_MAX_ACCUM:
         {
            RM_VERIFY(!stack.empty());
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.mAccumulator1 = max(statStep.mAccumulator1, stack.back());
            stack.pop_back();
            continue;
         }
         case ProcessStep::BAND_SUM_ACCUM:
         {
            RM_VERIFY(!stack.empty());
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.mAccumulator1 += stack.back();
            stack.pop_back();
            continue;
         }
         case ProcessStep::BAND_MEAN_ACCUM:
         {
            RM_VERIFY(!stack.empty());
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.mAccumulator1 += stack.back();
            statStep.mAccumulator2++;
            stack.pop_back();
            continue;
         }
         case ProcessStep::BAND_GEOMEAN_ACCUM:
         {
            RM_VERIFY(!stack.empty());
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.mAccumulator1 *= stack.back();
            statStep.mAccumulator2++;
            stack.pop_back();
            continue;
         }
         case ProcessStep::BAND_HARMEAN_ACCUM:
         {
            RM_VERIFY(!stack.empty());
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            if (stack.back() == 0.0)
            {
               storeErrorValue();
               stack.back() = mDefaultValue;
               ppStep = mSteps.end()-1;
            }
            else
            {
               statStep.mAccumulator1 += 1.0/stack.back();
               statStep.mAccumulator2++;
               stack.pop_back();
            }
            continue;
         }
         case ProcessStep::BAND_STDDEV_ACCUM:
         {
            RM_VERIFY(!stack.empty());
            ProcessStepStatFunc& statStep = static_cast<ProcessStepStatFunc&>(step);
            statStep.mAccumulator1 += stack.back();
            statStep.mAccumulator2 += stack.back()*stack.back();
            statStep.mAccumulator3++;
            stack.pop_back();
            continue;
         }
         case ProcessStep::VALUE_AOI:
            stack.push_back(step.mValue);
            step.nextColumn();
            continue;
         case ProcessStep::REFERENCE:
         {
            ProcessStepReference& refStep = static_cast<ProcessStepReference&>(step);
            stack.push_back(refStep.mRef);
            continue;
         }
         default:
            break;
      }
   }
}

void ProcessStack::storeErrorValue()
{
   if (mFailOnError)
   {
      throw RasterMathException ("Computation error");
   }

   RM_VERIFY(!mSteps.empty());
   shared_ptr<ProcessStep> pStep = mSteps.back();
   RM_NULLCHK(pStep);
   if (pStep->type() == ProcessStep::RESULT_RASTER)
   {
      ProcessStepRasterResult& rasterStep = static_cast<ProcessStepRasterResult&>(*pStep);
      switchOnEncoding(rasterStep.mEncodingType, setRasterStepValue, rasterStep.mAccessor->getColumn(), mDefaultValue);
      rasterStep.nextColumn();
   }
   else if (pStep->type() == ProcessStep::RESULT_NUMBER)
   {
      pStep->mValue = mDefaultValue;
   }
}

void ProcessStack::execute(RasterMathProgress& progress)
{
   const std::vector<shared_ptr<ProcessStep> >& mSteps = getSteps();
   if (mSteps.empty())
   {
      throw RasterMathException("Formula is empty");
   }

   optimize();
   initializeSteps();

   RM_VERIFY(!mSteps.empty());
   RM_NULLCHK(mSteps.back());
   int bandCount = mSteps.back()->bands();
   int rowCount = mSteps.back()->rows();
   int columnCount = mSteps.back()->columns();

   vector<double> workingStack;
   workingStack.reserve(mSteps.size());

   for (int band=0; band<bandCount; ++band)
   {
      for (int row=0; row<rowCount; ++row)
      {
         for (int column=0; column<columnCount; ++column)
         {
            workingStack.clear();
            compute(workingStack, progress);
         }
         nextRow();
         bool aborted = progress.addWorkCompleted(columnCount*mSteps.size());
         if (aborted)
         {
            throw RasterMathAbortException("Raster Math aborted");
         }
      }
      nextBand();
   }
}

int64_t ProcessStack::totalWork() const
{
   RM_VERIFY(!mSteps.empty());
   RM_NULLCHK(mSteps.back());
   int bandCount = mSteps.back()->bands();
   int rowCount = mSteps.back()->rows();
   int columnCount = mSteps.back()->columns();

   int64_t work = static_cast<int64_t>(bandCount) * rowCount * columnCount * mSteps.size();
   for (vector<shared_ptr<ProcessStep> >::const_iterator ppStep=mSteps.begin();
      ppStep!=mSteps.end(); ++ppStep)
   {
      work += RM_NULLCHK(*ppStep)->oneTimeWork();
   }

   return work;
}

void ProcessStack::optimize()
{
   // long-term, create a suffix tree to identify repeated substrings
   // for now, identify repeated raster or statistical function steps 
   // via n^2 search :(
   for (vector<shared_ptr<ProcessStep> >::iterator ppStep1=mSteps.begin();
      ppStep1!=mSteps.end(); ++ppStep1)
   {
      for (vector<shared_ptr<ProcessStep> >::iterator ppStep2=ppStep1+1;
         ppStep2!=mSteps.end(); ++ppStep2)
      {
         if (*RM_NULLCHK(*ppStep1) == *RM_NULLCHK(*ppStep2))
         {
            ProcessStep::StepType type = (*ppStep1)->type();
            if (type == ProcessStep::VALUE_RASTER || 
               type == ProcessStep::BAND_MIN ||
               type == ProcessStep::BAND_MAX ||
               type == ProcessStep::BAND_MEAN ||
               type == ProcessStep::BAND_GEOMEAN ||
               type == ProcessStep::BAND_HARMEAN ||
               type == ProcessStep::BAND_SUM ||
               type == ProcessStep::BAND_STDDEV)
            {
               *ppStep2 = shared_ptr<ProcessStep>(new ProcessStepReference(**ppStep1));
            }
         }
      }
   }
}
