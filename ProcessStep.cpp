/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AoiElement.h"
#include "AppVerify.h"
#include "BitMask.h"
#include "DataAccessor.h"
#include "DataAccessorImpl.h"
#include "DataRequest.h"
#include "DimensionDescriptor.h"
#include "Location.h"
#include "ObjectResource.h"
#include "ProcessStep.h"
#include "RasterCorrelator.h"
#include "RasterDataDescriptor.h"
#include "RasterElement.h"
#include "RasterMathException.h"
#include "switchOnEncoding.h"

#include <sstream>

using namespace boost;
using namespace Opticks;
using namespace std;

namespace
{
   Location<int,3> getArgDimensions(const vector<shared_ptr<ProcessStep> >& args, int argCount)
   {
      Location<int,3> dims;
      VERIFYRV(static_cast<int>(args.size()) >= argCount, dims);
      VERIFYRV(argCount >= 1, dims);
      vector<shared_ptr<ProcessStep> >::const_iterator ppStep=args.end()-1;
      RM_NULLCHK(*ppStep);
      dims.mX = max(dims.mX, (*ppStep)->columns());
      dims.mY = max(dims.mY, (*ppStep)->rows());
      dims.mZ = max(dims.mZ, (*ppStep)->bands());
      for (int i=0; i<argCount-1; ++i)
      {
         int subArgCount = (*ppStep)->argCount();
         while (subArgCount != 0)
         {
            --subArgCount;
            RM_VERIFY(ppStep != args.begin());
            --ppStep;
            subArgCount += RM_NULLCHK(*ppStep)->argCount();
         }
         RM_VERIFY(ppStep != args.begin());
         --ppStep;
         RM_NULLCHK(*ppStep);
         dims.mX = max(dims.mX, (*ppStep)->columns());
         dims.mY = max(dims.mY, (*ppStep)->rows());
         dims.mZ = max(dims.mZ, (*ppStep)->bands());
      }
      return dims;
   }

   template<typename T>
   double getRasterStepValue(T* pData)
   {
      return ModelServices::getDataValue(*pData, COMPLEX_MAGNITUDE);
   }
}

ProcessStepSignature::ProcessStepSignature(const std::string& description, Signature* pSignature, int bandCount) :
   ProcessStep(description, ProcessStep::RESULT_SIGNATURE),
   mpSignature(pSignature)
{
   mBands = bandCount;
}

ProcessStepAoi::ProcessStepAoi(const string& description) :
   ProcessStep(description, VALUE_AOI),
   mpElement(NULL),
   mpMask(NULL),
   mCurrentRow(0),
   mCurrentColumn(0)
{
   mArgCount = 0;

   int index = -1;
   stringstream descStream(description);
   char rasterChar = '\0';
   descStream >> rasterChar >> index;
   if (rasterChar != 'a' || index < 1 || index > AoiCorrelator::MAX_CORREL)
   {
      throw RasterMathException("Invalid AOI indicator: " + description);
   }

   mpElement = RM_NULLCHK(AoiCorrelator::instance())->getElement(index);
   mpMask = RM_NULLCHK(mpElement)->getSelectedPoints();

   int x1=0;
   int y1=0;
   int x2=0;
   int y2=0;
   RM_NULLCHK(mpMask)->getBoundingBox(x1, y1, x2, y2);
   int mRows = y2+1;
   int mColumns = x2+1;
}

void ProcessStepAoi::initialize()
{
   mCurrentRow = 0;
   mCurrentColumn = 0;
   mValue = mpMask->getPixel(mCurrentColumn, mCurrentRow);
}

bool ProcessStepAoi::nextRow()
{
   ++mCurrentRow;
   mCurrentColumn = 0;
   mValue = mpMask->getPixel(mCurrentColumn, mCurrentRow);
   return true;
}

bool ProcessStepAoi::nextColumn()
{
   ++mCurrentColumn;
   mValue = mpMask->getPixel(mCurrentColumn, mCurrentRow);
   return true;
}

