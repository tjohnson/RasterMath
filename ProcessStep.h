/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PROCESSSTEP_H
#define PROCESSSTEP_H

#include "AppConfig.h"
#include "DataAccessor.h"
#include "TypesFile.h"

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

class AoiElement;
class BitMask;
class RasterElement;
class Signature;

class ProcessStep
{
   friend class ProcessStack;
public:
   enum StepType 
   { 
      NUMBER, 
      NEGATE, 
      EXPONENTIATE, 
      MULTIPLY,
      DIVIDE,
      MODULO,
      ADD,
      SUBTRACT,
      RESULT_NUMBER,
      RESULT_SIGNATURE,
      RESULT_RASTER,
      VALUE_RASTER,
      VALUE_AOI,
      ABS,
      ACOS,
      COS,
      ASIN,
      SIN,
      ATAN,
      TAN,
      COSH,
      SINH,
      TANH,
      SQRT,
      EXP,
      COMPUTED_SIGNATURE,
      BAND_MIN,
      BAND_MAX,
      BAND_MEAN,
      BAND_GEOMEAN,
      BAND_HARMEAN,
      BAND_SUM,
      BAND_STDDEV,
      BAND_MIN_ACCUM,
      BAND_MAX_ACCUM,
      BAND_MEAN_ACCUM,
      BAND_GEOMEAN_ACCUM,
      BAND_HARMEAN_ACCUM,
      BAND_SUM_ACCUM,
      BAND_STDDEV_ACCUM,
      ATAN2,
      LOGN,
      LOG10,
      LOG2,
      LOG,
      LESS_THAN,
      GREATER_THAN,
      EQUALS,
      NOT_EQUALS,
      LESS_OR_EQUAL,
      GREATER_OR_EQUAL,
      NOT,
      AND,
      OR,
      CLAMP,
      REFERENCE
   };
   ProcessStep(const std::string& description, StepType type) : 
      mDescription(description), 
      mStepType(type), 
      mRows(1),
      mColumns(1),
      mBands(1),
      mArgCount(2),
      mValue(0)
   {
   }

   bool isScalar() const
   {
      return (mRows==1)&&(mColumns==1)&&(mBands==1);
   }

   bool isSignature() const
   {
      return (mRows==1)&&(mColumns==1)&&(mBands!=1);
   }

   StepType type() const
   {
      return mStepType;
   }

   const std::string& description() const
   {
      return mDescription;
   }

   int rows() const
   {
      return mRows;
   }

   int columns() const
   {
      return mColumns;
   }

   int bands() const
   {
      return mBands;
   }

   double value() const
   {
      return mValue;
   }

   const double& valueRef() const
   {
      return mValue;
   }

   int argCount() const
   {
      return mArgCount;
   }

   virtual int64_t oneTimeWork() const
   {
      return 0;
   }

   virtual void initialize()
   {
   }

   virtual bool nextRow()
   {
      return true;
   }

   virtual bool nextColumn()
   {
      return true;
   }

   virtual bool operator==(const ProcessStep& rhs) const
   {
      if (rhs.mStepType != mStepType) return false;
      if (rhs.mRows != mRows) return false;
      if (rhs.mColumns != mColumns) return false;
      if (rhs.mBands != mBands) return false;
      if (rhs.mArgCount != mArgCount) return false;
      if (rhs.mDescription != mDescription) return false;
      return true;
   }

protected:
   std::string mDescription;
   StepType mStepType;
   int mRows;
   int mColumns;
   int mBands;
   int mArgCount;
   double mValue;
};

class ProcessStepNumber : public ProcessStep
{
public:
   ProcessStepNumber(const std::string& description, StepType type, double value) : 
      ProcessStep(description, type)
   {
      mArgCount = 0;
      mValue = value;
   }
};

class ProcessStepFunction : public ProcessStep
{
public:
   ProcessStepFunction(const std::string& description, StepType type, const std::vector<boost::shared_ptr<ProcessStep> >& args, int argCount);
};

class ProcessStepSignature : public ProcessStep
{
   friend class ProcessStack;
public:
   ProcessStepSignature(const std::string& description, Signature* pSignature, int bandCount);
   bool operator=(const ProcessStep& rhs) const
   {
      if (ProcessStep::operator ==(rhs))
      {
         return mpSignature == static_cast<const ProcessStepSignature&>(rhs).mpSignature;
      }
      return false;
   }

private:
   Signature* mpSignature;
   std::vector<double> mValues;
};

class ProcessStepAoi : public ProcessStep
{
   friend class ProcessStack;
public:
   ProcessStepAoi(const std::string& description);
   void initialize();
   bool nextRow();
   bool nextColumn();
   bool operator==(const ProcessStep& rhs) const
   {
      if (ProcessStep::operator ==(rhs))
      {
         return mpElement == static_cast<const ProcessStepAoi&>(rhs).mpElement;
      }
      return false;
   }

private:
   AoiElement* mpElement;
   const BitMask* mpMask;
   int mCurrentRow;
   int mCurrentColumn;
};

class ProcessStepRaster : public ProcessStep
{
   friend class ProcessStack;
public:
   ProcessStepRaster(const std::string& description, StepType type, int minBand, int maxBand);
   void initialize();
   bool nextRow();
   bool nextColumn();
   bool operator==(const ProcessStep& rhs) const
   {
      if (ProcessStep::operator ==(rhs))
      {
         const ProcessStepRaster& rasterStep = static_cast<const ProcessStepRaster&>(rhs);
         if (mpElement != rasterStep.mpElement) return false;
         if (mMinBand != rasterStep.mMinBand) return false;
         if (mMaxBand != rasterStep.mMaxBand) return false;
         return true;
      }
      return false;
   }

protected:
   void updateAccessor();
   int mMinBand;
   int mMaxBand;
   int mCurrentBand;
   int mCurrentRow;
   int mCurrentColumn;
   RasterElement* mpElement;
   EncodingType mEncodingType;
   DataAccessor mAccessor;
   double mDefaultValue;
};

class ProcessStepRasterResult : public ProcessStepRaster
{
   friend class ProcessStack;
public:
   ProcessStepRasterResult(int bandCount);
};

class ProcessStepReference : public ProcessStep
{
   friend class ProcessStack;
public:
   ProcessStepReference(const ProcessStep& ref) : ProcessStep("ref", REFERENCE), mRef(ref.valueRef()) {}
private:
   const double& mRef;
};

#endif
