/*
 * The information in this file is
 * Copyright(c) 2009 Todd A. Johnson
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "ProcessStepStatFunc.h"
#include "RasterMathException.h"
#include "RasterMathProgress.h"

#include <boost/bind.hpp>
#include <limits>

using namespace std;

ProcessStepStatFunc::ProcessStepStatFunc(const string& description, StepType type, const vector<boost::shared_ptr<ProcessStep> >& args, int argCount) : 
   ProcessStepFunction(description, type, args, argCount),
   mSubStackBands(mBands),
   mSubStackRows(mRows),
   mSubStackColumns(mColumns),
   mAccumulator1(0.0),
   mAccumulator2(0.0),
   mAccumulator3(0.0)
{
   mArgCount = 0;
   if (mRows == 1 && mColumns == 1)
   {
      mBands = 1;
   }

   mRows = mColumns = 1;

   initializeAccumulators();
}

void ProcessStepStatFunc::setSubStack(const vector<boost::shared_ptr<ProcessStep> >& subStack)
{
   for(vector<boost::shared_ptr<ProcessStep> >::const_iterator iter=subStack.begin(); iter!=subStack.end(); ++iter)
   {
      mSubStack.add(*iter);
   }
}

void ProcessStepStatFunc::initializeAccumulators()
{
   mAccumulator1 = mAccumulator2 = mAccumulator3 = 0.0;

   switch (mStepType)
   {
      case BAND_MIN:
         mAccumulator1 = std::numeric_limits<double>::max();
         break;
      case BAND_MAX:
         mAccumulator1 = -std::numeric_limits<double>::max();
         break;
   }
}

void ProcessStepStatFunc::execute(RasterMathProgress& progress)
{
   ProcessStack stack = mSubStack;
   mSubStack.clear();
   std::swap(mBands, mSubStackBands);
   std::swap(mRows, mSubStackRows);
   std::swap(mColumns, mSubStackColumns);
   mStepType = static_cast<ProcessStep::StepType>(mStepType+7);
   stack.execute(progress);
   std::swap(mBands, mSubStackBands);
   std::swap(mRows, mSubStackRows);
   std::swap(mColumns, mSubStackColumns);
   mStepType = ProcessStep::COMPUTED_SIGNATURE;
   mValue = mValues.front();
   mValues.pop_front();
}

int64_t ProcessStepStatFunc::oneTimeWork() const
{
   const vector<boost::shared_ptr<ProcessStep> >& steps = mSubStack.getSteps();
   int64_t work = static_cast<int64_t>(mSubStackBands) * mSubStackRows * mSubStackColumns * steps.size();
   for (vector<boost::shared_ptr<ProcessStep> >::const_iterator ppStep=steps.begin(); ppStep!=steps.end(); ++ppStep)
   {
      if (ppStep->get() != this)
      {
         work += RM_NULLCHK(*ppStep)->oneTimeWork();
      }
   }

   return work;
}

bool ProcessStepStatFunc::operator==(const ProcessStep& rhs) const
{
   if (!ProcessStep::operator ==(rhs))
   {
      return false;
   }

   const ProcessStepStatFunc& statRhs = static_cast<const ProcessStepStatFunc&>(rhs);

   const vector<boost::shared_ptr<ProcessStep> >& steps1 = mSubStack.getSteps();
   const vector<boost::shared_ptr<ProcessStep> >& steps2 = statRhs.mSubStack.getSteps();

   if (steps1.size() != steps2.size()) 
   {
      return false;
   }

   size_t count = steps1.size();
   for (size_t i=0; i<count-1; ++i)
   {
      if (!(*RM_NULLCHK(steps1[i].get()) == *RM_NULLCHK(steps2[i].get())))
      {
         return false;
      }
   }

   return true;
}