ProcessStepRaster::ProcessStepRaster(const std::string& description, StepType type, int minBand, int maxBand) : 
   ProcessStep(description, type),
   mMinBand(minBand),
   mMaxBand(maxBand),
   mCurrentBand(minBand),
   mCurrentRow(0),
   mCurrentColumn(0),
   mpElement(NULL),
   mEncodingType(INT1UBYTE),
   mAccessor(NULL, NULL),
   mDefaultValue(1.0)
{
   mArgCount = 0;

   int index = -1;
   if (description == "result")
   {
      index = 0;
   }
   else
   {
      stringstream descStream(description);
      char rasterChar = '\0';
      descStream >> rasterChar >> index;
      if (rasterChar != 'r' || index < 1 || index > RasterCorrelator::MAX_CORREL)
      {
         throw RasterMathException("Invalid raster indicator: " + description);
      }
   }

   mpElement = RM_NULLCHK(RasterCorrelator::instance())->getElement(index);
   if (mpElement == NULL)
   {
      throw RasterMathException("No RasterElement for the given raster indicator: " + description);
   }

   RasterDataDescriptor* pDescriptor = dynamic_cast<RasterDataDescriptor*>(mpElement->getDataDescriptor());
   if (pDescriptor == NULL)
   {
      throw RasterMathException("Invalid RasterElement in RasterMath");
   }

   int numBands = pDescriptor->getBandCount();

   // to handle r1[n:] syntax
   if (mMaxBand == -1)
   {
      mMaxBand = numBands-1;
   }

   if (mMaxBand < mMinBand || mMaxBand>=numBands || mMinBand < 0)
   {
      stringstream indexErrors;
      indexErrors << "[" << minBand << ":" << maxBand << "], num bands = " << numBands;
      throw RasterMathException("Invalid raster subscript(s) for raster indicator: " + description+indexErrors.str());
   }

   mBands = mMaxBand-mMinBand+1;
   mRows = pDescriptor->getRowCount();
   mColumns = pDescriptor->getColumnCount();
   mEncodingType = pDescriptor->getDataType();
}

void ProcessStepRaster::initialize()
{
   mCurrentBand = mMinBand;
   updateAccessor();
   if (mStepType == ProcessStep::VALUE_RASTER)
   {
      switchOnEncoding(mEncodingType, mValue = getRasterStepValue, mAccessor->getColumn());
   }
}

bool ProcessStepRaster::nextRow()
{
   if (mCurrentRow != -1)
   {
      ++mCurrentRow;
      mAccessor->nextRow();
   }
   else
   {
      return false;
   }

   if (mAccessor.isValid() == false)
   {
      mCurrentRow = -1;
      mCurrentColumn = -1;
      mValue = mDefaultValue;
   }
   else
   {
      mCurrentColumn = 0;
      if (mStepType == ProcessStep::VALUE_RASTER)
      {
         switchOnEncoding(mEncodingType, mValue = getRasterStepValue, mAccessor->getColumn());
      }
   }

   return true;
}

bool ProcessStepRaster::nextColumn()
{
   if (mCurrentColumn == -1)
   {
      return false;
   }
   if (mCurrentColumn < mColumns-1)
   {
      ++mCurrentColumn;
      mAccessor->nextColumn();
      if (mStepType == ProcessStep::VALUE_RASTER)
      {
         switchOnEncoding(mEncodingType, mValue = getRasterStepValue, mAccessor->getColumn());
      }
   }
   else
   {
      mCurrentColumn = -1;
      mValue = mDefaultValue;
   }

   return true;
}

void ProcessStepRaster::updateAccessor()
{
   RasterDataDescriptor* pDescriptor = dynamic_cast<RasterDataDescriptor*>(RM_NULLCHK(mpElement)->getDataDescriptor());
   DimensionDescriptor startBand = RM_NULLCHK(pDescriptor)->getActiveBand(mCurrentBand);
   DimensionDescriptor stopBand = startBand;
   FactoryResource<DataRequest> pRequest;
   RM_NULLCHK(pRequest.get());
   pRequest->setBands(startBand, stopBand);
   mAccessor = mpElement->getDataAccessor(pRequest.release());
}

ProcessStepRasterResult::ProcessStepRasterResult(int bandCount) :
   ProcessStepRaster("result", RESULT_RASTER, 0, bandCount-1)
{
}

ProcessStepFunction::ProcessStepFunction(const std::string& description, StepType type, const vector<shared_ptr<ProcessStep> >& args, int argCount) : 
   ProcessStep(description, type)
{
   mArgCount = argCount;
   Location<int,3> argDims = getArgDimensions(args, argCount);
   mBands = static_cast<int>(argDims.mZ);
   mRows = static_cast<int>(argDims.mY);
   mColumns = static_cast<int>(argDims.mX);
}
